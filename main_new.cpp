// 确保Windows版本定义一致
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600
#define ORANGE RGB(255, 165, 0)

#include <graphics.h> // EasyX - 必须先包含，避免颜色常量冲突
#include "Shuttle.h"
#include "Device.h"
#include "Task.h"
#include "Simulation.h"
#include "Rendering.h"
#include "Utils.h"
#include <conio.h>    // For _kbhit(), _getch()
#include <windows.h>  // For Sleep, GetTickCount, SetConsoleOutputCP
#include <iostream>   // For debugging, std::cin, std::cout
#include <locale.h>   // For setlocale
#include <sstream>    // For stringstream
#include <iomanip>    // For setprecision
#include <fstream>    // For file operations
#include <ctime>      // For time functions
#include <algorithm>  // For std::min, std::max
#include <cmath>      // For fmod

// 日志文件声明（定义在 Simulation.cpp 中）
extern std::ofstream task_exe_log_file;

// 函数声明（定义在 Simulation.cpp 中）
extern void close_log_files();

// --- Main Simulation Loop ---
void run_simulation_loop(int num_shuttles_to_run) {
    try {
        // 记录开始仿真
        add_debug_message("开始初始化仿真环境");

        // 初始化绘图参数
        init_graphics();
        add_debug_message("图形环境初始化完成");

        // 初始化设备和穿梭车
        init_devices();
        add_debug_message("设备初始化完成，共 " + std::to_string(devices_global.size()) + " 个设备");

        init_shuttles(num_shuttles_to_run);
        add_debug_message("穿梭车初始化完成，共 " + std::to_string(shuttles_global.size()) + " 台穿梭车");

        // 初始化任务
        if (!RUN_TASK1_MODE) {
            add_debug_message("开始加载任务2数据");
            load_tasks_from_markdown_data();
            init_task2_device_initial_tasks();
            add_debug_message("任务2数据加载完成，共 " + std::to_string(all_tasks_global.size()) + " 个任务");
        } else {
            all_tasks_global.clear();
            pending_task_queues_by_start_device.clear();
            add_debug_message("任务1模式，无需加载任务数据");
        }

        // 打开日志文件
        std::string suffix = RUN_TASK1_MODE ? "_T1" : ("_T2_" + std::to_string(shuttles_global.size()) + "cars");
        std::string task_exe_filename = "TaskExeLog" + suffix + ".txt";
        std::string device_state_filename = "DeviceStateLog" + suffix + ".txt";

        // 删除之前的日志文件
        DeleteFile(task_exe_filename.c_str());
        DeleteFile(device_state_filename.c_str());
        add_debug_message("已清除旧日志文件");

        // 打开日志文件
        task_exe_log_file.open(task_exe_filename.c_str());
        device_state_log_file.open(device_state_filename.c_str());

        if (task_exe_log_file.is_open()) {
            task_exe_log_file << "任务编号\t物料编号\t任务类型\t起始设备\t目的设备\t起始时间\t穿梭车编号\t取货完成时间\t放货完成时间\t货物取走时间\n";
            add_debug_message("任务执行日志文件已打开: " + task_exe_filename);
        } else {
            std::string error_msg = "错误: 无法打开 " + task_exe_filename;
            std::cerr << error_msg << std::endl;
            add_debug_message(error_msg);
            throw std::runtime_error(error_msg);
        }

        if (device_state_log_file.is_open()) {
            device_state_log_file << "时间\t设备编号\t货物编号\t状态\n";
            add_debug_message("设备状态日志文件已打开: " + device_state_filename);
        } else {
            std::string error_msg = "错误: 无法打开 " + device_state_filename;
            std::cerr << error_msg << std::endl;
            add_debug_message(error_msg);
            throw std::runtime_error(error_msg);
        }

        add_debug_message("仿真环境初始化完成，开始仿真循环");
    }
    catch (const std::exception& e) {
        std::string error_msg = "仿真初始化异常: " + std::string(e.what());
        add_debug_message(error_msg);
        log_exception(error_msg);
        MessageBox(GetHWnd(), error_msg.c_str(), "初始化异常", MB_ICONERROR);
        return;
    }
    catch (...) {
        std::string error_msg = "仿真初始化发生未知异常";
        add_debug_message(error_msg);
        log_exception(error_msg);
        MessageBox(GetHWnd(), error_msg.c_str(), "初始化异常", MB_ICONERROR);
        return;
    }

    // 初始化仿真时间
    CURRENT_SIM_TIME_S = 0.0;
    PAUSED = false;

    // 初始化设备和穿梭车统计数据
    for (auto& pair : devices_global) {
        pair.second.total_idle_time_s = 0;
        pair.second.total_has_goods_time_s = 0;
        pair.second.last_state_change_time_s = 0.0;
    }
    for (auto& shuttle : shuttles_global) {
        shuttle.total_run_time_task1_s = 0;
        shuttle.stop_count_task1 = 0;
        shuttle.movement_log_task1.clear();
        shuttle.in_logged_accel_decel_phase = false;
    }

    // 仿真循环变量
    unsigned long last_frame_time_ms = GetTickCount();
    unsigned long sim_steps_this_frame = 0;
    bool simulation_complete_flag = false;

    // 主仿真循环
    while (true) {
        try {
            unsigned long current_frame_time_ms = GetTickCount();
            unsigned long elapsed_ms_real = current_frame_time_ms - last_frame_time_ms;

            // 计算本帧需要执行的仿真步数
            sim_steps_this_frame = 0;
            if (PAUSED) {
                last_frame_time_ms = current_frame_time_ms;
            } else {
                double target_step_duration_ms_per_sim_step = (SIM_TIME_STEP_S * 1000.0);
                if (SIM_SPEED_MULTIPLIER > 1e-3) {
                    target_step_duration_ms_per_sim_step /= SIM_SPEED_MULTIPLIER;
                } else {
                    target_step_duration_ms_per_sim_step = 0;
                }

                if (target_step_duration_ms_per_sim_step < 1.0) {
                    sim_steps_this_frame = (SIM_SPEED_MULTIPLIER > 50) ? 20 : ((SIM_SPEED_MULTIPLIER > 10) ? 10 : 5);
                    last_frame_time_ms = current_frame_time_ms;
                } else if (elapsed_ms_real >= target_step_duration_ms_per_sim_step) {
                    sim_steps_this_frame = 1;
                    if (elapsed_ms_real > target_step_duration_ms_per_sim_step * 1.2 && target_step_duration_ms_per_sim_step > 0) {
                        sim_steps_this_frame = std::min(20UL, (unsigned long)(elapsed_ms_real / target_step_duration_ms_per_sim_step));
                    }
                    last_frame_time_ms = current_frame_time_ms;
                }
            }

            // 执行仿真步骤
            if (sim_steps_this_frame > 0 && !simulation_complete_flag) {
                for (unsigned long step = 0; step < sim_steps_this_frame; ++step) {
                    try {
                        // 更新设备状态
                        if (!RUN_TASK1_MODE) {
                            for (auto& pair : devices_global) {
                                update_device_state(pair.second, all_tasks_global, pending_task_queues_by_start_device);
                            }
                        }

                        // 调度任务
                        if (!RUN_TASK1_MODE) {
                            // 任务调度函数在 Simulation.cpp 中实现
                            // 这里直接调用任务分配函数
                            assign_tasks_to_shuttles(shuttles_global, all_tasks_global, devices_global);
                        }

                        // 更新穿梭车物理状态和逻辑状态
                        for (int i=0; i < (int)shuttles_global.size(); ++i) {
                            update_shuttle_physics_and_state(shuttles_global[i], i, shuttles_global, devices_global, all_tasks_global);
                        }

                        // 更新仿真时间
                        CURRENT_SIM_TIME_S += SIM_TIME_STEP_S;

                        // 在任务1模式下，每10秒记录一次所有穿梭车的位置信息
                        if (RUN_TASK1_MODE && fmod(CURRENT_SIM_TIME_S, 10.0) < SIM_TIME_STEP_S) {
                            std::stringstream status_ss;
                            status_ss << "=== 穿梭车状态报告 (时间: " << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << "s) ===";
                            add_debug_message(status_ss.str());

                            for (const auto& s : shuttles_global) {
                                std::stringstream shuttle_ss;
                                shuttle_ss << "穿梭车 S" << s.id
                                        << " 位置: " << std::fixed << std::setprecision(2) << s.position_mm << " mm"
                                        << ", 速度: " << std::fixed << std::setprecision(2) << s.speed_mm_s << " mm/s"
                                        << ", 状态: " << get_shuttle_state_str_cn(s.agent_state)
                                        << ", 行驶距离: " << std::fixed << std::setprecision(2) << s.distance_traveled_mm << " mm"
                                        << ", 停车次数: " << s.stop_count_task1
                                        << ", 完成一圈: " << (s.completed_full_circle ? "是" : "否");
                                add_debug_message(shuttle_ss.str());
                            }
                        }

                        // 检查任务2是否完成
                        if (!RUN_TASK1_MODE && all_tasks_completed()) {
                            simulation_complete_flag = true;
                            PAUSED = true;
                            std::string complete_msg = "所有任务已完成 (" + std::to_string(num_shuttles_to_run) + " 台穿梭车)。总耗时: "
                                                    + std::to_string(CURRENT_SIM_TIME_S) + "秒";
                            std::cout << complete_msg << std::endl;
                            add_debug_message(complete_msg);
                            output_task2_summary_files();
                            break;
                        }
                    }
                    catch (const std::exception& e) {
                        std::string error_msg = "仿真步骤异常: " + std::string(e.what()) + " (时间: " + std::to_string(CURRENT_SIM_TIME_S) + "s)";
                        add_debug_message(error_msg);
                        log_exception(error_msg);
                        // 继续执行，不中断仿真
                    }
                    catch (...) {
                        std::string error_msg = "仿真步骤发生未知异常 (时间: " + std::to_string(CURRENT_SIM_TIME_S) + "s)";
                        add_debug_message(error_msg);
                        log_exception(error_msg);
                        // 继续执行，不中断仿真
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::string error_msg = "仿真主循环异常: " + std::string(e.what());
            add_debug_message(error_msg);
            log_exception(error_msg);
            // 继续执行，不中断仿真
        }
        catch (...) {
            std::string error_msg = "仿真主循环发生未知异常";
            add_debug_message(error_msg);
            log_exception(error_msg);
            // 继续执行，不中断仿真
        }

        try {
            // 绘制界面
            BeginBatchDraw();
            // 设置背景颜色为浅灰色，更加舒适
            setbkcolor(RGB(240, 240, 245));
            cleardevice();

            // 绘制背景网格
            setlinecolor(RGB(230, 230, 235));
            setlinestyle(PS_SOLID, 1);
            for (int x = 0; x < SCREEN_WIDTH; x += 50) {
                line(x, UI_INFO_PANE_HEIGHT, x, SCREEN_HEIGHT);
            }
            for (int y = UI_INFO_PANE_HEIGHT; y < SCREEN_HEIGHT; y += 50) {
                line(0, y, SCREEN_WIDTH, y);
            }

            // UI Info Pane background
            setfillcolor(RGB(50, 50, 50));
            solidrectangle(0, 0, SCREEN_WIDTH, UI_INFO_PANE_HEIGHT);
            draw_ui_info(num_shuttles_to_run); // Text color for UI is set within draw_ui_info

            // 绘制轨道、设备和穿梭车
            draw_track();
            // 绘制设备和穿梭车
            draw_devices(devices_global, all_tasks_global);
            draw_shuttles(shuttles_global, all_tasks_global);
            EndBatchDraw();
        }
        catch (const std::exception& e) {
            std::string error_msg = "绘制界面异常: " + std::string(e.what());
            add_debug_message(error_msg);
            log_exception(error_msg);
            // 继续执行，不中断仿真
        }
        catch (...) {
            std::string error_msg = "绘制界面发生未知异常";
            add_debug_message(error_msg);
            log_exception(error_msg);
            // 继续执行，不中断仿真
        }

        try {
            // 处理键盘输入
            if (_kbhit()) {
                char key = _getch();
                if (key == 'q' || key == 'Q') {
                    add_debug_message("用户按下Q键，退出仿真");
                    if (RUN_TASK1_MODE && !shuttles_global.empty()) {
                        for(auto& s : shuttles_global) { if (s.in_logged_accel_decel_phase) log_shuttle_movement_phase_task1(s); }
                        output_task1_summary();
                    } else if (!RUN_TASK1_MODE && simulation_complete_flag && !shuttles_global.empty()) {
                        // Summary already output if auto-paused.
                    }
                    close_log_files();
                    return;
                }
                if (key == 'p' || key == 'P') {
                    PAUSED = !PAUSED;
                    add_debug_message(std::string("用户按下P键，") + (PAUSED ? "暂停" : "继续") + "仿真");
                }
                if (key == '+' || key == '=') {
                    double old_speed = SIM_SPEED_MULTIPLIER;
                    SIM_SPEED_MULTIPLIER = std::min(256.0, SIM_SPEED_MULTIPLIER * 1.5);
                    add_debug_message("用户按下+键，仿真速度从 " + std::to_string(old_speed) + " 增加到 " + std::to_string(SIM_SPEED_MULTIPLIER));
                }
                if (key == '-') {
                    double old_speed = SIM_SPEED_MULTIPLIER;
                    SIM_SPEED_MULTIPLIER = std::max(0.05, SIM_SPEED_MULTIPLIER / 1.5);
                    add_debug_message("用户按下-键，仿真速度从 " + std::to_string(old_speed) + " 减少到 " + std::to_string(SIM_SPEED_MULTIPLIER));
                }

                // 按D键将当前调试信息保存到文件
                if (key == 'd' || key == 'D') {
                    std::string debug_filename = "DebugInfo_" + std::to_string((int)CURRENT_SIM_TIME_S) + "s.txt";
                    std::ofstream debug_file(debug_filename);
                    if (debug_file.is_open()) {
                        debug_file << "=== 调试信息 (仿真时间: " << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << "s) ===" << std::endl;
                        debug_file << "模式: " << (RUN_TASK1_MODE ? "任务1 (巡游)" : "任务2 (调度)") << std::endl;
                        debug_file << "穿梭车数量: " << shuttles_global.size() << std::endl;
                        debug_file << "\n--- 穿梭车状态 ---" << std::endl;

                        for (const auto& shuttle : shuttles_global) {
                            debug_file << "穿梭车 " << shuttle.id << ": "
                                      << get_shuttle_state_str_cn(shuttle.agent_state)
                                      << ", 位置: " << std::fixed << std::setprecision(2) << shuttle.position_mm << "mm";

                            if (shuttle.assigned_task_idx != -1 && shuttle.assigned_task_idx < (int)all_tasks_global.size()) {
                                const Task& t = all_tasks_global[shuttle.assigned_task_idx];
                                debug_file << ", 任务: " << t.id << " (" << t.material_id << ")";
                            }

                            debug_file << ", 有货: " << (shuttle.has_goods() ? "是" : "否") << std::endl;
                        }

                        debug_file << "\n--- 设备状态 ---" << std::endl;
                        for (const auto& pair : devices_global) {
                            const Device& dev = pair.second;
                            debug_file << "设备 " << dev.id << " (" << dev.name << "): ";

                            debug_file << get_device_state_str_cn(dev.op_state);

                            if (dev.current_task_idx_on_device != -1 &&
                                dev.current_task_idx_on_device < (int)all_tasks_global.size()) {
                                const Task& t = all_tasks_global[dev.current_task_idx_on_device];
                                debug_file << ", 任务: " << t.id << " (" << t.material_id << ")";
                            }

                            debug_file << std::endl;
                        }

                        debug_file << "\n--- 最近调试消息 ---" << std::endl;
                        for (const auto& msg : debug_messages) {
                            debug_file << msg << std::endl;
                        }

                        debug_file.close();
                        std::cout << "\n调试信息已保存到 " << debug_filename << std::endl;
                        add_debug_message("调试信息已保存到 " + debug_filename);
                    }
                }
                if (key == 'm' || key == 'M') {
                    add_debug_message("用户按下M键，切换模式");
                    if (RUN_TASK1_MODE && !shuttles_global.empty()) {
                        for(auto& s : shuttles_global) { if (s.in_logged_accel_decel_phase) log_shuttle_movement_phase_task1(s); }
                        output_task1_summary();
                    } else if (!RUN_TASK1_MODE && !shuttles_global.empty()){
                        if (simulation_complete_flag) {
                            // Already outputted.
                        } else {
                            std::cout << "任务2未完成，切换模式时不生成最终总结。" << std::endl;
                            add_debug_message("任务2未完成，切换模式时不生成最终总结");
                        }
                    }
                    RUN_TASK1_MODE = !RUN_TASK1_MODE;
                    close_log_files();
                    return;
                }
            }
        }
        catch (const std::exception& e) {
            std::string error_msg = "处理键盘输入异常: " + std::string(e.what());
            add_debug_message(error_msg);
            log_exception(error_msg);
            // 继续执行，不中断仿真
        }
        catch (...) {
            std::string error_msg = "处理键盘输入发生未知异常";
            add_debug_message(error_msg);
            log_exception(error_msg);
            // 继续执行，不中断仿真
        }

        try {
            // 检查仿真是否完成
            if (simulation_complete_flag && !PAUSED) {
                PAUSED = true;
                add_debug_message("仿真完成，自动暂停");
            }

            // 任务2模式下，3小时超时
            if (!RUN_TASK1_MODE && CURRENT_SIM_TIME_S > 3600 * 3 && !simulation_complete_flag) {
                std::string timeout_msg = "警告: 仿真时间超过3小时，可能存在死锁或任务无法完成。自动暂停。";
                std::cout << timeout_msg << std::endl;
                add_debug_message(timeout_msg);
                PAUSED = true;
                simulation_complete_flag = true;
                output_task2_summary_files();
            }

            // 任务1模式下，检查完成条件
            if (RUN_TASK1_MODE && !simulation_complete_flag) {
                // 检查所有穿梭车是否都完成了一整圈巡游
                bool all_shuttles_completed_circle = true;
                for (const auto& shuttle : shuttles_global) {
                    if (!shuttle.completed_full_circle) {
                        all_shuttles_completed_circle = false;
                        break;
                    }
                }

                // 满足条件：所有穿梭车完成一圈巡游，或者仿真时间超过10分钟
                if (all_shuttles_completed_circle || CURRENT_SIM_TIME_S > 600) {
                    std::stringstream reason;
                    if (all_shuttles_completed_circle) {
                        reason << "所有穿梭车已完成一整圈巡游";
                    } else {
                        reason << "仿真时间已超过10分钟，但部分穿梭车未完成一整圈巡游";
                    }

                    std::string complete_msg = "任务1完成条件已满足（" + reason.str() + "），生成统计报告。";
                    std::cout << complete_msg << std::endl;
                    add_debug_message(complete_msg);

                    // 确保记录最后的加减速阶段
                    for(auto& s : shuttles_global) {
                        if (s.in_logged_accel_decel_phase) log_shuttle_movement_phase_task1(s);
                    }

                    output_task1_summary();
                    PAUSED = true;
                    simulation_complete_flag = true;
                }
            }
        }
        catch (const std::exception& e) {
            std::string error_msg = "检查仿真完成条件异常: " + std::string(e.what());
            add_debug_message(error_msg);
            log_exception(error_msg);
            // 继续执行，不中断仿真
        }
        catch (...) {
            std::string error_msg = "检查仿真完成条件发生未知异常";
            add_debug_message(error_msg);
            log_exception(error_msg);
            // 继续执行，不中断仿真
        }

        // 如果本帧没有执行仿真步骤且未暂停，短暂休眠以减少CPU使用
        if (sim_steps_this_frame == 0 && !PAUSED) {
            Sleep(1);
        }
    }

    // 确保关闭所有日志文件
    try {
        close_log_files();
        add_debug_message("仿真结束，日志文件已关闭");
    }
    catch (...) {
        // 忽略关闭日志文件时的异常
    }
}

// 全局异常处理函数
void log_exception(const std::string& exception_info) {
    // 确保日志文件已打开
    std::ofstream crash_log("crash_log.txt", std::ios::app);
    if (crash_log.is_open()) {
        // 获取当前时间
        time_t now = time(0);
        struct tm* timeinfo = localtime(&now);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

        crash_log << "===== 程序异常 (" << buffer << ") =====" << std::endl;
        crash_log << "异常信息: " << exception_info << std::endl;
        crash_log << "当前仿真时间: " << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << " 秒" << std::endl;
        crash_log << "当前模式: " << (RUN_TASK1_MODE ? "任务1 (巡游)" : "任务2 (调度)") << std::endl;

        // 记录穿梭车状态
        crash_log << "\n--- 穿梭车状态 ---" << std::endl;
        for (const auto& shuttle : shuttles_global) {
            crash_log << "穿梭车 " << shuttle.id << ": "
                     << get_shuttle_state_str_cn(shuttle.agent_state)
                     << ", 位置: " << std::fixed << std::setprecision(2) << shuttle.position_mm << "mm";

            if (shuttle.assigned_task_idx != -1 && shuttle.assigned_task_idx < (int)all_tasks_global.size()) {
                const Task& t = all_tasks_global[shuttle.assigned_task_idx];
                crash_log << ", 任务: " << t.id << " (" << t.material_id << ")";
            }

            crash_log << ", 有货: " << (shuttle.has_goods() ? "是" : "否") << std::endl;
        }

        // 记录设备状态
        crash_log << "\n--- 设备状态 ---" << std::endl;
        for (const auto& pair : devices_global) {
            const Device& dev = pair.second;
            crash_log << "设备 " << dev.id << " (" << dev.name << "): ";
            crash_log << get_device_state_str_cn(dev.op_state);

            if (dev.current_task_idx_on_device != -1 &&
                dev.current_task_idx_on_device < (int)all_tasks_global.size()) {
                const Task& t = all_tasks_global[dev.current_task_idx_on_device];
                crash_log << ", 任务: " << t.id << " (" << t.material_id << ")";
            }
            crash_log << std::endl;
        }

        // 记录最近的调试消息
        crash_log << "\n--- 最近调试消息 ---" << std::endl;
        for (const auto& msg : debug_messages) {
            crash_log << msg << std::endl;
        }

        crash_log << "\n===== 异常日志结束 =====" << std::endl << std::endl;
        crash_log.close();
    }

    // 尝试在控制台输出错误信息
    std::cerr << "程序发生异常: " << exception_info << std::endl;
    std::cerr << "详细信息已记录到 crash_log.txt" << std::endl;
}

int main() {
    try {
        // 设置控制台输出为GBK编码
        SetConsoleOutputCP(936); // 936是GBK编码
        // 设置控制台输入为GBK编码
        SetConsoleCP(936);

        setlocale(LC_ALL, "chs");

        // 删除之前的崩溃日志文件
        DeleteFile("crash_log.txt");

        // 初始化图形窗口
        initgraph(SCREEN_WIDTH, SCREEN_HEIGHT, EW_SHOWCONSOLE);
        if (GetHWnd() == NULL) {
            std::cout << "图形初始化失败! 无法获取窗口句柄。" << std::endl;
            std::cin.get();
            return 1;
        }

        // 创建调试日志文件
        create_debug_log_file();

        // 记录调试信息
        add_debug_message("程序启动");
        add_debug_message("图形窗口初始化成功");
        add_debug_message("调试日志文件创建成功: " + current_log_filename);

        srand(static_cast<unsigned int>(time(NULL)));

        int num_shuttles_for_current_run = 3;
        bool program_running = true;

        while(program_running) {
            try {
                // 显示图形化菜单
                MenuResult menu_result = show_graphical_menu(RUN_TASK1_MODE, num_shuttles_for_current_run);

                switch (menu_result) {
                    case MenuResult::START_SIMULATION: {
                        int shuttles_for_this_specific_run = RUN_TASK1_MODE ? 3 : num_shuttles_for_current_run;

                        // 记录调试信息
                        if (RUN_TASK1_MODE) {
                            add_debug_message("开始任务1 (3台穿梭车)");
                        } else {
                            add_debug_message("开始任务2 (" + std::to_string(shuttles_for_this_specific_run) + "台穿梭车)");
                        }

                        // 运行仿真
                        run_simulation_loop(shuttles_for_this_specific_run);

                        add_debug_message("仿真已结束或已返回主菜单");
                        break;
                    }
                    case MenuResult::SWITCH_MODE:
                        add_debug_message("模式已切换。当前模式: " + std::string(RUN_TASK1_MODE ? "任务1" : "任务2"));
                        break;
                    case MenuResult::CHANGE_SHUTTLES:
                        add_debug_message("穿梭车数量已更改为: " + std::to_string(num_shuttles_for_current_run));
                        break;
                    case MenuResult::EXIT:
                        program_running = false;
                        add_debug_message("用户选择退出程序");
                        break;
                }
            }
            catch (const std::exception& e) {
                log_exception(std::string("标准异常: ") + e.what());
                MessageBox(GetHWnd(), ("程序发生异常: " + std::string(e.what()) + "\n详细信息已记录到crash_log.txt").c_str(), "程序异常", MB_ICONERROR);
            }
            catch (...) {
                log_exception("未知异常");
                MessageBox(GetHWnd(), "程序发生未知异常\n详细信息已记录到crash_log.txt", "程序异常", MB_ICONERROR);
            }
        }

        closegraph();
        std::cout << "程序已退出。" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        log_exception(std::string("主函数标准异常: ") + e.what());
        MessageBox(NULL, ("程序发生严重异常: " + std::string(e.what()) + "\n详细信息已记录到crash_log.txt").c_str(), "严重异常", MB_ICONERROR);
        return 1;
    }
    catch (...) {
        log_exception("主函数未知异常");
        MessageBox(NULL, "程序发生严重未知异常\n详细信息已记录到crash_log.txt", "严重异常", MB_ICONERROR);
        return 1;
    }
}
