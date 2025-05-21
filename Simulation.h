#ifndef SIMULATION_H
#define SIMULATION_H

// 确保Windows版本定义一致
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600

#include "Shuttle.h"
#include "Device.h"
#include "Task.h"
#include <string>
#include <fstream>

// 仿真参数常量
// 轨道参数
extern const double TOTAL_TRACK_LENGTH;
extern const double POS_TOP_STRAIGHT_START;
extern const double POS_FIRST_CURVE_START;
extern const double POS_FIRST_CURVE_END;
extern const double POS_BOTTOM_STRAIGHT_START;
extern const double POS_SECOND_CURVE_START;
extern const double POS_SECOND_CURVE_END;
extern const double TRACK_STRAIGHT_LENGTH;
extern const double TRACK_CURVE_LENGTH;

// 穿梭车参数
extern const double MAX_SPEED_STRAIGHT_MM_PER_S;
extern const double MAX_SPEED_CURVE_MM_PER_S;
extern const double ACCEL_MM_PER_S2;
extern const double DECEL_MM_PER_S2;
extern const double MIN_INTER_SHUTTLE_DISTANCE_MM;
extern const double LOAD_UNLOAD_TIME_S;
extern const double SHUTTLE_LENGTH_MM;

// 设备操作时间
extern const double STACKER_TO_OUT_INTERFACE_TIME_S;
extern const double STACKER_FROM_IN_INTERFACE_TIME_S;
extern const double OPERATOR_AT_OUT_PORT_TIME_S;
extern const double OPERATOR_AT_IN_PORT_TIME_S;

// 仿真设置
extern double SIM_TIME_STEP_S;
extern double CURRENT_SIM_TIME_S;
extern double SIM_SPEED_MULTIPLIER;
extern bool PAUSED;
extern bool RUN_TASK1_MODE;

// 日志文件
extern std::ofstream debug_log_file;
extern std::ofstream device_state_log_file;
extern std::ofstream task_event_log_file;

// 仿真相关函数声明
void init_simulation();
void run_simulation(int num_shuttles);
void output_task1_summary();
void output_task2_summary_files();
void close_log_files();
void add_debug_message(const std::string& message);
void log_device_state_change(int log_entity_id, const std::string& material_id_str, const std::string& change_description, bool is_shuttle = false);
void log_task_event_completion(const Task& task);
void assign_tasks_to_shuttles(std::vector<Shuttle>& shuttles, std::vector<Task>& tasks, std::map<int, Device>& devices);

// 工具函数
double normalize_track_pos(double pos_mm);
double distance_on_track(double pos1_mm, double pos2_mm);
bool is_on_curve(double pos_mm);
double get_max_speed_at_pos(double pos_mm);
bool is_position_between(double pos, double start, double end);

#endif // SIMULATION_H
