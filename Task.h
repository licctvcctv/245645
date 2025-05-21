#ifndef TASK_H
#define TASK_H

// 确保Windows版本定义一致
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600

#include <string>
#include <vector>
#include <map>
#include <deque>

// 任务类型枚举
enum class TaskType {
    INBOUND,  // 入库
    OUTBOUND, // 出库
    NONE
};

// 任务状态枚举
enum class TaskStatus {
    PENDING,
    DEVICE_PREPARING,
    READY_FOR_PICKUP,
    ASSIGNED_TO_SHUTTLE,
    SHUTTLE_MOVING_TO_PICKUP,
    SHUTTLE_WAITING_AT_PICKUP,
    SHUTTLE_LOADING,
    SHUTTLE_MOVING_TO_DROPOFF,
    SHUTTLE_WAITING_AT_DROPOFF,
    SHUTTLE_UNLOADING,
    GOODS_AT_DEST_AWAITING_REMOVAL,
    COMPLETED,
    FAILED
};

// 任务结构体
struct Task {
    int id;
    std::string material_id;
    TaskType type;
    int start_device_id;
    int end_device_id;
    TaskStatus status;
    int original_task_list_idx;

    double time_placed_on_start_device_s;
    int assigned_shuttle_id;
    double time_pickup_complete_s;
    double time_dropoff_complete_s;
    double time_goods_taken_from_dest_s;
    bool is_actively_handled_by_shuttle;

    Task();
};

// 任务相关函数声明
void load_tasks_from_markdown_data();
void init_task2_device_initial_tasks();
std::string get_task_type_str(TaskType type);
std::string get_task_status_str(TaskStatus status);
bool all_tasks_completed();

// 全局变量声明
extern std::vector<Task> all_tasks_global;
extern const std::string markdown_task_data;

#endif // TASK_H