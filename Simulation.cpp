#include "Simulation.h"
#include "Shuttle.h"
#include "Device.h"
#include "Task.h"
#include "Rendering.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <ctime>
#include <conio.h>

// 轨道参数
const double TOTAL_TRACK_LENGTH = 80000.0;
const double POS_TOP_STRAIGHT_START = 0.0;
const double POS_FIRST_CURVE_START = 30000.0;
const double POS_FIRST_CURVE_END = 40000.0;
const double POS_BOTTOM_STRAIGHT_START = 40000.0;
const double POS_SECOND_CURVE_START = 70000.0;
const double POS_SECOND_CURVE_END = 80000.0;

// 添加与main.cpp相同的轨道段长度常量
const double TRACK_STRAIGHT_LENGTH = 30000.0;  // 直道长度
const double TRACK_CURVE_LENGTH = 10000.0;     // 弯道长度

// 穿梭车参数
const double MAX_SPEED_STRAIGHT_MM_PER_S = 200.0 * 1000.0 / 60.0; // 3333.33 mm/s (increased from 160m/min to 200m/min)
const double MAX_SPEED_CURVE_MM_PER_S = 80.0 * 1000.0 / 60.0;    // 1333.33 mm/s (increased from 40m/min to 80m/min)
const double ACCEL_MM_PER_S2 = 0.8 * 1000.0;                     // 800 mm/s^2 (increased from 0.5m/s² to 0.8m/s²)
const double DECEL_MM_PER_S2 = 0.8 * 1000.0;                     // 800 mm/s^2 (increased from 0.5m/s² to 0.8m/s²)
const double MIN_INTER_SHUTTLE_DISTANCE_MM = 200.0;
const double LOAD_UNLOAD_TIME_S = 7.5;
const double SHUTTLE_LENGTH_MM = 2000.0;

// 设备操作时间
const double STACKER_TO_OUT_INTERFACE_TIME_S = 50.0;
const double STACKER_FROM_IN_INTERFACE_TIME_S = 25.0;
const double OPERATOR_AT_OUT_PORT_TIME_S = 30.0;
const double OPERATOR_AT_IN_PORT_TIME_S = 30.0;

// 绘图参数
double DRAW_SCALE = 1.0;
double DRAW_OFFSET_X = 0.0;
double DRAW_OFFSET_Y = 0.0;

// 仿真设置
double SIM_TIME_STEP_S = 0.05; // 20 Hz simulation update
double CURRENT_SIM_TIME_S = 0.0;
double SIM_SPEED_MULTIPLIER = 10.0; // 默认提高到10倍速
bool PAUSED = false;
bool RUN_TASK1_MODE = false; // Default to Task 2, can be toggled

// 日志文件
std::ofstream debug_log_file;
std::ofstream device_state_log_file;
std::ofstream task_event_log_file;
std::ofstream task_exe_log_file;

// 初始化仿真
void init_simulation() {
    // 初始化随机数生成器
    srand(static_cast<unsigned int>(time(nullptr)));

    // 初始化仿真时间
    CURRENT_SIM_TIME_S = 0.0;
    PAUSED = false;

    // 打开日志文件
    debug_log_file.open("DebugLog.txt");
    device_state_log_file.open("DeviceStateLog.txt");
    task_event_log_file.open("TaskEventLog.txt");

    // 写入日志文件头
    debug_log_file << "时间(s),消息" << std::endl;
    device_state_log_file << "时间(s),设备ID,物料ID,状态变化" << std::endl;
    task_event_log_file << "时间(s),任务ID,物料ID,起点,终点,事件" << std::endl;

    // 初始化设备和任务
    init_devices();

    // 初始化任务
    if (!RUN_TASK1_MODE) {
        load_tasks_from_markdown_data();
        init_task2_device_initial_tasks();
    } else {
        all_tasks_global.clear();
        pending_task_queues_by_start_device.clear();
    }
}

// 记录调试信息
void add_debug_message(const std::string& message) {
    std::cout << "[" << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << "s] " << message << std::endl;
    if (debug_log_file.is_open()) {
        debug_log_file << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << "," << message << std::endl;
    }
}

// 记录设备状态变化
void log_device_state_change(int log_entity_id, const std::string& material_id_str, const std::string& change_description, bool is_shuttle) {
    if (!device_state_log_file.is_open()) return;

    device_state_log_file << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << ","
                          << (is_shuttle ? "S" : "") << log_entity_id << ","
                          << material_id_str << ","
                          << change_description << "\n";
    device_state_log_file.flush();
}

// 记录任务事件完成
void log_task_event_completion(const Task& task) {
    if (!task_event_log_file.is_open()) return;

    task_event_log_file << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << ","
                       << task.id << ","
                       << task.material_id << ","
                       << task.start_device_id << ","
                       << task.end_device_id << ","
                       << get_task_status_str(task.status) << "\n";
    task_event_log_file.flush();
}

// 关闭日志文件
void close_log_files() {
    if (debug_log_file.is_open()) debug_log_file.close();
    if (device_state_log_file.is_open()) device_state_log_file.close();
    if (task_event_log_file.is_open()) task_event_log_file.close();
}

// 输出任务1总结
void output_task1_summary() {
    std::cout << "\n任务1总结：" << std::endl;

    // 确保日志文件已打开
    if (!debug_log_file.is_open()) {
        debug_log_file.open("DebugLog.txt", std::ios::app);
    }

    // 写入总结标题到日志
    debug_log_file << "\n========== 任务1总结 ==========" << std::endl;
    debug_log_file << "仿真时间: " << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << " 秒" << std::endl;

    for (const auto& shuttle : shuttles_global) {
        std::stringstream shuttle_info;
        shuttle_info << "\n穿梭车 " << shuttle.id << ":" << std::endl;
        shuttle_info << "  总运行时间: " << std::fixed << std::setprecision(2) << shuttle.total_run_time_task1_s << " 秒" << std::endl;
        shuttle_info << "  停车次数: " << shuttle.stop_count_task1 << std::endl;
        shuttle_info << "  总行驶距离: " << std::fixed << std::setprecision(2) << shuttle.distance_traveled_mm << " mm ("
                     << std::fixed << std::setprecision(2) << shuttle.distance_traveled_mm / 1000.0 << " m)" << std::endl;
        shuttle_info << "  是否完成一整圈巡游: " << (shuttle.completed_full_circle ? "是" : "否") << std::endl;
        shuttle_info << "  加减速记录:" << std::endl;

        if (shuttle.movement_log_task1.empty()) {
            shuttle_info << "    无加减速记录。" << std::endl;
        }

        // 同时写入日志文件
        debug_log_file << shuttle_info.str();

        for (const auto& rec : shuttle.movement_log_task1) {
            shuttle_info << "    加减速记录 #" << (&rec - &shuttle.movement_log_task1[0] + 1) << ":" << std::endl;
            shuttle_info << "      起始时间: " << std::fixed << std::setprecision(2) << rec.start_time_s << " 秒" << std::endl;
            shuttle_info << "      起始速度: " << std::fixed << std::setprecision(2) << rec.start_speed_mm_s << " mm/s" << std::endl;
            shuttle_info << "      起始加速度: " << std::fixed << std::setprecision(2) << rec.acceleration_mm_s2 << " mm/s²" << std::endl;
            shuttle_info << "      终止时间: " << std::fixed << std::setprecision(2) << rec.end_time_s << " 秒" << std::endl;
            shuttle_info << "      终止速度: " << std::fixed << std::setprecision(2) << rec.end_speed_mm_s << " mm/s" << std::endl;
            shuttle_info << "      终止加速度: " << std::fixed << std::setprecision(2) << rec.acceleration_mm_s2 << " mm/s²" << std::endl;

            // 同时写入日志文件
            debug_log_file << shuttle_info.str();
        }

        // 输出到控制台
        std::cout << shuttle_info.str();
    }

    // 刷新日志文件
    debug_log_file.flush();
}

// 输出任务2总结文件
void output_task2_summary_files() {
    // 实现任务2总结输出
}

// 工具函数：规范化轨道位置
double normalize_track_pos(double pos_mm) {
    pos_mm = fmod(pos_mm, TOTAL_TRACK_LENGTH);
    if (pos_mm < 0) {
        pos_mm += TOTAL_TRACK_LENGTH;
    }
    return pos_mm;
}

// 工具函数：计算轨道上两点之间的距离
double distance_on_track(double pos1_mm, double pos2_mm) {
    pos1_mm = normalize_track_pos(pos1_mm);
    pos2_mm = normalize_track_pos(pos2_mm);
    if (pos2_mm >= pos1_mm) {
        return pos2_mm - pos1_mm;
    } else {
        return (TOTAL_TRACK_LENGTH - pos1_mm) + pos2_mm;
    }
}

// 工具函数：判断位置是否在曲线上
bool is_on_curve(double pos_mm) {
    pos_mm = normalize_track_pos(pos_mm);
    return (pos_mm >= POS_FIRST_CURVE_START && pos_mm < POS_FIRST_CURVE_END) ||
           (pos_mm >= POS_SECOND_CURVE_START && pos_mm < POS_SECOND_CURVE_END);
}

// 工具函数：获取指定位置的最大速度
double get_max_speed_at_pos(double pos_mm) {
    return is_on_curve(pos_mm) ? MAX_SPEED_CURVE_MM_PER_S : MAX_SPEED_STRAIGHT_MM_PER_S;
}

// 工具函数：判断位置是否在两点之间
bool is_position_between(double pos, double start, double end) {
    start = normalize_track_pos(start);
    end = normalize_track_pos(end);
    pos = normalize_track_pos(pos);

    if (start <= end) {
        return pos >= start && pos <= end;
    } else {
        return pos >= start || pos <= end;
    }
}

// 任务分配函数
void assign_tasks_to_shuttles(std::vector<Shuttle>& shuttles, std::vector<Task>& tasks, std::map<int, Device>& devices) {
    // 遍历所有穿梭车
    for (auto& shuttle : shuttles) {
        // 如果穿梭车已经有任务，跳过
        if (shuttle.assigned_task_idx != -1 || shuttle.agent_state != ShuttleAgentState::IDLE_EMPTY) {
            continue;
        }

        // 寻找最近的待处理任务
        int best_task_idx = -1;
        double min_distance = TOTAL_TRACK_LENGTH;

        for (size_t i = 0; i < tasks.size(); ++i) {
            Task& task = tasks[i];

            // 只考虑准备好取货的任务
            if (task.status == TaskStatus::READY_FOR_PICKUP && !task.is_actively_handled_by_shuttle) {
                // 获取任务起点设备位置
                if (devices.find(task.start_device_id) != devices.end()) {
                    Device& start_device = devices[task.start_device_id];
                    double task_pos = start_device.position_on_track_mm;
                    double dist = distance_on_track(shuttle.position_mm, task_pos);

                    // 如果这个任务更近，更新最佳任务
                    if (dist < min_distance) {
                        min_distance = dist;
                        best_task_idx = i;
                    }
                }
            }
        }

        // 如果找到了合适的任务，分配给穿梭车
        if (best_task_idx != -1) {
            Task& assigned_task = tasks[best_task_idx];
            shuttle.assigned_task_idx = best_task_idx;
            shuttle.agent_state = ShuttleAgentState::MOVING_TO_PICKUP;
            assigned_task.status = TaskStatus::ASSIGNED_TO_SHUTTLE;
            assigned_task.assigned_shuttle_id = shuttle.id;
            assigned_task.is_actively_handled_by_shuttle = true;

            // 记录日志
            std::stringstream ss;
            ss << "穿梭车 S" << shuttle.id << " 被分配任务 #" << assigned_task.id
               << " (" << assigned_task.material_id << "), 从设备 "
               << assigned_task.start_device_id << " 到设备 " << assigned_task.end_device_id;
            add_debug_message(ss.str());

            // 设置目标位置为起点设备位置
            if (devices.find(assigned_task.start_device_id) != devices.end()) {
                Device& start_device = devices[assigned_task.start_device_id];
                shuttle.target_pos_mm = start_device.position_on_track_mm;
                shuttle.has_target_pos = true;
            }
        }
    }
}
