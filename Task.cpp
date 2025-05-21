#include "Task.h"
#include "Device.h"
#include "Simulation.h"
#include "Utils.h"  // 添加Utils.h，提供编码转换函数
#include <sstream>
#include <iostream>
#include <stdexcept>

// 全局变量定义
std::vector<Task> all_tasks_global;

// 任务数据
const std::string markdown_task_data = R"(
nan	1	TP001	入库	16	1
nan	2	TP002	入库	16	3
nan	3	TP003	入库	16	5
nan	4	TP004	入库	16	7
nan	5	TP005	入库	16	9
nan	6	TP006	入库	16	11
nan	7	TP007	入库	16	1
nan	8	TP008	入库	16	3
nan	9	TP009	入库	16	5
nan	10	TP010	入库	16	7
nan	11	TP011	入库	16	9
nan	12	TP012	入库	16	11
nan	13	TP013	入库	16	1
nan	14	TP014	入库	16	3
nan	15	TP015	入库	16	5
nan	16	TP016	入库	16	7
nan	17	TP017	入库	16	9
nan	18	TP018	入库	16	11
nan	19	TP019	入库	17	1
nan	20	TP020	入库	17	3
nan	21	TP021	入库	17	5
nan	22	TP022	入库	17	7
nan	23	TP023	入库	17	9
nan	24	TP024	入库	17	11
nan	25	TP025	入库	17	1
nan	26	TP026	入库	17	3
nan	27	TP027	入库	17	5
nan	28	TP028	入库	17	7
nan	29	TP029	入库	17	9
nan	30	TP030	入库	17	11
nan	31	TP031	入库	17	1
nan	32	TP032	入库	17	3
nan	33	TP033	入库	17	5
nan	34	TP034	入库	17	7
nan	35	TP035	入库	17	9
nan	36	TP036	入库	17	11
nan	37	TP037	入库	18	1
nan	38	TP038	入库	18	3
nan	39	TP039	入库	18	5
nan	40	TP040	入库	18	7
nan	41	TP041	入库	18	9
nan	42	TP042	入库	18	11
nan	43	TP043	入库	18	1
nan	44	TP044	入库	18	3
nan	45	TP045	入库	18	5
nan	46	TP046	入库	18	7
nan	47	TP047	入库	18	9
nan	48	TP048	入库	18	11
nan	49	TP049	入库	18	1
nan	50	TP050	入库	18	3
nan	51	TP051	入库	18	5
nan	52	TP052	入库	18	7
nan	53	TP053	入库	18	9
nan	54	TP054	入库	18	11
nan	55	TP055	出库	2	13
nan	56	TP056	出库	2	14
nan	57	TP057	出库	2	15
nan	58	TP058	出库	2	13
nan	59	TP059	出库	2	14
nan	60	TP060	出库	2	15
nan	61	TP061	出库	2	13
nan	62	TP062	出库	2	14
nan	63	TP063	出库	2	15
nan	64	TP064	出库	4	13
nan	65	TP065	出库	4	14
nan	66	TP066	出库	4	15
nan	67	TP067	出库	4	13
nan	68	TP068	出库	4	14
nan	69	TP069	出库	4	15
nan	70	TP070	出库	4	13
nan	71	TP071	出库	4	14
nan	72	TP072	出库	4	15
nan	73	TP073	出库	6	13
nan	74	TP074	出库	6	14
nan	75	TP075	出库	6	15
nan	76	TP076	出库	6	13
nan	77	TP077	出库	6	14
nan	78	TP078	出库	6	15
nan	79	TP079	出库	6	13
nan	80	TP080	出库	6	14
nan	81	TP081	出库	6	15
nan	82	TP082	出库	8	13
nan	83	TP083	出库	8	14
nan	84	TP084	出库	8	15
nan	85	TP085	出库	8	13
nan	86	TP086	出库	8	14
nan	87	TP087	出库	8	15
nan	88	TP088	出库	8	13
nan	89	TP089	出库	8	14
nan	90	TP090	出库	8	15
nan	91	TP091	出库	10	13
nan	92	TP092	出库	10	14
nan	93	TP093	出库	10	15
nan	94	TP094	出库	10	13
nan	95	TP095	出库	10	14
nan	96	TP096	出库	10	15
nan	97	TP097	出库	10	13
nan	98	TP098	出库	10	14
nan	99	TP099	出库	10	15
nan	100	TP100	出库	12	13
nan	101	TP101	出库	12	14
nan	102	TP102	出库	12	15
nan	103	TP103	出库	12	13
nan	104	TP104	出库	12	14
nan	105	TP105	出库	12	15
nan	106	TP106	出库	12	13
nan	107	TP107	出库	12	14
nan	108	TP108	出库	12	15
)";

// Task构造函数实现
Task::Task() : id(-1), type(TaskType::NONE), start_device_id(-1), end_device_id(-1),
             status(TaskStatus::PENDING), original_task_list_idx(-1),
             time_placed_on_start_device_s(0), assigned_shuttle_id(-1),
             time_pickup_complete_s(0), time_dropoff_complete_s(0), time_goods_taken_from_dest_s(0),
             is_actively_handled_by_shuttle(false) {}

// 从Markdown数据加载任务
void load_tasks_from_markdown_data() {
    all_tasks_global.clear();
    pending_task_queues_by_start_device.clear();

    std::stringstream ss(markdown_task_data);
    std::string line;
    int current_task_idx = 0;

    while (std::getline(ss, line)) {
        if (line.empty() || line.find("nan") == std::string::npos || line.find("任务编号") != std::string::npos) {
            continue;
        }

        std::stringstream line_ss(line);
        std::string nan_col, task_id_str, material_id_str, task_type_str, start_dev_str, end_dev_str;

        std::getline(line_ss, nan_col, '\t');
        std::getline(line_ss, task_id_str, '\t');
        std::getline(line_ss, material_id_str, '\t');
        std::getline(line_ss, task_type_str, '\t');
        std::getline(line_ss, start_dev_str, '\t');
        std::getline(line_ss, end_dev_str, '\t');

        if (task_id_str.empty()) continue;

        Task t;
        try {
            t.id = std::stoi(task_id_str);
            t.material_id = material_id_str;
            t.type = (task_type_str == "入库") ? TaskType::INBOUND : TaskType::OUTBOUND;
            t.start_device_id = std::stoi(start_dev_str);
            t.end_device_id = std::stoi(end_dev_str);
            t.status = TaskStatus::PENDING;
            t.original_task_list_idx = current_task_idx;

            all_tasks_global.push_back(t);
            pending_task_queues_by_start_device[t.start_device_id].push_back(current_task_idx);
            current_task_idx++;
        } catch (const std::invalid_argument& ia) {
            std::cerr << "错误: 解析任务数据时发生无效参数: " << ia.what() << " 行: " << line << std::endl;
        } catch (const std::out_of_range& oor) {
            std::cerr << "错误: 解析任务数据时发生越界错误: " << oor.what() << " 行: " << line << std::endl;
        }
    }
}

// 初始化任务2设备初始任务
void init_task2_device_initial_tasks() {
    if (RUN_TASK1_MODE) return;

    for (auto& pair : devices_global) {
        Device& dev = pair.second;
        bool is_supplying_device = (dev.model_type == DeviceModelType::IN_PORT || dev.model_type == DeviceModelType::OUT_PORT);

        if (is_supplying_device && pending_task_queues_by_start_device.count(dev.id) && !pending_task_queues_by_start_device[dev.id].empty()) {
            int first_task_idx = pending_task_queues_by_start_device[dev.id].front();

            pending_task_queues_by_start_device[dev.id].pop_front();

            dev.current_task_idx_on_device = first_task_idx;
            all_tasks_global[first_task_idx].status = TaskStatus::READY_FOR_PICKUP;
            all_tasks_global[first_task_idx].time_placed_on_start_device_s = 0.0;

            dev.op_state = DeviceOperationalState::HAS_GOODS_WAITING_FOR_SHUTTLE;
            dev.last_state_change_time_s = 0.0;

            log_device_state_change(dev.id, all_tasks_global[first_task_idx].material_id, "无货变为有货");
        }
    }
}

// 获取任务类型字符串
std::string get_task_type_str(TaskType type) {
    // 注意：这里返回的是UTF-8编码的字符串，在显示时需要转换为GBK
    return type == TaskType::INBOUND ? "入库" : (type == TaskType::OUTBOUND ? "出库" : "N/A");
}

// 获取任务状态字符串
std::string get_task_status_str(TaskStatus status) {
    // 注意：这里返回的是UTF-8编码的字符串，在显示时需要转换为GBK
    switch (status) {
        case TaskStatus::PENDING: return "待处理";
        case TaskStatus::DEVICE_PREPARING: return "设备准备中";
        case TaskStatus::READY_FOR_PICKUP: return "待取货";
        case TaskStatus::ASSIGNED_TO_SHUTTLE: return "已分配";
        case TaskStatus::SHUTTLE_MOVING_TO_PICKUP: return "穿梭车前往取货";
        case TaskStatus::SHUTTLE_WAITING_AT_PICKUP: return "穿梭车等待取货";
        case TaskStatus::SHUTTLE_LOADING: return "穿梭车装货中";
        case TaskStatus::SHUTTLE_MOVING_TO_DROPOFF: return "穿梭车前往卸货";
        case TaskStatus::SHUTTLE_WAITING_AT_DROPOFF: return "穿梭车等待卸货";
        case TaskStatus::SHUTTLE_UNLOADING: return "穿梭车卸货中";
        case TaskStatus::GOODS_AT_DEST_AWAITING_REMOVAL: return "货物等待清理";
        case TaskStatus::COMPLETED: return "已完成";
        case TaskStatus::FAILED: return "失败";
        default: return "未知状态";
    }
}

// 检查所有任务是否完成
bool all_tasks_completed() {
    if (RUN_TASK1_MODE) return false;
    if (all_tasks_global.empty() && !RUN_TASK1_MODE) return true;
    if (all_tasks_global.empty()) return false;

    for (const auto& task : all_tasks_global) {
        if (task.status != TaskStatus::COMPLETED && task.status != TaskStatus::FAILED) {
            return false;
        }
    }
    return true;
}
