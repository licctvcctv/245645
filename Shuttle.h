#ifndef SHUTTLE_H
#define SHUTTLE_H

// 确保Windows版本定义一致
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600

#include <vector>
#include <string>
#include <map>
#include <graphics.h>  // EasyX图形库

// 前向声明
struct Device;
struct Task;

// 穿梭车状态枚举
enum class ShuttleAgentState {
    IDLE_EMPTY,
    MOVING_TO_PICKUP,
    WAITING_FOR_PICKUP_AVAILABILITY,
    LOADING,
    MOVING_TO_DROPOFF,
    WAITING_FOR_DROPOFF_AVAILABILITY,
    UNLOADING,
    PATROLLING_CRUISE,
    PATROLLING_ACCEL,
    PATROLLING_DECEL,
    PATROLLING_STOPPED
};

// 运动记录结构体
struct MovementRecord {
    double start_time_s, end_time_s;
    double start_speed_mm_s, end_speed_mm_s;
    double acceleration_mm_s2;
};

// 穿梭车结构体
struct Shuttle {
    int id;
    double position_mm;
    double speed_mm_s;
    double current_acceleration_mm_s2;
    ShuttleAgentState agent_state;
    int assigned_task_idx;

    double target_pos_mm;
    bool has_target_pos;
    double operation_timer_s;

    double total_run_time_task1_s;
    int stop_count_task1;
    std::vector<MovementRecord> movement_log_task1;
    double current_phase_start_time_s;
    double current_phase_start_speed_mm_s;
    double current_phase_acceleration_mm_s2;
    bool in_logged_accel_decel_phase;

    // 任务一巡游一圈相关变量
    double initial_position_mm;  // 初始位置
    bool completed_full_circle;  // 是否完成一整圈
    double distance_traveled_mm; // 已行驶距离

    // 碰撞检测相关变量
    bool is_stopped_at_station;  // 是否在站点停止
    double stopped_position_mm;  // 停止位置

    COLORREF color;

    Shuttle();

    // 判断穿梭车是否载有货物
    bool has_goods() const;
};

// 穿梭车相关函数声明
void init_shuttles(int num_shuttles);
void update_shuttle_physics_and_state(Shuttle& shuttle, int shuttle_idx, std::vector<Shuttle>& all_shuttles,
                                     std::map<int, Device>& devices, std::vector<Task>& tasks);
void log_shuttle_movement_phase_task1(Shuttle& shuttle);
std::string get_shuttle_state_str_cn(ShuttleAgentState state);

// 全局变量声明
extern std::vector<Shuttle> shuttles_global;

#endif // SHUTTLE_H
