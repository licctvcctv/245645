#include "Device.h"
#include "Task.h"
#include "Simulation.h"
#include "Rendering.h"
#include "Utils.h"  // 添加Utils.h，提供编码转换函数
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm> // 添加algorithm头文件，包含std::min函数

// 全局变量定义
std::map<int, Device> devices_global;
std::map<int, std::deque<int>> pending_task_queues_by_start_device;

// Device构造函数实现
Device::Device() : id(-1), position_on_track_mm(0), op_state(DeviceOperationalState::IDLE_EMPTY),
               current_task_idx_on_device(-1), busy_timer_s(0),
               total_idle_time_s(0), total_has_goods_time_s(0), last_state_change_time_s(0) {}

// 初始化设备
void init_devices() {
    devices_global.clear();
    pending_task_queues_by_start_device.clear();

    // 创建设备
    // 入库口
    Device in_port_13;
    in_port_13.id = 13;
    in_port_13.model_type = DeviceModelType::IN_PORT;
    in_port_13.position_on_track_mm = POS_BOTTOM_STRAIGHT_START + 1000.0;
    in_port_13.name = "入库口13";
    devices_global[in_port_13.id] = in_port_13;

    Device in_port_14;
    in_port_14.id = 14;
    in_port_14.model_type = DeviceModelType::IN_PORT;
    in_port_14.position_on_track_mm = POS_BOTTOM_STRAIGHT_START + 3000.0;
    in_port_14.name = "入库口14";
    devices_global[in_port_14.id] = in_port_14;

    // 出库口
    Device out_port_15;
    out_port_15.id = 15;
    out_port_15.model_type = DeviceModelType::OUT_PORT;
    out_port_15.position_on_track_mm = POS_BOTTOM_STRAIGHT_START + 5000.0;
    out_port_15.name = "出库口15";
    devices_global[out_port_15.id] = out_port_15;

    Device out_port_16;
    out_port_16.id = 16;
    out_port_16.model_type = DeviceModelType::OUT_PORT;
    out_port_16.position_on_track_mm = POS_BOTTOM_STRAIGHT_START + 7000.0;
    out_port_16.name = "出库口16";
    devices_global[out_port_16.id] = out_port_16;

    Device out_port_17;
    out_port_17.id = 17;
    out_port_17.model_type = DeviceModelType::OUT_PORT;
    out_port_17.position_on_track_mm = POS_BOTTOM_STRAIGHT_START + 9000.0;
    out_port_17.name = "出库口17";
    devices_global[out_port_17.id] = out_port_17;

    Device out_port_18;
    out_port_18.id = 18;
    out_port_18.model_type = DeviceModelType::OUT_PORT;
    out_port_18.position_on_track_mm = POS_BOTTOM_STRAIGHT_START + 11000.0;
    out_port_18.name = "出库口18";
    devices_global[out_port_18.id] = out_port_18;

    // 堆垛机
    Device stacker_21;
    stacker_21.id = 21;
    stacker_21.model_type = DeviceModelType::STACKER;
    stacker_21.position_on_track_mm = POS_TOP_STRAIGHT_START + 2000.0;
    stacker_21.name = "堆垛机21";
    devices_global[stacker_21.id] = stacker_21;

    Device stacker_22;
    stacker_22.id = 22;
    stacker_22.model_type = DeviceModelType::STACKER;
    stacker_22.position_on_track_mm = POS_TOP_STRAIGHT_START + 4000.0;
    stacker_22.name = "堆垛机22";
    devices_global[stacker_22.id] = stacker_22;

    Device stacker_23;
    stacker_23.id = 23;
    stacker_23.model_type = DeviceModelType::STACKER;
    stacker_23.position_on_track_mm = POS_TOP_STRAIGHT_START + 6000.0;
    stacker_23.name = "堆垛机23";
    devices_global[stacker_23.id] = stacker_23;

    Device stacker_24;
    stacker_24.id = 24;
    stacker_24.model_type = DeviceModelType::STACKER;
    stacker_24.position_on_track_mm = POS_TOP_STRAIGHT_START + 8000.0;
    stacker_24.name = "堆垛机24";
    devices_global[stacker_24.id] = stacker_24;

    Device stacker_25;
    stacker_25.id = 25;
    stacker_25.model_type = DeviceModelType::STACKER;
    stacker_25.position_on_track_mm = POS_TOP_STRAIGHT_START + 10000.0;
    stacker_25.name = "堆垛机25";
    devices_global[stacker_25.id] = stacker_25;

    // 初始化任务队列
    for (const auto& pair : devices_global) {
        pending_task_queues_by_start_device[pair.first] = std::deque<int>();
    }

    std::cout << "初始化了 " << devices_global.size() << " 个设备" << std::endl;
}

// 更新设备状态
void update_device_state(Device& dev, std::vector<Task>& tasks, std::map<int, std::deque<int>>& pending_task_queues) {
    if (dev.op_state == DeviceOperationalState::BUSY_EMPTY) {
        dev.busy_timer_s -= SIM_TIME_STEP_S;
        if (dev.busy_timer_s <= 0) {
            dev.busy_timer_s = 0;

            bool is_supplying_device = (dev.model_type == DeviceModelType::STACKER || dev.model_type == DeviceModelType::OUT_PORT);

            if (is_supplying_device && pending_task_queues.count(dev.id) && !pending_task_queues[dev.id].empty()) {
                int first_task_idx = pending_task_queues[dev.id].front();

                pending_task_queues[dev.id].pop_front();

                dev.current_task_idx_on_device = first_task_idx;
                tasks[first_task_idx].status = TaskStatus::READY_FOR_PICKUP;
                tasks[first_task_idx].time_placed_on_start_device_s = CURRENT_SIM_TIME_S;

                dev.op_state = DeviceOperationalState::HAS_GOODS_WAITING_FOR_SHUTTLE;
                dev.last_state_change_time_s = CURRENT_SIM_TIME_S;

                log_device_state_change(dev.id, tasks[first_task_idx].material_id, "无货变为有货");
            } else {
                dev.op_state = DeviceOperationalState::IDLE_EMPTY;
                dev.last_state_change_time_s = CURRENT_SIM_TIME_S;
            }
        }
    }
}

// 获取设备状态字符串
std::string get_device_state_str_cn(DeviceOperationalState state) {
    // 注意：这里返回的是UTF-8编码的字符串，在显示时需要转换为GBK
    switch (state) {
        case DeviceOperationalState::IDLE_EMPTY: return "空闲";
        case DeviceOperationalState::BUSY_EMPTY: return "忙碌";
        case DeviceOperationalState::HAS_GOODS_WAITING_FOR_SHUTTLE: return "等待穿梭车";
        case DeviceOperationalState::HAS_GOODS_BEING_ACCESSED_BY_SHUTTLE: return "穿梭车操作中";
        case DeviceOperationalState::HAS_GOODS_WAITING_FOR_CLEARANCE: return "等待清理";
        default: return "未知状态";
    }
}

// 获取设备类型字符串
std::string get_device_type_str_cn(DeviceModelType type) {
    // 注意：这里返回的是UTF-8编码的字符串，在显示时需要转换为GBK
    switch (type) {
        case DeviceModelType::IN_PORT: return "入库口";
        case DeviceModelType::OUT_PORT: return "出库口";
        case DeviceModelType::STACKER: return "堆垛机";
        default: return "未知类型";
    }
}

// 绘制设备 - 已移至Rendering.cpp
// void draw_devices(const std::map<int, Device>& devices_map, const std::vector<Task>& tasks_vec) {
// }
