#include "Shuttle.h"
#include "Device.h"
#include "Task.h"
#include "Simulation.h"
#include "Rendering.h"
#include "Utils.h"  // 添加Utils.h，提供编码转换函数
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <ctime>

// 全局变量定义
std::vector<Shuttle> shuttles_global;

// 获取穿梭车状态的中文描述
std::string get_shuttle_state_str_cn(ShuttleAgentState state) {
    // 注意：这里返回的是UTF-8编码的字符串，在显示时需要转换为GBK
    switch (state) {
        case ShuttleAgentState::IDLE_EMPTY: return "空闲";
        case ShuttleAgentState::MOVING_TO_PICKUP: return "往取货点";
        case ShuttleAgentState::WAITING_FOR_PICKUP_AVAILABILITY: return "等待取货";
        case ShuttleAgentState::LOADING: return "装货中";
        case ShuttleAgentState::MOVING_TO_DROPOFF: return "往卸货点";
        case ShuttleAgentState::WAITING_FOR_DROPOFF_AVAILABILITY: return "等待卸货";
        case ShuttleAgentState::UNLOADING: return "卸货中";
        case ShuttleAgentState::PATROLLING_CRUISE: return "巡游";
        case ShuttleAgentState::PATROLLING_ACCEL: return "巡游-加速";
        case ShuttleAgentState::PATROLLING_DECEL: return "巡游-减速";
        case ShuttleAgentState::PATROLLING_STOPPED: return "巡游-停止";
        default: return "未知状态";
    }
}

// Shuttle构造函数实现
Shuttle::Shuttle() : id(-1), position_mm(0), speed_mm_s(0), current_acceleration_mm_s2(0),
                agent_state(ShuttleAgentState::IDLE_EMPTY), assigned_task_idx(-1),
                target_pos_mm(0), has_target_pos(false), operation_timer_s(0),
                total_run_time_task1_s(0), stop_count_task1(0),
                current_phase_start_time_s(0), current_phase_start_speed_mm_s(0),
                current_phase_acceleration_mm_s2(0), in_logged_accel_decel_phase(false),
                initial_position_mm(0), completed_full_circle(false), distance_traveled_mm(0),
                is_stopped_at_station(false), stopped_position_mm(0),
                color(RED) {}

// 判断穿梭车是否载有货物
bool Shuttle::has_goods() const {
    if (RUN_TASK1_MODE) return false;
    return agent_state == ShuttleAgentState::LOADING ||
           agent_state == ShuttleAgentState::MOVING_TO_DROPOFF ||
           agent_state == ShuttleAgentState::WAITING_FOR_DROPOFF_AVAILABILITY ||
           agent_state == ShuttleAgentState::UNLOADING;
}

// 记录穿梭车移动阶段（任务1）
void log_shuttle_movement_phase_task1(Shuttle& shuttle) {
    if (!RUN_TASK1_MODE || !shuttle.in_logged_accel_decel_phase) return;

    MovementRecord rec;
    rec.start_time_s = shuttle.current_phase_start_time_s;
    rec.end_time_s = CURRENT_SIM_TIME_S;
    rec.start_speed_mm_s = shuttle.current_phase_start_speed_mm_s;
    rec.end_speed_mm_s = shuttle.speed_mm_s;
    rec.acceleration_mm_s2 = shuttle.current_phase_acceleration_mm_s2;

    // 记录加减速信息到日志
    std::stringstream debug_ss;
    debug_ss << "穿梭车 S" << shuttle.id << " 加减速记录: "
             << "开始时间=" << std::fixed << std::setprecision(2) << rec.start_time_s << "s, "
             << "结束时间=" << std::fixed << std::setprecision(2) << rec.end_time_s << "s, "
             << "初速度=" << std::fixed << std::setprecision(2) << rec.start_speed_mm_s << "mm/s, "
             << "末速度=" << std::fixed << std::setprecision(2) << rec.end_speed_mm_s << "mm/s, "
             << "加速度=" << std::fixed << std::setprecision(2) << rec.acceleration_mm_s2 << "mm/s²";
    add_debug_message(debug_ss.str());

    // 根据要求记录所有加减速数据，但过滤掉微小变化
    // 只有当加速度不为0且速度变化明显时才记录
    if (std::abs(rec.acceleration_mm_s2) > 1e-3 &&
        std::abs(rec.end_speed_mm_s - rec.start_speed_mm_s) > 1.0) {
        shuttle.movement_log_task1.push_back(rec);
    }

    shuttle.in_logged_accel_decel_phase = false;
}

// 初始化穿梭车
void init_shuttles(int num_shuttles) {
    shuttles_global.clear();

    // 查找15号出库口位置
    double pos_dev15 = POS_BOTTOM_STRAIGHT_START;
    bool found_dev15 = false;

    if (devices_global.find(15) != devices_global.end()) {
        pos_dev15 = devices_global.at(15).position_on_track_mm;
        found_dev15 = true;
        std::cout << "找到15号出库口，位置: " << pos_dev15 << " mm" << std::endl;
    } else {
        std::cerr << "错误: 初始化穿梭车时未找到15号出库口!" << std::endl;
        // 尝试找到任意出库口作为备用
        for(const auto& pair : devices_global){
            if(pair.second.model_type == DeviceModelType::OUT_PORT){
                pos_dev15 = pair.second.position_on_track_mm;
                found_dev15 = true;
                std::cout << "警告: 未找到15号出库口，使用设备 " << pair.first << " 位置: " << pos_dev15 << " mm" << std::endl;
                break;
            }
        }

        if (!found_dev15) {
            std::cout << "警告: 未找到任何出库口，使用默认位置。" << std::endl;
        }
    }

    COLORREF colors[] = { RED, GREEN, BLUE, YELLOW, MAGENTA, CYAN, BROWN,
                          LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED, LIGHTMAGENTA, DARKGRAY };

    // 创建穿梭车
    for (int i = 0; i < num_shuttles; ++i) {
        Shuttle s;
        s.id = i + 1;

        // 设置初始位置：第1辆与15号出库口精确对齐，其他车辆间距更大以避免碰撞
        if (i == 0) {
            s.position_mm = pos_dev15;
        } else {
            // 增加车辆间距，避免碰撞
            double prev_shuttle_pos = shuttles_global[i - 1].position_mm;
            // 使用更大的间距：车长 + 安全距离的3倍
            double safe_distance = SHUTTLE_LENGTH_MM + MIN_INTER_SHUTTLE_DISTANCE_MM * 3.0;
            s.position_mm = normalize_track_pos(prev_shuttle_pos - safe_distance);
        }

        // 记录详细的初始位置信息
        std::stringstream pos_debug_ss;
        pos_debug_ss << "穿梭车 S" << s.id << " 初始位置设置为: " << std::fixed << std::setprecision(2) << s.position_mm
                    << " mm，与15号出库口的距离: " << std::fixed << std::setprecision(2)
                    << distance_on_track(s.position_mm, pos_dev15) << " mm";
        add_debug_message(pos_debug_ss.str());

        // 记录初始位置，用于判断是否完成一圈
        s.initial_position_mm = s.position_mm;
        s.completed_full_circle = false;
        s.distance_traveled_mm = 0.0;

        // 初始状态：所有穿梭车均处于停止状态
        s.speed_mm_s = 0.0;
        s.current_acceleration_mm_s2 = 0.0;

        // 设置初始状态
        if (RUN_TASK1_MODE) {
            // 任务1模式：所有车辆都设置为准备加速状态，但错开启动时间
            s.agent_state = ShuttleAgentState::PATROLLING_ACCEL;
            // 设置更大的启动间隔，确保车辆不会碰撞
            // 第一辆车先启动，后面的车辆延迟更长时间
            if (i == 0) {
                s.operation_timer_s = 0.0; // 第一辆车立即启动
            } else {
                // 设置足够长的启动间隔，确保前车有足够时间先行驶
                s.operation_timer_s = i * 10.0; // 每辆车错开10秒启动
            }

            // 初始速度设为0，所有车辆从停止状态开始
            s.speed_mm_s = 0.0;

            // 设置初始巡游方向 - 让每辆车选择不同的目标点
            std::vector<int> io_port_ids;

            // 收集所有出入库口
            for(const auto& pair : devices_global) {
                if (pair.second.model_type == DeviceModelType::IN_PORT ||
                    pair.second.model_type == DeviceModelType::OUT_PORT) {
                    io_port_ids.push_back(pair.first);
                }
            }

            // 如果找到了出入库口，为每辆车选择不同的目标
            if (!io_port_ids.empty()) {
                // 为每辆车选择不同的目标点，避免所有车都去同一个地方
                // 使用车辆ID作为随机种子，确保不同车选择不同目标
                srand(time(NULL) + s.id * 100);

                // 随机选择一个出入库口作为目标
                int random_index = rand() % io_port_ids.size();
                int target_port_id = io_port_ids[random_index];

                // 计算与目标的距离
                double target_dist = distance_on_track(s.position_mm, devices_global.at(target_port_id).position_on_track_mm);

                // 设置目标
                s.target_pos_mm = devices_global.at(target_port_id).position_on_track_mm;
                s.has_target_pos = true;

                std::stringstream target_ss;
                target_ss << "穿梭车 S" << s.id << " 初始目标设置为: 设备 " << target_port_id
                         << " (" << (devices_global.at(target_port_id).model_type == DeviceModelType::IN_PORT ? "入库口" : "出库口")
                         << ")，距离: " << std::fixed << std::setprecision(2) << target_dist << " mm";
                add_debug_message(target_ss.str());
            } else {
                // 如果没有找到出入库口，不设置目标
                s.has_target_pos = false;
            }

            std::stringstream debug_ss;
            debug_ss << "穿梭车 S" << s.id << " 将在 " << s.operation_timer_s << " 秒后启动，初始速度: "
                    << std::fixed << std::setprecision(2) << s.speed_mm_s << " mm/s";
            add_debug_message(debug_ss.str());

            // 确保穿梭车不会立即停车
            s.stop_count_task1 = 0;
        } else {
            // 任务2模式
            s.agent_state = ShuttleAgentState::IDLE_EMPTY;
            s.has_target_pos = false;
        }

        s.assigned_task_idx = -1;
        // 注意：不要在这里重置has_target_pos，因为我们可能已经在上面设置了它
        s.color = colors[i % (sizeof(colors)/sizeof(COLORREF))];

        // 初始化任务1相关统计数据
        s.total_run_time_task1_s = 0.0;
        s.stop_count_task1 = 0;
        s.movement_log_task1.clear();
        s.in_logged_accel_decel_phase = false;
        s.current_phase_start_time_s = 0.0;
        s.current_phase_start_speed_mm_s = 0.0;
        s.current_phase_acceleration_mm_s2 = 0.0;

        // 初始化碰撞检测相关变量
        s.is_stopped_at_station = false;
        s.stopped_position_mm = 0.0;

        shuttles_global.push_back(s);

        // 记录初始位置信息
        std::stringstream debug_ss;
        debug_ss << "初始化穿梭车 S" << s.id << " 位置: " << s.position_mm << " mm";
        add_debug_message(debug_ss.str());
    }
}

// 绘制穿梭车 - 已移至Rendering.cpp
// void draw_shuttles(const std::vector<Shuttle>& shuttles_vec, const std::vector<Task>& tasks_vec) {
// }

// 更新穿梭车物理状态和逻辑状态
void update_shuttle_physics_and_state(Shuttle& shuttle, int shuttle_idx, std::vector<Shuttle>& all_shuttles,
                                     std::map<int, Device>& devices, std::vector<Task>& tasks) {
    // 保存当前位置和速度，用于后续计算
    double prev_pos_mm = shuttle.position_mm;
    double prev_speed_mm_s = shuttle.speed_mm_s;

    // 任务1模式：巡游模式
    if (RUN_TASK1_MODE) {
        // 更新穿梭车位置
        shuttle.position_mm += shuttle.speed_mm_s * SIM_TIME_STEP_S;

        // 确保位置在轨道范围内（循环轨道）
        shuttle.position_mm = normalize_track_pos(shuttle.position_mm);

        // 更新行驶距离
        double distance_this_step = shuttle.speed_mm_s * SIM_TIME_STEP_S;
        shuttle.distance_traveled_mm += distance_this_step;

        // 检查是否完成一圈
        if (!shuttle.completed_full_circle &&
            shuttle.distance_traveled_mm >= TOTAL_TRACK_LENGTH) {
            shuttle.completed_full_circle = true;
            std::stringstream ss;
            ss << "穿梭车 S" << shuttle.id << " 已完成一整圈巡游，总行驶距离: "
               << std::fixed << std::setprecision(2) << shuttle.distance_traveled_mm << " mm";
            add_debug_message(ss.str());
        }

        // 根据当前状态更新速度和加速度
        switch (shuttle.agent_state) {
            case ShuttleAgentState::PATROLLING_CRUISE:
                // 巡游状态，保持匀速
                shuttle.current_acceleration_mm_s2 = 0;

                // 检查是否需要减速（接近站点）
                for (const auto& pair : devices) {
                    const Device& dev = pair.second;
                    double dist_to_device = distance_on_track(shuttle.position_mm, dev.position_on_track_mm);

                    // 如果距离站点小于减速距离，开始减速
                    if (dist_to_device < 500 && shuttle.speed_mm_s > 0) {
                        shuttle.agent_state = ShuttleAgentState::PATROLLING_DECEL;
                        shuttle.current_acceleration_mm_s2 = -1000; // 减速度

                        // 记录减速开始信息
                        shuttle.current_phase_start_time_s = CURRENT_SIM_TIME_S;
                        shuttle.current_phase_start_speed_mm_s = shuttle.speed_mm_s;
                        shuttle.current_phase_acceleration_mm_s2 = shuttle.current_acceleration_mm_s2;
                        shuttle.in_logged_accel_decel_phase = true;
                        break;
                    }
                }
                break;

            case ShuttleAgentState::PATROLLING_DECEL:
                // 减速状态，应用减速度
                shuttle.speed_mm_s += shuttle.current_acceleration_mm_s2 * SIM_TIME_STEP_S;

                // 如果速度接近0，停止
                if (shuttle.speed_mm_s <= 10) {
                    shuttle.speed_mm_s = 0;
                    shuttle.agent_state = ShuttleAgentState::PATROLLING_STOPPED;
                    shuttle.stop_count_task1++;

                    // 记录停止位置
                    shuttle.is_stopped_at_station = true;
                    shuttle.stopped_position_mm = shuttle.position_mm;

                    // 记录减速阶段
                    log_shuttle_movement_phase_task1(shuttle);

                    // 设置停止计时器
                    shuttle.operation_timer_s = 2.0; // 停止2秒
                }
                break;

            case ShuttleAgentState::PATROLLING_STOPPED:
                // 停止状态，等待计时器结束
                shuttle.operation_timer_s -= SIM_TIME_STEP_S;

                if (shuttle.operation_timer_s <= 0) {
                    shuttle.agent_state = ShuttleAgentState::PATROLLING_ACCEL;
                    shuttle.current_acceleration_mm_s2 = 500; // 加速度

                    // 记录加速开始信息
                    shuttle.current_phase_start_time_s = CURRENT_SIM_TIME_S;
                    shuttle.current_phase_start_speed_mm_s = shuttle.speed_mm_s;
                    shuttle.current_phase_acceleration_mm_s2 = shuttle.current_acceleration_mm_s2;
                    shuttle.in_logged_accel_decel_phase = true;
                }
                break;

            case ShuttleAgentState::PATROLLING_ACCEL:
                // 加速状态，应用加速度
                shuttle.speed_mm_s += shuttle.current_acceleration_mm_s2 * SIM_TIME_STEP_S;

                // 如果达到巡游速度，切换到巡游状态
                if (shuttle.speed_mm_s >= 1000) {
                    shuttle.speed_mm_s = 1000; // 巡游速度
                    shuttle.agent_state = ShuttleAgentState::PATROLLING_CRUISE;

                    // 记录加速阶段
                    log_shuttle_movement_phase_task1(shuttle);
                }
                break;

            default:
                // 初始状态，设置为巡游状态
                shuttle.agent_state = ShuttleAgentState::PATROLLING_CRUISE;
                shuttle.speed_mm_s = 1000; // 巡游速度
                shuttle.current_acceleration_mm_s2 = 0;
        }
    }
    // 任务2模式：任务调度模式
    else {
        // 根据当前状态更新穿梭车
        switch (shuttle.agent_state) {
            case ShuttleAgentState::IDLE_EMPTY:
                // 空闲状态，等待任务分配
                shuttle.current_acceleration_mm_s2 = 0;
                break;

            case ShuttleAgentState::MOVING_TO_PICKUP:
                // 前往取货点
                if (shuttle.has_target_pos) {
                    // 计算到目标的距离
                    double dist_to_target = distance_on_track(shuttle.position_mm, shuttle.target_pos_mm);

                    // 根据距离调整速度
                    if (dist_to_target > 1000) {
                        // 远距离，加速到巡航速度
                        if (shuttle.speed_mm_s < 1000) {
                            shuttle.current_acceleration_mm_s2 = 500;
                            shuttle.speed_mm_s += shuttle.current_acceleration_mm_s2 * SIM_TIME_STEP_S;
                            if (shuttle.speed_mm_s > 1000) shuttle.speed_mm_s = 1000;
                        } else {
                            shuttle.current_acceleration_mm_s2 = 0;
                        }
                    } else if (dist_to_target > 100) {
                        // 中距离，开始减速
                        if (shuttle.speed_mm_s > 200) {
                            shuttle.current_acceleration_mm_s2 = -500;
                            shuttle.speed_mm_s += shuttle.current_acceleration_mm_s2 * SIM_TIME_STEP_S;
                            if (shuttle.speed_mm_s < 200) shuttle.speed_mm_s = 200;
                        } else {
                            shuttle.current_acceleration_mm_s2 = 0;
                        }
                    } else if (dist_to_target > 10) {
                        // 近距离，低速接近
                        if (shuttle.speed_mm_s > 50) {
                            shuttle.current_acceleration_mm_s2 = -200;
                            shuttle.speed_mm_s += shuttle.current_acceleration_mm_s2 * SIM_TIME_STEP_S;
                            if (shuttle.speed_mm_s < 50) shuttle.speed_mm_s = 50;
                        } else {
                            shuttle.current_acceleration_mm_s2 = 0;
                        }
                    } else {
                        // 到达目标，停止
                        shuttle.speed_mm_s = 0;
                        shuttle.current_acceleration_mm_s2 = 0;
                        shuttle.position_mm = shuttle.target_pos_mm;
                        shuttle.has_target_pos = false;

                        // 更新状态为等待取货
                        shuttle.agent_state = ShuttleAgentState::WAITING_FOR_PICKUP_AVAILABILITY;

                        // 如果有分配的任务，更新任务状态
                        if (shuttle.assigned_task_idx != -1 && shuttle.assigned_task_idx < (int)tasks.size()) {
                            Task& task = tasks[shuttle.assigned_task_idx];
                            task.status = TaskStatus::SHUTTLE_WAITING_AT_PICKUP;

                            // 如果设备有货物，开始装货
                            if (devices.find(task.start_device_id) != devices.end()) {
                                Device& dev = devices[task.start_device_id];
                                if (dev.current_task_idx_on_device == shuttle.assigned_task_idx &&
                                    dev.op_state == DeviceOperationalState::HAS_GOODS_WAITING_FOR_SHUTTLE) {

                                    // 更新设备状态
                                    dev.op_state = DeviceOperationalState::HAS_GOODS_BEING_ACCESSED_BY_SHUTTLE;
                                    dev.last_state_change_time_s = CURRENT_SIM_TIME_S;

                                    // 更新穿梭车状态
                                    shuttle.agent_state = ShuttleAgentState::LOADING;
                                    shuttle.operation_timer_s = 3.0; // 装货时间3秒

                                    // 更新任务状态
                                    task.status = TaskStatus::SHUTTLE_LOADING;

                                    // 记录日志
                                    log_device_state_change(dev.id, task.material_id, "开始装货");
                                }
                            }
                        }
                    }
                }

                // 更新位置
                shuttle.position_mm += shuttle.speed_mm_s * SIM_TIME_STEP_S;
                shuttle.position_mm = normalize_track_pos(shuttle.position_mm);
                break;

            case ShuttleAgentState::WAITING_FOR_PICKUP_AVAILABILITY:
                // 等待取货点可用
                // 这里可以添加等待逻辑
                break;

            case ShuttleAgentState::LOADING:
                // 装货中
                shuttle.operation_timer_s -= SIM_TIME_STEP_S;

                if (shuttle.operation_timer_s <= 0) {
                    // 装货完成
                    if (shuttle.assigned_task_idx != -1 && shuttle.assigned_task_idx < (int)tasks.size()) {
                        Task& task = tasks[shuttle.assigned_task_idx];

                        // 更新任务状态
                        task.status = TaskStatus::SHUTTLE_MOVING_TO_DROPOFF;
                        task.time_pickup_complete_s = CURRENT_SIM_TIME_S;

                        // 更新设备状态
                        if (devices.find(task.start_device_id) != devices.end()) {
                            Device& dev = devices[task.start_device_id];
                            dev.op_state = DeviceOperationalState::IDLE_EMPTY;
                            dev.current_task_idx_on_device = -1;
                            dev.last_state_change_time_s = CURRENT_SIM_TIME_S;

                            // 记录日志
                            log_device_state_change(dev.id, task.material_id, "装货完成，设备空闲");
                        }

                        // 更新穿梭车状态
                        shuttle.agent_state = ShuttleAgentState::MOVING_TO_DROPOFF;

                        // 设置目标为卸货点
                        if (devices.find(task.end_device_id) != devices.end()) {
                            Device& end_dev = devices[task.end_device_id];
                            shuttle.target_pos_mm = end_dev.position_on_track_mm;
                            shuttle.has_target_pos = true;
                        }
                    }
                }
                break;

            case ShuttleAgentState::MOVING_TO_DROPOFF:
                // 前往卸货点，逻辑类似于前往取货点
                if (shuttle.has_target_pos) {
                    // 计算到目标的距离
                    double dist_to_target = distance_on_track(shuttle.position_mm, shuttle.target_pos_mm);

                    // 根据距离调整速度
                    if (dist_to_target > 1000) {
                        // 远距离，加速到巡航速度
                        if (shuttle.speed_mm_s < 1000) {
                            shuttle.current_acceleration_mm_s2 = 500;
                            shuttle.speed_mm_s += shuttle.current_acceleration_mm_s2 * SIM_TIME_STEP_S;
                            if (shuttle.speed_mm_s > 1000) shuttle.speed_mm_s = 1000;
                        } else {
                            shuttle.current_acceleration_mm_s2 = 0;
                        }
                    } else if (dist_to_target > 100) {
                        // 中距离，开始减速
                        if (shuttle.speed_mm_s > 200) {
                            shuttle.current_acceleration_mm_s2 = -500;
                            shuttle.speed_mm_s += shuttle.current_acceleration_mm_s2 * SIM_TIME_STEP_S;
                            if (shuttle.speed_mm_s < 200) shuttle.speed_mm_s = 200;
                        } else {
                            shuttle.current_acceleration_mm_s2 = 0;
                        }
                    } else if (dist_to_target > 10) {
                        // 近距离，低速接近
                        if (shuttle.speed_mm_s > 50) {
                            shuttle.current_acceleration_mm_s2 = -200;
                            shuttle.speed_mm_s += shuttle.current_acceleration_mm_s2 * SIM_TIME_STEP_S;
                            if (shuttle.speed_mm_s < 50) shuttle.speed_mm_s = 50;
                        } else {
                            shuttle.current_acceleration_mm_s2 = 0;
                        }
                    } else {
                        // 到达目标，停止
                        shuttle.speed_mm_s = 0;
                        shuttle.current_acceleration_mm_s2 = 0;
                        shuttle.position_mm = shuttle.target_pos_mm;
                        shuttle.has_target_pos = false;

                        // 更新状态为等待卸货
                        shuttle.agent_state = ShuttleAgentState::WAITING_FOR_DROPOFF_AVAILABILITY;

                        // 如果有分配的任务，更新任务状态
                        if (shuttle.assigned_task_idx != -1 && shuttle.assigned_task_idx < (int)tasks.size()) {
                            Task& task = tasks[shuttle.assigned_task_idx];
                            task.status = TaskStatus::SHUTTLE_WAITING_AT_DROPOFF;

                            // 如果设备可用，开始卸货
                            if (devices.find(task.end_device_id) != devices.end()) {
                                Device& dev = devices[task.end_device_id];
                                if (dev.op_state == DeviceOperationalState::IDLE_EMPTY) {
                                    // 更新设备状态
                                    dev.op_state = DeviceOperationalState::HAS_GOODS_BEING_ACCESSED_BY_SHUTTLE;
                                    dev.current_task_idx_on_device = shuttle.assigned_task_idx;
                                    dev.last_state_change_time_s = CURRENT_SIM_TIME_S;

                                    // 更新穿梭车状态
                                    shuttle.agent_state = ShuttleAgentState::UNLOADING;
                                    shuttle.operation_timer_s = 3.0; // 卸货时间3秒

                                    // 更新任务状态
                                    task.status = TaskStatus::SHUTTLE_UNLOADING;

                                    // 记录日志
                                    log_device_state_change(dev.id, task.material_id, "开始卸货");
                                }
                            }
                        }
                    }
                }

                // 更新位置
                shuttle.position_mm += shuttle.speed_mm_s * SIM_TIME_STEP_S;
                shuttle.position_mm = normalize_track_pos(shuttle.position_mm);
                break;

            case ShuttleAgentState::WAITING_FOR_DROPOFF_AVAILABILITY:
                // 等待卸货点可用
                // 这里可以添加等待逻辑
                break;

            case ShuttleAgentState::UNLOADING:
                // 卸货中
                shuttle.operation_timer_s -= SIM_TIME_STEP_S;

                if (shuttle.operation_timer_s <= 0) {
                    // 卸货完成
                    if (shuttle.assigned_task_idx != -1 && shuttle.assigned_task_idx < (int)tasks.size()) {
                        Task& task = tasks[shuttle.assigned_task_idx];

                        // 更新任务状态
                        task.status = TaskStatus::GOODS_AT_DEST_AWAITING_REMOVAL;
                        task.time_dropoff_complete_s = CURRENT_SIM_TIME_S;
                        task.is_actively_handled_by_shuttle = false;

                        // 更新设备状态
                        if (devices.find(task.end_device_id) != devices.end()) {
                            Device& dev = devices[task.end_device_id];
                            dev.op_state = DeviceOperationalState::HAS_GOODS_WAITING_FOR_CLEARANCE;
                            dev.last_state_change_time_s = CURRENT_SIM_TIME_S;

                            // 记录日志
                            log_device_state_change(dev.id, task.material_id, "卸货完成，等待清理");

                            // 设置设备清理计时器
                            dev.busy_timer_s = 5.0; // 清理时间5秒
                        }

                        // 更新穿梭车状态
                        shuttle.agent_state = ShuttleAgentState::IDLE_EMPTY;
                        shuttle.assigned_task_idx = -1;

                        // 记录任务完成
                        log_task_event_completion(task);
                    }
                }
                break;

            default:
                // 未知状态，设置为空闲
                shuttle.agent_state = ShuttleAgentState::IDLE_EMPTY;
                shuttle.current_acceleration_mm_s2 = 0;
        }
    }

    // 检查穿梭车之间的碰撞
    for (int i = 0; i < (int)all_shuttles.size(); ++i) {
        if (i == shuttle_idx) continue;

        Shuttle& other = all_shuttles[i];
        double dist = distance_on_track(shuttle.position_mm, other.position_mm);

        // 如果距离小于安全距离，减速或停止
        if (dist < 300) {
            // 如果两车都在移动，根据优先级决定哪个停止
            if (shuttle.speed_mm_s > 0 && other.speed_mm_s > 0) {
                // 简单策略：ID小的优先
                if (shuttle.id > other.id) {
                    shuttle.speed_mm_s = 0;
                    shuttle.current_acceleration_mm_s2 = 0;
                }
            }
            // 如果只有当前车在移动，则停止
            else if (shuttle.speed_mm_s > 0) {
                shuttle.speed_mm_s = 0;
                shuttle.current_acceleration_mm_s2 = 0;
            }
        }
    }
}
