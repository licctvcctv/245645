#ifndef DEVICE_H
#define DEVICE_H

// 确保Windows版本定义一致
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600

#include <map>
#include <string>
#include <vector>
#include <deque>

// 前向声明
struct Task;

// 设备模型类型枚举
enum class DeviceModelType {
    IN_PORT,
    OUT_PORT,
    STACKER,
    UNKNOWN
};

// 设备操作状态枚举
enum class DeviceOperationalState {
    IDLE_EMPTY,
    BUSY_EMPTY,
    HAS_GOODS_WAITING_FOR_SHUTTLE,
    HAS_GOODS_BEING_ACCESSED_BY_SHUTTLE,
    HAS_GOODS_WAITING_FOR_CLEARANCE,
    BUSY_SUPPLYING_NEXT_TASK,
    BUSY_CLEARING_GOODS,
    UNKNOWN
};

// 设备结构体
struct Device {
    int id;
    DeviceModelType model_type;
    double position_on_track_mm;
    DeviceOperationalState op_state;
    int current_task_idx_on_device;
    double busy_timer_s;

    double total_idle_time_s;
    double total_has_goods_time_s;
    double last_state_change_time_s;
    std::string name;

    Device();
};

// 设备相关函数声明
void init_devices();
void update_device_state(Device& dev, std::vector<Task>& tasks, std::map<int, std::deque<int>>& pending_task_queues);
std::string get_device_state_str_cn(DeviceOperationalState state);
std::string get_device_type_str_cn(DeviceModelType type);

// 全局变量声明
extern std::map<int, Device> devices_global;
extern std::map<int, std::deque<int>> pending_task_queues_by_start_device;

#endif // DEVICE_H
