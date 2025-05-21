#define WINVER 0x0601
#define _WIN32_WINNT 0x0601
#define ORANGE RGB(255, 165, 0)
#include <graphics.h> // EasyX
#include <conio.h>    // For _kbhit(), _getch()
#include <vector>
#include <string>
#include <cmath>
#include <windows.h>  // For Sleep, GetTickCount, SetConsoleOutputCP
#include <fstream>    // For logging
#include <iomanip>    // For std::fixed, std::setprecision
#include <sstream>    // For string manipulation
#include <map>        // For mapping IDs to objects
#include <deque>      // For task queues
#include <algorithm>  // For std::min, std::max, std::sort
#include <iostream>   // For debugging, std::cin, std::cout
#include <ctime>      // For srand, time
#include <limits>     // For std::numeric_limits
#include <cstdio>     // For snprintf
#include <locale.h>   // For setlocale

// --- Constants ---
const double PI = 3.141592653589793;

// Track parameters from Figure 2 interpretation (all in mm) - UPDATED
const double TRACK_STRAIGHT_LENGTH = 40000.0; // L_S (from Figure 2)
const double TRACK_CURVE_RADIUS = 2500.0;     // R (from Figure 2, 2500mm radius)
const double TRACK_CURVE_LENGTH = PI * TRACK_CURVE_RADIUS; // L_C

// 调试信息缓冲区
const int MAX_DEBUG_MESSAGES = 20;
std::vector<std::string> debug_messages;

// 全局变量，存储当前日志文件名
std::string current_log_filename;

// 函数声明
void create_debug_log_file();

const double POS_TOP_STRAIGHT_START = 0.0;
const double POS_TOP_STRAIGHT_END = POS_TOP_STRAIGHT_START + TRACK_STRAIGHT_LENGTH;
const double POS_FIRST_CURVE_START = POS_TOP_STRAIGHT_END;
const double POS_FIRST_CURVE_END = POS_FIRST_CURVE_START + TRACK_CURVE_LENGTH;
const double POS_BOTTOM_STRAIGHT_START = POS_FIRST_CURVE_END;
const double POS_BOTTOM_STRAIGHT_END = POS_BOTTOM_STRAIGHT_START + TRACK_STRAIGHT_LENGTH;
const double POS_SECOND_CURVE_START = POS_BOTTOM_STRAIGHT_END;
const double POS_SECOND_CURVE_END = POS_SECOND_CURVE_START + TRACK_CURVE_LENGTH;
const double TOTAL_TRACK_LENGTH = POS_SECOND_CURVE_END;

// Shuttle parameters (converted to mm and s)
const double MAX_SPEED_STRAIGHT_MM_PER_S = 200.0 * 1000.0 / 60.0; // 3333.33 mm/s (increased from 160m/min to 200m/min)
const double MAX_SPEED_CURVE_MM_PER_S = 80.0 * 1000.0 / 60.0;    // 1333.33 mm/s (increased from 40m/min to 80m/min)
const double ACCEL_MM_PER_S2 = 0.8 * 1000.0;                     // 800 mm/s^2 (increased from 0.5m/s? to 0.8m/s?)
const double DECEL_MM_PER_S2 = 0.8 * 1000.0;                     // 800 mm/s^2 (increased from 0.5m/s? to 0.8m/s?)
const double MIN_INTER_SHUTTLE_DISTANCE_MM = 200.0;
const double LOAD_UNLOAD_TIME_S = 7.5;
const double SHUTTLE_LENGTH_MM = 2000.0;

// Device operation times (seconds)
const double STACKER_TO_OUT_INTERFACE_TIME_S = 50.0;
const double STACKER_FROM_IN_INTERFACE_TIME_S = 25.0;
const double OPERATOR_AT_OUT_PORT_TIME_S = 30.0;
const double OPERATOR_AT_IN_PORT_TIME_S = 30.0;

// Simulation settings
double SIM_TIME_STEP_S = 0.05; // 20 Hz simulation update
double CURRENT_SIM_TIME_S = 0.0;
double SIM_SPEED_MULTIPLIER = 10.0; // 默认提高到10倍速
bool PAUSED = false;
bool RUN_TASK1_MODE = false; // Default to Task 2, can be toggled

// Shuttle ID offset for logging in DeviceStateLog.txt to avoid collision with Device IDs
const int SHUTTLE_ID_LOG_OFFSET = 100;


// Drawing constants
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;
const int UI_INFO_PANE_HEIGHT = 200;
const int DRAW_AREA_WIDTH = SCREEN_WIDTH - 20;
const int DRAW_AREA_HEIGHT = SCREEN_HEIGHT - UI_INFO_PANE_HEIGHT - 20;
double DRAW_SCALE = 1.0;
double DRAW_OFFSET_X = 0.0;
double DRAW_OFFSET_Y = 0.0;

// Log files
std::ofstream task_exe_log_file;
std::ofstream device_state_log_file;

// --- Forward Declarations ---
struct Task;
struct Device;
struct Shuttle;
void update_shuttle_physics_and_state(Shuttle& shuttle, int shuttle_idx, std::vector<Shuttle>& all_shuttles, std::map<int, Device>& devices, std::vector<Task>& tasks);
void log_task_event_completion(const Task& task);
void log_device_state_change(int log_entity_id, const std::string& material_id_str, const std::string& change_description, bool is_shuttle = false);

// --- 图形菜单相关结构和函数 ---
// 菜单按钮结构
struct MenuButton {
    int x, y, width, height;
    std::string text;
    char hotkey;
    bool enabled;

    MenuButton(int _x, int _y, int _w, int _h, const std::string& _text, char _hotkey, bool _enabled = true)
        : x(_x), y(_y), width(_w), height(_h), text(_text), hotkey(_hotkey), enabled(_enabled) {}

    bool contains(int mx, int my) const {
        return mx >= x && mx <= x + width && my >= y && my <= y + height;
    }
};

// 绘制菜单按钮
void draw_menu_button(const MenuButton& button, bool hover = false) {
    // 按钮背景
    COLORREF bg_color = button.enabled ?
        (hover ? RGB(80, 120, 170) : RGB(60, 80, 120)) :
        RGB(100, 100, 100);
    setfillcolor(bg_color);
    solidroundrect(button.x, button.y, button.x + button.width, button.y + button.height, 10, 10);

    // 按钮边框
    setlinecolor(RGB(150, 150, 150));
    roundrect(button.x, button.y, button.x + button.width, button.y + button.height, 10, 10);

    // 按钮文本
    settextcolor(WHITE);
    settextstyle(20, 0, "微软雅黑");
    int text_width = textwidth(button.text.c_str());
    int text_height = textheight(button.text.c_str());
    outtextxy(button.x + (button.width - text_width) / 2,
              button.y + (button.height - text_height) / 2,
              button.text.c_str());

    // 热键提示
    std::string hotkey_text = std::string("[") + button.hotkey + "]";
    settextstyle(14, 0, "微软雅黑");
    outtextxy(button.x + button.width - 30, button.y + button.height - 20, hotkey_text.c_str());
}

// 绘制标题
void draw_title(const std::string& title, int x, int y) {
    settextcolor(RGB(255, 255, 200));
    settextstyle(28, 0, "微软雅黑");
    int text_width = textwidth(title.c_str());
    outtextxy(x - text_width / 2, y, title.c_str());
}

// 绘制信息文本
void draw_info_text(const std::string& text, int x, int y, COLORREF color = RGB(220, 220, 220)) {
    settextcolor(color);
    settextstyle(16, 0, "微软雅黑");
    outtextxy(x, y, text.c_str());
}

// 调试日志文件
std::ofstream debug_log_file;

// 添加调试信息到缓冲区和日志文件
void add_debug_message(const std::string& message) {
    // 添加时间戳
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << "[" << CURRENT_SIM_TIME_S << "s] " << message;

    // 添加到缓冲区
    debug_messages.push_back(ss.str());

    // 如果超过最大数量，删除最旧的消息
    if (debug_messages.size() > MAX_DEBUG_MESSAGES) {
        debug_messages.erase(debug_messages.begin());
    }

    // 刷新输出缓冲区
    std::cout.flush();

    // 同时输出到控制台（如果可见）
    std::cout << ss.str() << std::endl;

    // 写入到调试日志文件
    if (debug_log_file.is_open()) {
        debug_log_file << ss.str() << std::endl;
        debug_log_file.flush(); // 确保立即写入文件
    }
}


// --- Data Structures ---
enum class TaskType { INBOUND, OUTBOUND, NONE };
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

    Task() : id(-1), type(TaskType::NONE), start_device_id(-1), end_device_id(-1),
             status(TaskStatus::PENDING), original_task_list_idx(-1),
             time_placed_on_start_device_s(0), assigned_shuttle_id(-1),
             time_pickup_complete_s(0), time_dropoff_complete_s(0), time_goods_taken_from_dest_s(0),
             is_actively_handled_by_shuttle(false) {}
};

enum class DeviceModelType { IN_PORT, OUT_PORT, IN_INTERFACE, OUT_INTERFACE };
enum class DeviceOperationalState {
    IDLE_EMPTY,
    BUSY_SUPPLYING_NEXT_TASK,
    HAS_GOODS_WAITING_FOR_SHUTTLE,
    HAS_GOODS_BEING_ACCESSED_BY_SHUTTLE,
    HAS_GOODS_WAITING_FOR_CLEARANCE,
    BUSY_CLEARING_GOODS
};

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

    Device() : id(-1), position_on_track_mm(0), op_state(DeviceOperationalState::IDLE_EMPTY),
               current_task_idx_on_device(-1), busy_timer_s(0),
               total_idle_time_s(0), total_has_goods_time_s(0), last_state_change_time_s(0) {}
};

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

struct MovementRecord {
    double start_time_s, end_time_s;
    double start_speed_mm_s, end_speed_mm_s;
    double acceleration_mm_s2;
};

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

    Shuttle() : id(-1), position_mm(0), speed_mm_s(0), current_acceleration_mm_s2(0),
                agent_state(ShuttleAgentState::IDLE_EMPTY), assigned_task_idx(-1),
                target_pos_mm(0), has_target_pos(false), operation_timer_s(0),
                total_run_time_task1_s(0), stop_count_task1(0),
                current_phase_start_time_s(0), current_phase_start_speed_mm_s(0),
                current_phase_acceleration_mm_s2(0), in_logged_accel_decel_phase(false),
                initial_position_mm(0), completed_full_circle(false), distance_traveled_mm(0),
                is_stopped_at_station(false), stopped_position_mm(0),
                color(RED) {}

    bool has_goods() const {
        if (RUN_TASK1_MODE) return false;
        return agent_state == ShuttleAgentState::LOADING ||
               agent_state == ShuttleAgentState::MOVING_TO_DROPOFF ||
               agent_state == ShuttleAgentState::WAITING_FOR_DROPOFF_AVAILABILITY ||
               agent_state == ShuttleAgentState::UNLOADING;
    }
};

std::vector<Task> all_tasks_global;
std::map<int, Device> devices_global;
std::vector<Shuttle> shuttles_global;
std::map<int, std::deque<int>> pending_task_queues_by_start_device;


// --- Utility Functions ---
double normalize_track_pos(double pos_mm) {
    pos_mm = fmod(pos_mm, TOTAL_TRACK_LENGTH);
    if (pos_mm < 0) {
        pos_mm += TOTAL_TRACK_LENGTH;
    }
    return pos_mm;
}

double distance_on_track(double pos1_mm, double pos2_mm) {
    pos1_mm = normalize_track_pos(pos1_mm);
    pos2_mm = normalize_track_pos(pos2_mm);
    if (pos2_mm >= pos1_mm) {
        return pos2_mm - pos1_mm;
    } else {
        return (TOTAL_TRACK_LENGTH - pos1_mm) + pos2_mm;
    }
}

bool is_on_curve(double pos_mm) {
    pos_mm = normalize_track_pos(pos_mm);
    return (pos_mm >= POS_FIRST_CURVE_START && pos_mm < POS_FIRST_CURVE_END) ||
           (pos_mm >= POS_SECOND_CURVE_START && pos_mm < POS_SECOND_CURVE_END);
}

double get_max_speed_at_pos(double pos_mm) {
    return is_on_curve(pos_mm) ? MAX_SPEED_CURVE_MM_PER_S : MAX_SPEED_STRAIGHT_MM_PER_S;
}

void map_track_pos_to_screen_xy(double pos_mm, int& screen_x, int& screen_y) {
    pos_mm = normalize_track_pos(pos_mm);
    double world_x, world_y;

    if (pos_mm >= POS_TOP_STRAIGHT_START && pos_mm < POS_TOP_STRAIGHT_END) {
        world_x = (pos_mm - POS_TOP_STRAIGHT_START) - TRACK_STRAIGHT_LENGTH / 2.0;
        world_y = -TRACK_CURVE_RADIUS;
    }
    else if (pos_mm >= POS_FIRST_CURVE_START && pos_mm < POS_FIRST_CURVE_END) {
        double angle_rad = PI * (pos_mm - POS_FIRST_CURVE_START) / TRACK_CURVE_LENGTH;
        world_x = TRACK_STRAIGHT_LENGTH / 2.0 + TRACK_CURVE_RADIUS * sin(angle_rad);
        world_y = -TRACK_CURVE_RADIUS * cos(angle_rad);
    }
    else if (pos_mm >= POS_BOTTOM_STRAIGHT_START && pos_mm < POS_BOTTOM_STRAIGHT_END) {
        world_x = (TRACK_STRAIGHT_LENGTH / 2.0) - (pos_mm - POS_BOTTOM_STRAIGHT_START);
        world_y = TRACK_CURVE_RADIUS;
    }
    else {
        double angle_rad = PI * (pos_mm - POS_SECOND_CURVE_START) / TRACK_CURVE_LENGTH;
        world_x = -TRACK_STRAIGHT_LENGTH / 2.0 - TRACK_CURVE_RADIUS * sin(angle_rad);
        world_y = TRACK_CURVE_RADIUS * cos(angle_rad);
    }

    screen_x = static_cast<int>(DRAW_OFFSET_X + world_x * DRAW_SCALE);
    screen_y = static_cast<int>(DRAW_OFFSET_Y - world_y * DRAW_SCALE);
}

std::string get_task_type_str(TaskType type) {
    return type == TaskType::INBOUND ? "入库" : (type == TaskType::OUTBOUND ? "出库" : "N/A");
}

std::string get_shuttle_state_str_cn(ShuttleAgentState state) {
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

std::string get_device_op_state_str_cn(DeviceOperationalState state) {
    switch (state) {
        case DeviceOperationalState::IDLE_EMPTY: return "空闲无货";
        case DeviceOperationalState::BUSY_SUPPLYING_NEXT_TASK: return "供货中";
        case DeviceOperationalState::HAS_GOODS_WAITING_FOR_SHUTTLE: return "有货(待取)";
        case DeviceOperationalState::HAS_GOODS_BEING_ACCESSED_BY_SHUTTLE: return "小车交互中";
        case DeviceOperationalState::HAS_GOODS_WAITING_FOR_CLEARANCE: return "有货(待清)";
        case DeviceOperationalState::BUSY_CLEARING_GOODS: return "清货中";
        default: return "未知状态";
    }
}


// --- Initialization Functions ---
void init_drawing_parameters() {
    double track_world_width = TRACK_STRAIGHT_LENGTH + 2 * TRACK_CURVE_RADIUS;
    double track_world_height = 2 * TRACK_CURVE_RADIUS;

    DRAW_SCALE = std::min(static_cast<double>(DRAW_AREA_WIDTH) / track_world_width,
                          static_cast<double>(DRAW_AREA_HEIGHT) / track_world_height) * 0.95;

    DRAW_OFFSET_X = (DRAW_AREA_WIDTH / 2.0) + 10;
    DRAW_OFFSET_Y = (DRAW_AREA_HEIGHT / 2.0) + UI_INFO_PANE_HEIGHT + 10;
}


void init_devices() {
    devices_global.clear();
    auto add_dev = [&](int id, DeviceModelType mt, double pos_on_segment, const std::string& name_prefix, bool is_top_straight) {
        Device d;
        d.id = id;
        d.model_type = mt;
        if (is_top_straight) {
            d.position_on_track_mm = normalize_track_pos(POS_TOP_STRAIGHT_START + pos_on_segment);
        } else {
            d.position_on_track_mm = normalize_track_pos(POS_BOTTOM_STRAIGHT_START + pos_on_segment);
        }
        d.op_state = DeviceOperationalState::IDLE_EMPTY;
        d.current_task_idx_on_device = -1;
        d.busy_timer_s = 0.0;
        d.total_idle_time_s = 0.0;
        d.total_has_goods_time_s = 0.0;
        d.last_state_change_time_s = 0.0;
        d.name = name_prefix;
        if (name_prefix == "入库接口" || name_prefix == "出库接口" || name_prefix == "入库口" || name_prefix == "出库口"){
            d.name += std::to_string(id);
        }
        devices_global[id] = d;
    };

    std::cout << "--- Initializing Devices (Positions relative to start of their straight segment) ---" << std::endl;
    std::cout << "Track Straight Length: " << TRACK_STRAIGHT_LENGTH << " mm, Curve Radius: " << TRACK_CURVE_RADIUS << " mm" << std::endl;
    std::cout << "Total Track Length: " << TOTAL_TRACK_LENGTH << " mm" << std::endl;
    std::cout << "Top Straight: " << POS_TOP_STRAIGHT_START << " to " << POS_TOP_STRAIGHT_END << std::endl;
    std::cout << "Bottom Straight: " << POS_BOTTOM_STRAIGHT_START << " to " << POS_BOTTOM_STRAIGHT_END << std::endl;

    // 根据图片布局设置设备位置
    // 设备宽度常量
    const double DEVICE_WIDTH = 1100.0;

    // 下方直线部分 - 自动化仓储区 (设备 1-12)
    // 设备1 - 距离左端3250mm（包含设备中心点）
    double current_pos_bottom = 3250.0;
    add_dev(1, DeviceModelType::IN_INTERFACE, current_pos_bottom, "入库接口", false);

    // 设备2 - 距离设备1中心点1300mm + 设备1宽度的一半 + 设备2宽度的一半
    current_pos_bottom += 1300.0 + DEVICE_WIDTH;
    add_dev(2, DeviceModelType::OUT_INTERFACE, current_pos_bottom, "出库接口", false);

    // 设备3 - 距离设备2中心点2500mm + 设备2宽度的一半 + 设备3宽度的一半
    current_pos_bottom += 2500.0 + DEVICE_WIDTH;
    add_dev(3, DeviceModelType::IN_INTERFACE, current_pos_bottom, "入库接口", false);

    // 设备4 - 距离设备3中心点1300mm + 设备3宽度的一半 + 设备4宽度的一半
    current_pos_bottom += 1300.0 + DEVICE_WIDTH;
    add_dev(4, DeviceModelType::OUT_INTERFACE, current_pos_bottom, "出库接口", false);

    // 设备5 - 距离设备4中心点2500mm + 设备4宽度的一半 + 设备5宽度的一半
    current_pos_bottom += 2500.0 + DEVICE_WIDTH;
    add_dev(5, DeviceModelType::IN_INTERFACE, current_pos_bottom, "入库接口", false);

    // 设备6 - 距离设备5中心点1300mm + 设备5宽度的一半 + 设备6宽度的一半
    current_pos_bottom += 1300.0 + DEVICE_WIDTH;
    add_dev(6, DeviceModelType::OUT_INTERFACE, current_pos_bottom, "出库接口", false);

    // 设备7 - 距离设备6中心点2500mm + 设备6宽度的一半 + 设备7宽度的一半
    current_pos_bottom += 2500.0 + DEVICE_WIDTH;
    add_dev(7, DeviceModelType::IN_INTERFACE, current_pos_bottom, "入库接口", false);

    // 设备8 - 距离设备7中心点1300mm + 设备7宽度的一半 + 设备8宽度的一半
    current_pos_bottom += 1300.0 + DEVICE_WIDTH;
    add_dev(8, DeviceModelType::OUT_INTERFACE, current_pos_bottom, "出库接口", false);

    // 设备9 - 距离设备8中心点2500mm + 设备8宽度的一半 + 设备9宽度的一半
    current_pos_bottom += 2500.0 + DEVICE_WIDTH;
    add_dev(9, DeviceModelType::IN_INTERFACE, current_pos_bottom, "入库接口", false);

    // 设备10 - 距离设备9中心点1300mm + 设备9宽度的一半 + 设备10宽度的一半
    current_pos_bottom += 1300.0 + DEVICE_WIDTH;
    add_dev(10, DeviceModelType::OUT_INTERFACE, current_pos_bottom, "出库接口", false);

    // 设备11 - 距离设备10中心点2500mm + 设备10宽度的一半 + 设备11宽度的一半
    current_pos_bottom += 2500.0 + DEVICE_WIDTH;
    add_dev(11, DeviceModelType::IN_INTERFACE, current_pos_bottom, "入库接口", false);

    // 设备12 - 距离设备11中心点1300mm + 设备11宽度的一半 + 设备12宽度的一半
    current_pos_bottom += 1300.0 + DEVICE_WIDTH;
    add_dev(12, DeviceModelType::OUT_INTERFACE, current_pos_bottom, "出库接口", false);

    // 上方直线部分 - 出入库作业区 (设备 13-18)
    // 设备13 - 距离右端7450mm (从右向左排列)
    double pos_dev13_rel = TRACK_STRAIGHT_LENGTH - 7450.0;
    add_dev(13, DeviceModelType::OUT_PORT, pos_dev13_rel, "出库口", true);

    // 设备14 - 距离设备13中心点1900mm + 设备13宽度的一半 + 设备14宽度的一半
    double pos_dev14_rel = pos_dev13_rel - (1900.0 + DEVICE_WIDTH);
    add_dev(14, DeviceModelType::OUT_PORT, pos_dev14_rel, "出库口", true);

    // 设备15 - 距离设备14中心点1900mm + 设备14宽度的一半 + 设备15宽度的一半
    double pos_dev15_rel = pos_dev14_rel - (1900.0 + DEVICE_WIDTH);
    add_dev(15, DeviceModelType::OUT_PORT, pos_dev15_rel, "出库口", true);

    // 设备16 - 距离设备15中心点10900mm + 设备15宽度的一半 + 设备16宽度的一半
    double pos_dev16_rel = pos_dev15_rel - (10900.0 + DEVICE_WIDTH);
    add_dev(16, DeviceModelType::IN_PORT, pos_dev16_rel, "入库口", true);

    // 设备17 - 距离设备16中心点1900mm + 设备16宽度的一半 + 设备17宽度的一半
    double pos_dev17_rel = pos_dev16_rel - (1900.0 + DEVICE_WIDTH);
    add_dev(17, DeviceModelType::IN_PORT, pos_dev17_rel, "入库口", true);

    // 设备18 - 距离设备17中心点1900mm + 设备17宽度的一半 + 设备18宽度的一半
    double pos_dev18_rel = pos_dev17_rel - (1900.0 + DEVICE_WIDTH);
    add_dev(18, DeviceModelType::IN_PORT, pos_dev18_rel, "入库口", true);

    std::cout << "\n--- Device Absolute Positions on Track ---" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    for(const auto& pair : devices_global){
        const Device& d = pair.second;
        std::cout << "Device ID: " << d.id << "\t Name: " << d.name
                  << "\t Absolute Position (mm): " << d.position_on_track_mm << std::endl;
    }
    std::cout << "----------------------------------------" << std::endl;
}


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

void init_task2_device_initial_tasks() {
    if (RUN_TASK1_MODE) return;

    for (auto& pair : devices_global) {
        Device& dev = pair.second;
        bool is_supplying_device = (dev.model_type == DeviceModelType::IN_PORT || dev.model_type == DeviceModelType::OUT_INTERFACE);

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


// --- Logging ---
void open_log_files() {
    // 设置控制台输出为GBK编码
    SetConsoleOutputCP(936); // 936是GBK编码

    std::string suffix = RUN_TASK1_MODE ? "_T1" : ("_T2_" + std::to_string(shuttles_global.size()) + "cars");
    std::string task_exe_filename = "TaskExeLog" + suffix + ".txt";
    std::string device_state_filename = "DeviceStateLog" + suffix + ".txt";

    // 删除之前的日志文件
    DeleteFile(task_exe_filename.c_str());
    DeleteFile(device_state_filename.c_str());

    // 直接打开任务执行日志文件
    task_exe_log_file.open(task_exe_filename.c_str());

    if (task_exe_log_file.is_open()) {
        // 直接写入标题行
        task_exe_log_file << "任务编号\t物料编号\t任务类型\t起始设备\t目的设备\t起始时间\t穿梭车编号\t取货完成时间\t放货完成时间\t货物取走时间\n";
    } else {
        std::cerr << "错误: 无法打开 " << task_exe_filename << std::endl;
    }

    // 直接打开设备状态日志文件
    device_state_log_file.open(device_state_filename.c_str());

    if (device_state_log_file.is_open()) {
        // 直接写入标题行
        device_state_log_file << "时间\t设备编号\t货物编号\t状态\n";
    } else {
        std::cerr << "错误: 无法打开 " << device_state_filename << std::endl;
    }
}

void close_log_files() {
    if (task_exe_log_file.is_open()) task_exe_log_file.close();
    if (device_state_log_file.is_open()) device_state_log_file.close();
    if (debug_log_file.is_open()) {
        debug_log_file << "=== 调试日志结束 ===" << std::endl;
        debug_log_file.close();
    }
}

void log_task_event_completion(const Task& task) {
    if (!task_exe_log_file.is_open() || RUN_TASK1_MODE) return;
    task_exe_log_file << task.id << "\t"
                      << task.material_id << "\t"
                      << get_task_type_str(task.type) << "\t"
                      << task.start_device_id << "\t"
                      << task.end_device_id << "\t"
                      << std::fixed << std::setprecision(2) << task.time_placed_on_start_device_s << "\t"
                      << (task.assigned_shuttle_id == -1 ? "N/A" : std::to_string(task.assigned_shuttle_id)) << "\t"
                      << std::fixed << std::setprecision(2) << task.time_pickup_complete_s << "\t"
                      << std::fixed << std::setprecision(2) << task.time_dropoff_complete_s << "\t"
                      << std::fixed << std::setprecision(2) << task.time_goods_taken_from_dest_s << "\n";
    task_exe_log_file.flush();
}

void log_device_state_change(int log_entity_id, const std::string& material_id_str, const std::string& change_description, bool is_shuttle) {
    if (!device_state_log_file.is_open() || RUN_TASK1_MODE) return;

    int id_to_log = is_shuttle ? (log_entity_id + SHUTTLE_ID_LOG_OFFSET) : log_entity_id;
    device_state_log_file << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << "\t"
                          << id_to_log << "\t"
                          << (material_id_str.empty() ? "N/A" : material_id_str) << "\t"
                          << change_description << "\n";
    device_state_log_file.flush();
}

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
             << "加速度=" << std::fixed << std::setprecision(2) << rec.acceleration_mm_s2 << "mm/s?";
    add_debug_message(debug_ss.str());

    // 根据要求记录所有加减速数据，但过滤掉微小变化
    // 只有当加速度不为0且速度变化明显时才记录
    if (std::abs(rec.acceleration_mm_s2) > 1e-3 &&
        std::abs(rec.end_speed_mm_s - rec.start_speed_mm_s) > 1.0) {
        shuttle.movement_log_task1.push_back(rec);
    }

    shuttle.in_logged_accel_decel_phase = false;
}


// --- Shuttle Physics and State Machine ---
void update_shuttle_physics_and_state(Shuttle& shuttle, int shuttle_idx, std::vector<Shuttle>& all_shuttles, std::map<int, Device>& devices, std::vector<Task>& tasks) {
    // Handle timed operations (loading, unloading, Task 1 stops, delayed starts)
    if (shuttle.agent_state == ShuttleAgentState::LOADING ||
        shuttle.agent_state == ShuttleAgentState::UNLOADING ||
        (RUN_TASK1_MODE && shuttle.agent_state == ShuttleAgentState::PATROLLING_STOPPED) ||
        (RUN_TASK1_MODE && shuttle.agent_state == ShuttleAgentState::PATROLLING_ACCEL && shuttle.operation_timer_s > 0)) {

        shuttle.operation_timer_s -= SIM_TIME_STEP_S;
        if (shuttle.operation_timer_s <= 0) {
            shuttle.operation_timer_s = 0;

            // 如果是停车状态结束，恢复巡游
            if (RUN_TASK1_MODE && shuttle.agent_state == ShuttleAgentState::PATROLLING_STOPPED) {
                shuttle.agent_state = ShuttleAgentState::PATROLLING_ACCEL;

                // 重置停车标志
                shuttle.is_stopped_at_station = false;

                std::stringstream debug_ss;
                debug_ss << "穿梭车 S" << shuttle.id << " 停车结束，继续巡游";
                add_debug_message(debug_ss.str());
            }

            // 如果是延迟启动的穿梭车，记录启动信息并确保它有一个目标
            if (RUN_TASK1_MODE && shuttle.agent_state == ShuttleAgentState::PATROLLING_ACCEL && shuttle.speed_mm_s < 1e-3) {
                std::stringstream debug_ss;
                debug_ss << "穿梭车 S" << shuttle.id << " 开始启动";
                add_debug_message(debug_ss.str());

                // 如果穿梭车没有目标，给它设置一个远离当前位置的目标
                if (!shuttle.has_target_pos) {
                    // 寻找最远的出入库口
                    double max_distance = 0;
                    int farthest_port_id = -1;

                    for(const auto& pair : devices_global) {
                        if (pair.second.model_type == DeviceModelType::IN_PORT ||
                            pair.second.model_type == DeviceModelType::OUT_PORT) {
                            // 计算与当前位置的距离
                            double dist = distance_on_track(shuttle.position_mm, pair.second.position_on_track_mm);
                            if (dist > max_distance) {
                                max_distance = dist;
                                farthest_port_id = pair.first;
                            }
                        }
                    }

                    // 如果找到了最远的出入库口，将其设为目标
                    if (farthest_port_id != -1) {
                        shuttle.target_pos_mm = devices_global.at(farthest_port_id).position_on_track_mm;
                        shuttle.has_target_pos = true;

                        std::stringstream target_ss;
                        target_ss << "穿梭车 S" << shuttle.id << " 启动时设置目标: 设备 " << farthest_port_id
                                << " (" << (devices_global.at(farthest_port_id).model_type == DeviceModelType::IN_PORT ? "入库口" : "出库口")
                                << ")，距离: " << std::fixed << std::setprecision(2) << max_distance << " mm";
                        add_debug_message(target_ss.str());
                    }
                }
            }

            if (shuttle.agent_state == ShuttleAgentState::LOADING) {
                if (shuttle.assigned_task_idx != -1) {
                    Task& current_task = tasks[shuttle.assigned_task_idx];
                    current_task.time_pickup_complete_s = CURRENT_SIM_TIME_S;
                    log_device_state_change(shuttle.id, current_task.material_id, "无货变为有货", true);

                    Device& start_dev = devices.at(current_task.start_device_id);
                    start_dev.op_state = DeviceOperationalState::IDLE_EMPTY;
                    start_dev.last_state_change_time_s = CURRENT_SIM_TIME_S;
                    log_device_state_change(start_dev.id, current_task.material_id, "有货变为无货");
                    start_dev.current_task_idx_on_device = -1;

                    // 打印取货完成信息
                    std::stringstream debug_ss;
                    debug_ss << "取货完成: ID=" << current_task.id
                            << ", 物料=" << current_task.material_id
                            << ", 穿梭车=" << shuttle.id
                            << ", 设备=" << current_task.start_device_id;
                    add_debug_message(debug_ss.str());

                    shuttle.agent_state = ShuttleAgentState::MOVING_TO_DROPOFF;
                    shuttle.has_target_pos = true;
                    shuttle.target_pos_mm = devices.at(current_task.end_device_id).position_on_track_mm;
                } else {
                    shuttle.agent_state = ShuttleAgentState::IDLE_EMPTY;
                }
            } else if (shuttle.agent_state == ShuttleAgentState::UNLOADING) {
               if (shuttle.assigned_task_idx != -1) {
                    Task& current_task = tasks[shuttle.assigned_task_idx];
                    current_task.time_dropoff_complete_s = CURRENT_SIM_TIME_S;
                    current_task.status = TaskStatus::GOODS_AT_DEST_AWAITING_REMOVAL;
                    log_device_state_change(shuttle.id, current_task.material_id, "有货变为无货", true);

                    Device& end_dev = devices.at(current_task.end_device_id);
                    end_dev.op_state = DeviceOperationalState::HAS_GOODS_WAITING_FOR_CLEARANCE;
                    end_dev.last_state_change_time_s = CURRENT_SIM_TIME_S;
                    end_dev.current_task_idx_on_device = shuttle.assigned_task_idx;
                    log_device_state_change(end_dev.id, current_task.material_id, "无货变为有货");

                    // 打印放货完成信息
                    std::stringstream debug_ss;
                    debug_ss << "放货完成: ID=" << current_task.id
                            << ", 物料=" << current_task.material_id
                            << ", 穿梭车=" << shuttle.id
                            << ", 设备=" << current_task.end_device_id;
                    add_debug_message(debug_ss.str());

                    shuttle.agent_state = ShuttleAgentState::IDLE_EMPTY;
                    shuttle.assigned_task_idx = -1;
                    shuttle.has_target_pos = false;
               } else {
                    shuttle.agent_state = ShuttleAgentState::IDLE_EMPTY;
               }
            } else if (RUN_TASK1_MODE && shuttle.agent_state == ShuttleAgentState::PATROLLING_STOPPED) {
                shuttle.agent_state = ShuttleAgentState::PATROLLING_ACCEL;
                shuttle.has_target_pos = false;
            }
        } else {
            shuttle.speed_mm_s = 0;
            shuttle.current_acceleration_mm_s2 = 0;
            return;
        }
    }

    double desired_speed_mm_s = 0;
    bool needs_to_stop_at_target = false;

    // 如果穿梭车处于延迟启动状态，则不进行加速
    if (RUN_TASK1_MODE && shuttle.agent_state == ShuttleAgentState::PATROLLING_ACCEL && shuttle.operation_timer_s > 0) {
        shuttle.speed_mm_s = 0;
        shuttle.current_acceleration_mm_s2 = 0;
        return;
    }

    if (shuttle.has_target_pos) {
        double dist_to_target = distance_on_track(shuttle.position_mm, shuttle.target_pos_mm);
        double max_speed_here = get_max_speed_at_pos(shuttle.position_mm);
        double stopping_dist_needed = (shuttle.speed_mm_s * shuttle.speed_mm_s) / (2.0 * DECEL_MM_PER_S2) + 5.0;
        double arrival_threshold = std::max(5.0, shuttle.speed_mm_s * SIM_TIME_STEP_S * 1.1);

        if (dist_to_target <= arrival_threshold) {
            shuttle.position_mm = shuttle.target_pos_mm;
            shuttle.speed_mm_s = 0;
            needs_to_stop_at_target = true;

            if (!RUN_TASK1_MODE) {
                if (shuttle.assigned_task_idx < 0 || shuttle.assigned_task_idx >= (int)tasks.size()) {
                    shuttle.agent_state = ShuttleAgentState::IDLE_EMPTY;
                    shuttle.has_target_pos = false;
                } else {
                    Task& current_task = tasks[shuttle.assigned_task_idx];
                    if (shuttle.agent_state == ShuttleAgentState::MOVING_TO_PICKUP ||
                        shuttle.agent_state == ShuttleAgentState::WAITING_FOR_PICKUP_AVAILABILITY) {
                        Device& pick_dev = devices.at(current_task.start_device_id);
                        if (pick_dev.op_state == DeviceOperationalState::HAS_GOODS_WAITING_FOR_SHUTTLE &&
                            pick_dev.current_task_idx_on_device == shuttle.assigned_task_idx) {
                            shuttle.agent_state = ShuttleAgentState::LOADING;
                            shuttle.operation_timer_s = LOAD_UNLOAD_TIME_S;
                            pick_dev.op_state = DeviceOperationalState::HAS_GOODS_BEING_ACCESSED_BY_SHUTTLE;
                            pick_dev.last_state_change_time_s = CURRENT_SIM_TIME_S;
                            shuttle.has_target_pos = false;
                        } else {
                            shuttle.agent_state = ShuttleAgentState::WAITING_FOR_PICKUP_AVAILABILITY;
                        }
                    } else if (shuttle.agent_state == ShuttleAgentState::MOVING_TO_DROPOFF ||
                               shuttle.agent_state == ShuttleAgentState::WAITING_FOR_DROPOFF_AVAILABILITY) {
                        Device& drop_dev = devices.at(current_task.end_device_id);
                        if (drop_dev.op_state == DeviceOperationalState::IDLE_EMPTY) {
                            shuttle.agent_state = ShuttleAgentState::UNLOADING;
                            shuttle.operation_timer_s = LOAD_UNLOAD_TIME_S;
                            drop_dev.op_state = DeviceOperationalState::HAS_GOODS_BEING_ACCESSED_BY_SHUTTLE;
                            drop_dev.last_state_change_time_s = CURRENT_SIM_TIME_S;
                            shuttle.has_target_pos = false;
                        } else {
                            shuttle.agent_state = ShuttleAgentState::WAITING_FOR_DROPOFF_AVAILABILITY;
                        }
                    }
                }
            } else {
               if (shuttle.agent_state == ShuttleAgentState::PATROLLING_DECEL ||
                   shuttle.agent_state == ShuttleAgentState::PATROLLING_ACCEL ||
                   shuttle.agent_state == ShuttleAgentState::PATROLLING_CRUISE) {
                    shuttle.agent_state = ShuttleAgentState::PATROLLING_STOPPED;

                    // 设置停车时间，确保有足够时间记录数据
                    shuttle.operation_timer_s = 5.0; // 固定5秒停车时间
                    shuttle.stop_count_task1++;
                    shuttle.has_target_pos = false;

                    // 设置停车标志，便于其他车辆检测
                    shuttle.is_stopped_at_station = true;
                    shuttle.stopped_position_mm = shuttle.position_mm;

                    // 记录停车信息
                    std::stringstream debug_ss;
                    debug_ss << "穿梭车 S" << shuttle.id << " 在出入库口停车，计划停车 "
                            << std::fixed << std::setprecision(1) << shuttle.operation_timer_s
                            << " 秒，这是第 " << shuttle.stop_count_task1 << " 次停车，位置: "
                            << std::fixed << std::setprecision(2) << shuttle.position_mm << " mm";
                    add_debug_message(debug_ss.str());
                }
            }
        } else if (dist_to_target <= stopping_dist_needed) {
            desired_speed_mm_s = 0;
        } else {
            desired_speed_mm_s = max_speed_here;
        }
    } else {
        if (RUN_TASK1_MODE) {
            if (shuttle.agent_state == ShuttleAgentState::IDLE_EMPTY || shuttle.agent_state == ShuttleAgentState::PATROLLING_ACCEL) {
               shuttle.agent_state = ShuttleAgentState::PATROLLING_ACCEL;
               // 使用随机速度，在最大速度的60%-100%之间
               double max_speed = get_max_speed_at_pos(shuttle.position_mm);
               double speed_factor = 0.6 + (static_cast<double>(rand()) / RAND_MAX) * 0.4; // 60%-100%的随机因子
               desired_speed_mm_s = max_speed * speed_factor;
            } else if (shuttle.agent_state == ShuttleAgentState::PATROLLING_CRUISE) {
                // 使用随机速度，在最大速度的60%-100%之间
                double max_speed = get_max_speed_at_pos(shuttle.position_mm);
                double speed_factor = 0.6 + (static_cast<double>(rand()) / RAND_MAX) * 0.4; // 60%-100%的随机因子
                desired_speed_mm_s = max_speed * speed_factor;

                // 在巡游过程中不随机停车，只在完成一圈后停车
                // 这里不需要任何停车逻辑，因为我们在检测到完成一圈时会直接停车
            } else {
                desired_speed_mm_s = 0;
            }
        } else {
            desired_speed_mm_s = 0;
        }
    }

    // 增强的碰撞检测和避让逻辑
    Shuttle* leader = nullptr;
    double dist_to_leader_rear_gap = TOTAL_TRACK_LENGTH + 1.0;

    // 寻找前方最近的穿梭车
    for (size_t i = 0; i < all_shuttles.size(); ++i) {
        if (all_shuttles[i].id == shuttle.id) continue;

        // 计算前车后部与当前车前部的距离
        double other_shuttle_rear_abs_pos = normalize_track_pos(all_shuttles[i].position_mm - SHUTTLE_LENGTH_MM / 2.0);
        double current_shuttle_front_abs_pos = normalize_track_pos(shuttle.position_mm + SHUTTLE_LENGTH_MM / 2.0);
        double current_gap = distance_on_track(current_shuttle_front_abs_pos, other_shuttle_rear_abs_pos);

        // 找到最近的前车
        if (current_gap < dist_to_leader_rear_gap) {
            dist_to_leader_rear_gap = current_gap;
            leader = &all_shuttles[i];
        }
    }

    // 如果找到了前车，进行避让处理
    if (leader) {
        // 在任务1模式下，使用更严格的跟车距离，防止穿梭车重叠
        if (RUN_TASK1_MODE) {
            // 增加最小安全距离，防止重叠
            // 根据车速动态调整安全距离
            double speed_factor = 1.0 + shuttle.speed_mm_s / 1000.0; // 速度越快，安全距离越大
            double min_safe_distance = MIN_INTER_SHUTTLE_DISTANCE_MM * 3.0 * speed_factor; // 增加最小安全距离
            double critical_distance = MIN_INTER_SHUTTLE_DISTANCE_MM * 1.5; // 临界距离

            // 如果前车停止，使用更大的安全距离
            if (leader->speed_mm_s < 1.0 || leader->agent_state == ShuttleAgentState::PATROLLING_STOPPED) {
                min_safe_distance *= 3.0; // 前车停止时增加200%的安全距离
                critical_distance *= 3.0; // 大幅增加临界距离，确保更早开始减速
            }

            // 记录跟车距离（所有车辆都记录）
            std::stringstream follow_debug_ss;
            follow_debug_ss << "穿梭车 S" << shuttle.id << " 与前车距离: "
                    << std::fixed << std::setprecision(2) << dist_to_leader_rear_gap
                    << " mm，前车ID: S" << leader->id
                    << "，前车状态: " << (leader->agent_state == ShuttleAgentState::PATROLLING_STOPPED ? "停止" : "移动")
                    << "，前车速度: " << std::fixed << std::setprecision(2) << leader->speed_mm_s
                    << " mm/s，前车位置: " << std::fixed << std::setprecision(2) << leader->position_mm
                    << " mm，当前位置: " << std::fixed << std::setprecision(2) << shuttle.position_mm << " mm";
            add_debug_message(follow_debug_ss.str());

            // 根据距离调整速度
            if (dist_to_leader_rear_gap < min_safe_distance) {
                // 在安全距离内，逐渐减速
                double speed_factor = dist_to_leader_rear_gap / min_safe_distance;

                // 如果前车停止，当前车需要更谨慎地减速直至停止
                if (leader->speed_mm_s < 1.0 || leader->agent_state == ShuttleAgentState::PATROLLING_STOPPED) {
                    // 根据距离逐渐减速直至停止
                    if (dist_to_leader_rear_gap < critical_distance * 3.0) {
                        // 在更远距离就开始急剧减速
                        desired_speed_mm_s = 0;

                        // 记录避让停止车辆的信息
                        if (shuttle.speed_mm_s > 10.0) { // 只在速度较高时记录，避免日志过多
                            std::stringstream avoid_ss;
                            avoid_ss << "穿梭车 S" << shuttle.id << " 检测到前方车辆S" << leader->id
                                    << "已停止，距离: " << std::fixed << std::setprecision(2) << dist_to_leader_rear_gap
                                    << " mm，执行紧急减速";
                            add_debug_message(avoid_ss.str());
                        }
                    } else if (dist_to_leader_rear_gap < min_safe_distance) {
                        // 在安全距离内逐渐减速
                        double decel_factor = (dist_to_leader_rear_gap - critical_distance * 3.0) /
                                             (min_safe_distance - critical_distance * 3.0);
                        decel_factor = std::max(0.0, std::min(0.2, decel_factor)); // 限制在0-0.2之间，更保守
                        desired_speed_mm_s = std::min(desired_speed_mm_s,
                                                     get_max_speed_at_pos(shuttle.position_mm) * decel_factor);
                    }
                } else {
                    // 前车移动中，跟随前车速度但保持更大距离
                    desired_speed_mm_s = std::min(desired_speed_mm_s, leader->speed_mm_s * speed_factor * 0.8); // 降低20%速度
                }

                // 在临界距离内，急剧减速
                if (dist_to_leader_rear_gap < critical_distance) {
                    desired_speed_mm_s = std::min(desired_speed_mm_s, std::max(0.0, leader->speed_mm_s - DECEL_MM_PER_S2 * SIM_TIME_STEP_S * 2.0));

                    // 记录接近警告
                    std::stringstream warning_ss;
                    warning_ss << "警告: 穿梭车 S" << shuttle.id << " 与前车距离接近临界值 ("
                            << std::fixed << std::setprecision(2) << dist_to_leader_rear_gap
                            << " mm)，急剧减速";
                    add_debug_message(warning_ss.str());

                    // 在极度接近时停止
                    if (dist_to_leader_rear_gap < critical_distance * 0.5 && shuttle.speed_mm_s > 0) {
                        desired_speed_mm_s = 0;

                        // 记录碰撞风险
                        std::stringstream critical_ss;
                        critical_ss << "严重警告: 穿梭车 S" << shuttle.id << " 与前车距离过近 ("
                                << std::fixed << std::setprecision(2) << dist_to_leader_rear_gap
                                << " mm)，紧急停车";
                        add_debug_message(critical_ss.str());
                    }
                }
            }
        }
        // 任务2模式下使用原有的更严格的跟车逻辑
        else {
            double self_stopping_dist = (shuttle.speed_mm_s * shuttle.speed_mm_s) / (2.0 * DECEL_MM_PER_S2);
            double safe_following_distance = MIN_INTER_SHUTTLE_DISTANCE_MM + self_stopping_dist + leader->speed_mm_s * 0.1;

            if (dist_to_leader_rear_gap < safe_following_distance) {
                if (dist_to_leader_rear_gap < MIN_INTER_SHUTTLE_DISTANCE_MM + 5.0) {
                    desired_speed_mm_s = std::min(desired_speed_mm_s, std::max(0.0, leader->speed_mm_s - DECEL_MM_PER_S2 * SIM_TIME_STEP_S));
                    if (dist_to_leader_rear_gap < 1.0 && shuttle.speed_mm_s > 0) desired_speed_mm_s = 0;
                } else {
                    double speed_factor = dist_to_leader_rear_gap / safe_following_distance;
                    desired_speed_mm_s = std::min(desired_speed_mm_s, leader->speed_mm_s * speed_factor);
                }
            }
        }
    }
    desired_speed_mm_s = std::min(desired_speed_mm_s, get_max_speed_at_pos(shuttle.position_mm));
    if (desired_speed_mm_s < 0) desired_speed_mm_s = 0;


    double old_speed_for_phase_tracking = shuttle.speed_mm_s;

    if (needs_to_stop_at_target && shuttle.speed_mm_s == 0 &&
        (!shuttle.has_target_pos || std::abs(normalize_track_pos(shuttle.position_mm - shuttle.target_pos_mm)) < 1.0) ) {
        shuttle.current_acceleration_mm_s2 = 0;
        shuttle.speed_mm_s = 0;
    } else if (shuttle.speed_mm_s < desired_speed_mm_s - 1e-3) {
        shuttle.current_acceleration_mm_s2 = ACCEL_MM_PER_S2;
        shuttle.speed_mm_s += shuttle.current_acceleration_mm_s2 * SIM_TIME_STEP_S;
        if (shuttle.speed_mm_s > desired_speed_mm_s) shuttle.speed_mm_s = desired_speed_mm_s;
    } else if (shuttle.speed_mm_s > desired_speed_mm_s + 1e-3) {
        shuttle.current_acceleration_mm_s2 = -DECEL_MM_PER_S2;
        shuttle.speed_mm_s += shuttle.current_acceleration_mm_s2 * SIM_TIME_STEP_S;
        if (shuttle.speed_mm_s < desired_speed_mm_s) shuttle.speed_mm_s = desired_speed_mm_s;
        if (shuttle.speed_mm_s < 0) shuttle.speed_mm_s = 0;
    } else {
        shuttle.current_acceleration_mm_s2 = 0;
        shuttle.speed_mm_s = desired_speed_mm_s;
    }

    if (RUN_TASK1_MODE) {
        shuttle.total_run_time_task1_s += SIM_TIME_STEP_S;
        bool actual_accel_is_zero = std::abs(shuttle.current_acceleration_mm_s2) < 1e-3;
        bool logged_phase_accel_was_zero = std::abs(shuttle.current_phase_acceleration_mm_s2) < 1e-3;
        bool accel_sign_changed = (shuttle.current_acceleration_mm_s2 * shuttle.current_phase_acceleration_mm_s2 < -1e-3);
        bool accel_magnitude_changed_significantly = std::abs(shuttle.current_acceleration_mm_s2 - shuttle.current_phase_acceleration_mm_s2) > 1.0;
        bool speed_became_zero_from_moving = shuttle.speed_mm_s < 1e-3 && old_speed_for_phase_tracking > 1e-3;


        if (shuttle.in_logged_accel_decel_phase) {
            if ( (actual_accel_is_zero && !logged_phase_accel_was_zero) ||
                 (speed_became_zero_from_moving && actual_accel_is_zero) ||
                 accel_sign_changed ||
                 (accel_magnitude_changed_significantly && !actual_accel_is_zero && !logged_phase_accel_was_zero && !accel_sign_changed)
               ) {
                log_shuttle_movement_phase_task1(shuttle);
            }
        }

        if (!actual_accel_is_zero && !shuttle.in_logged_accel_decel_phase) {
            shuttle.in_logged_accel_decel_phase = true;
            shuttle.current_phase_start_time_s = (CURRENT_SIM_TIME_S > SIM_TIME_STEP_S) ? CURRENT_SIM_TIME_S - SIM_TIME_STEP_S : 0.0;
            shuttle.current_phase_start_speed_mm_s = old_speed_for_phase_tracking;
            shuttle.current_phase_acceleration_mm_s2 = shuttle.current_acceleration_mm_s2;
        }

        if (shuttle.agent_state != ShuttleAgentState::PATROLLING_STOPPED) {
            if (shuttle.current_acceleration_mm_s2 > 1e-3) shuttle.agent_state = ShuttleAgentState::PATROLLING_ACCEL;
            else if (shuttle.current_acceleration_mm_s2 < -1e-3) shuttle.agent_state = ShuttleAgentState::PATROLLING_DECEL;
            else if (shuttle.speed_mm_s > 1e-3) shuttle.agent_state = ShuttleAgentState::PATROLLING_CRUISE;
        }
    }

    // 计算移动距离
    double distance_moved = shuttle.speed_mm_s * SIM_TIME_STEP_S;

    // 更新位置
    double old_position = shuttle.position_mm;
    shuttle.position_mm += distance_moved;
    shuttle.position_mm = normalize_track_pos(shuttle.position_mm);

    // 累计行驶距离
    shuttle.distance_traveled_mm += distance_moved;

    // 检查是否完成一圈（任务一模式）
    if (RUN_TASK1_MODE && !shuttle.completed_full_circle) {
        // 如果穿梭车经过或到达初始位置，且已经行驶了足够的距离（至少一圈）
        if (shuttle.distance_traveled_mm >= TOTAL_TRACK_LENGTH) {
            // 检查是否经过了初始位置
            bool crossed_initial_pos = false;

            // 如果位置发生了"跳变"（从轨道末端到开始），需要特殊处理
            if (old_position > shuttle.position_mm && old_position > TOTAL_TRACK_LENGTH * 0.9 && shuttle.position_mm < TOTAL_TRACK_LENGTH * 0.1) {
                // 检查初始位置是否在这个跳变区间内
                crossed_initial_pos = (shuttle.initial_position_mm > old_position || shuttle.initial_position_mm < shuttle.position_mm);
            } else {
                // 正常情况：检查是否经过了初始位置
                crossed_initial_pos = (old_position <= shuttle.initial_position_mm && shuttle.position_mm >= shuttle.initial_position_mm) ||
                                     (old_position >= shuttle.initial_position_mm && shuttle.position_mm <= shuttle.initial_position_mm);
            }

            if (crossed_initial_pos) {
                shuttle.completed_full_circle = true;
                std::stringstream debug_ss;
                debug_ss << "穿梭车 S" << shuttle.id << " 已完成一整圈巡游！总行驶距离: "
                        << std::fixed << std::setprecision(2) << shuttle.distance_traveled_mm << " mm"
                        << "，初始位置: " << std::fixed << std::setprecision(2) << shuttle.initial_position_mm << " mm"
                        << "，当前位置: " << std::fixed << std::setprecision(2) << shuttle.position_mm << " mm"
                        << "，停车次数: " << shuttle.stop_count_task1;
                add_debug_message(debug_ss.str());

                // 完成一圈后，立即停车
                if (shuttle.stop_count_task1 == 0) {
                    // 直接在当前位置停车
                    shuttle.agent_state = ShuttleAgentState::PATROLLING_STOPPED;
                    shuttle.operation_timer_s = 999999.0; // 设置一个很大的值，实际上是永久停止
                    shuttle.stop_count_task1++;
                    shuttle.has_target_pos = false;
                    shuttle.is_stopped_at_station = true;
                    shuttle.stopped_position_mm = shuttle.position_mm;

                    std::stringstream stop_ss;
                    stop_ss << "穿梭车 S" << shuttle.id << " 已完成一圈巡游，停止运行。总行驶距离: "
                           << std::fixed << std::setprecision(2) << shuttle.distance_traveled_mm << " mm";
                    add_debug_message(stop_ss.str());

                    // 记录停车信息到日志
                    std::stringstream log_ss;
                    log_ss << "穿梭车 S" << shuttle.id << " 完成任务，停止运行。"
                          << "总运行时间: " << std::fixed << std::setprecision(2) << shuttle.total_run_time_task1_s << " 秒, "
                          << "总行驶距离: " << std::fixed << std::setprecision(2) << shuttle.distance_traveled_mm / 1000.0 << " m";
                    add_debug_message(log_ss.str());
                }

                // 检查所有穿梭车是否都完成了一圈
                bool all_completed = true;
                for (const auto& s : shuttles_global) {
                    if (!s.completed_full_circle) {
                        all_completed = false;
                        break;
                    }
                }

                if (all_completed) {
                    add_debug_message("所有穿梭车已完成一整圈巡游！");
                }
            }
        }
    }
}


// --- Device Logic ---
void update_device_state(Device& device, std::vector<Task>& tasks, std::map<int, std::deque<int>>& task_queues) {
    double time_in_prev_state = CURRENT_SIM_TIME_S - device.last_state_change_time_s;
    if (device.last_state_change_time_s < CURRENT_SIM_TIME_S - 1e-4) {
        if (device.op_state == DeviceOperationalState::IDLE_EMPTY) {
            device.total_idle_time_s += time_in_prev_state;
        }
        if (device.op_state == DeviceOperationalState::HAS_GOODS_WAITING_FOR_SHUTTLE ||
            device.op_state == DeviceOperationalState::HAS_GOODS_BEING_ACCESSED_BY_SHUTTLE ||
            device.op_state == DeviceOperationalState::HAS_GOODS_WAITING_FOR_CLEARANCE) {
            device.total_has_goods_time_s += time_in_prev_state;
        }
    }
    device.last_state_change_time_s = CURRENT_SIM_TIME_S;


    if (device.busy_timer_s > 0) {
        device.busy_timer_s -= SIM_TIME_STEP_S;
        if (device.busy_timer_s <= 0) {
            device.busy_timer_s = 0;
            if (device.op_state == DeviceOperationalState::BUSY_SUPPLYING_NEXT_TASK) {
                if (device.current_task_idx_on_device != -1 && device.current_task_idx_on_device < (int)tasks.size()) {
                    if (tasks[device.current_task_idx_on_device].status == TaskStatus::DEVICE_PREPARING) {
                        int task_id = tasks[device.current_task_idx_on_device].id;
                        std::string material_id = tasks[device.current_task_idx_on_device].material_id;

                        tasks[device.current_task_idx_on_device].status = TaskStatus::READY_FOR_PICKUP;
                        tasks[device.current_task_idx_on_device].time_placed_on_start_device_s = CURRENT_SIM_TIME_S;
                        device.op_state = DeviceOperationalState::HAS_GOODS_WAITING_FOR_SHUTTLE;
                        log_device_state_change(device.id, material_id, "无货变为有货");

                        // 打印调试信息
                        std::stringstream debug_ss;
                        debug_ss << "任务准备完成: ID=" << task_id
                                << ", 物料=" << material_id
                                << ", 设备=" << device.id;
                        add_debug_message(debug_ss.str());
                    } else {
                        device.op_state = DeviceOperationalState::IDLE_EMPTY;
                        device.current_task_idx_on_device = -1;
                    }
                } else {
                    device.op_state = DeviceOperationalState::IDLE_EMPTY;
                    log_device_state_change(device.id, "N/A", "供货错误(无有效任务索引)转空闲");
                    device.current_task_idx_on_device = -1;
                }
            } else if (device.op_state == DeviceOperationalState::BUSY_CLEARING_GOODS) {
                if (device.current_task_idx_on_device != -1 && device.current_task_idx_on_device < (int)tasks.size()) {
                    if (tasks[device.current_task_idx_on_device].status == TaskStatus::GOODS_AT_DEST_AWAITING_REMOVAL) {
                        // 标记任务为完成状态
                        int task_id = tasks[device.current_task_idx_on_device].id;
                        std::string material_id = tasks[device.current_task_idx_on_device].material_id;

                        tasks[device.current_task_idx_on_device].status = TaskStatus::COMPLETED;
                        tasks[device.current_task_idx_on_device].time_goods_taken_from_dest_s = CURRENT_SIM_TIME_S;
                        log_task_event_completion(tasks[device.current_task_idx_on_device]);

                        device.op_state = DeviceOperationalState::IDLE_EMPTY;
                        log_device_state_change(device.id, material_id, "有货变为无货");

                        // 打印调试信息
                        std::stringstream debug_ss;
                        debug_ss << "任务完成: ID=" << task_id
                                << ", 物料=" << material_id
                                << ", 设备=" << device.id;
                        add_debug_message(debug_ss.str());

                        device.current_task_idx_on_device = -1;
                    } else if (tasks[device.current_task_idx_on_device].status == TaskStatus::COMPLETED) {
                        // 任务已经完成，避免重复处理
                        std::stringstream debug_ss;
                        debug_ss << "警告: 任务 " << device.current_task_idx_on_device << " 已经被标记为完成，避免重复处理";
                        add_debug_message(debug_ss.str());
                        device.op_state = DeviceOperationalState::IDLE_EMPTY;
                        device.current_task_idx_on_device = -1;
                    } else {
                        device.op_state = DeviceOperationalState::IDLE_EMPTY;
                    }
                } else {
                    device.op_state = DeviceOperationalState::IDLE_EMPTY;
                    log_device_state_change(device.id, "N/A", "清货错误(无有效任务索引)转空闲");
                    device.current_task_idx_on_device = -1;
                }
            }
        }
    } else {
        if (device.op_state == DeviceOperationalState::IDLE_EMPTY) {
            if ((device.model_type == DeviceModelType::IN_PORT || device.model_type == DeviceModelType::OUT_INTERFACE) &&
                task_queues.count(device.id) && !task_queues[device.id].empty()) {

                int next_task_idx_in_queue = task_queues[device.id].front();

                if (next_task_idx_in_queue >=0 && next_task_idx_in_queue < (int)tasks.size() &&
                    device.current_task_idx_on_device == -1) {

                    Task& task_to_prepare = tasks[next_task_idx_in_queue];
                    if (task_to_prepare.status == TaskStatus::PENDING &&
                        task_to_prepare.assigned_shuttle_id == -1) {

                        task_queues[device.id].pop_front();
                        device.current_task_idx_on_device = next_task_idx_in_queue;

                        task_to_prepare.status = TaskStatus::DEVICE_PREPARING;
                        device.op_state = DeviceOperationalState::BUSY_SUPPLYING_NEXT_TASK;
                        if (device.model_type == DeviceModelType::IN_PORT) {
                            device.busy_timer_s = OPERATOR_AT_IN_PORT_TIME_S;
                        } else {
                            device.busy_timer_s = STACKER_TO_OUT_INTERFACE_TIME_S;
                        }
                    } else if (task_to_prepare.status == TaskStatus::COMPLETED) {
                        // 任务已完成，从队列中移除
                        std::stringstream debug_ss;
                        debug_ss << "任务 " << task_to_prepare.id << " 已完成，从队列中移除";
                        add_debug_message(debug_ss.str());
                        task_queues[device.id].pop_front();
                    }
                }
            }
        } else if (device.op_state == DeviceOperationalState::HAS_GOODS_WAITING_FOR_CLEARANCE) {
            if (device.model_type == DeviceModelType::IN_INTERFACE || device.model_type == DeviceModelType::OUT_PORT) {
                if (device.current_task_idx_on_device != -1 && device.current_task_idx_on_device < (int)tasks.size() &&
                    tasks[device.current_task_idx_on_device].status == TaskStatus::GOODS_AT_DEST_AWAITING_REMOVAL) {

                    device.op_state = DeviceOperationalState::BUSY_CLEARING_GOODS;
                    if (device.model_type == DeviceModelType::IN_INTERFACE) {
                        device.busy_timer_s = STACKER_FROM_IN_INTERFACE_TIME_S;
                    } else {
                        device.busy_timer_s = OPERATOR_AT_OUT_PORT_TIME_S;
                    }
                }
            }
        }
    }
}


// --- Task Scheduling Logic (Simple Greedy for Task 2) ---
void schedule_tasks(std::vector<Shuttle>& shuttles, std::vector<Task>& tasks, std::map<int, Device>& devices) {
    if (RUN_TASK1_MODE) return;

    for (size_t i = 0; i < shuttles.size(); ++i) {
        Shuttle& shuttle = shuttles[i];
        if (shuttle.agent_state == ShuttleAgentState::IDLE_EMPTY) {
            int best_task_idx = -1;
            double min_dist_to_pickup = TOTAL_TRACK_LENGTH * 2.0;

            // 添加调试信息，记录空闲穿梭车的状态
            std::stringstream idle_debug_ss;
            idle_debug_ss << "空闲穿梭车: S" << shuttle.id
                         << ", 位置=" << std::fixed << std::setprecision(2) << shuttle.position_mm
                         << "mm, 开始寻找任务";
            add_debug_message(idle_debug_ss.str());

            for (int task_idx = 0; task_idx < (int)tasks.size(); ++task_idx) {
                Task& task = tasks[task_idx];
                // 只考虑状态为READY_FOR_PICKUP的任务
                if (task.status == TaskStatus::READY_FOR_PICKUP) {
                    if (!devices.count(task.start_device_id)) {
                        continue;
                    }
                    Device& start_dev = devices.at(task.start_device_id);
                    if (start_dev.op_state == DeviceOperationalState::HAS_GOODS_WAITING_FOR_SHUTTLE &&
                        start_dev.current_task_idx_on_device == task_idx) {

                        bool already_targeted_by_other = false;
                        for(const auto& other_shuttle : shuttles) {
                            if (other_shuttle.id != shuttle.id && other_shuttle.assigned_task_idx == task_idx &&
                                (other_shuttle.agent_state == ShuttleAgentState::MOVING_TO_PICKUP ||
                                 other_shuttle.agent_state == ShuttleAgentState::WAITING_FOR_PICKUP_AVAILABILITY ||
                                 other_shuttle.agent_state == ShuttleAgentState::LOADING )) {
                                    already_targeted_by_other = true;
                                    break;
                                }
                        }
                        if (already_targeted_by_other) continue;

                        double dist = distance_on_track(shuttle.position_mm, start_dev.position_on_track_mm);

                        // 添加调试信息，记录找到的可用任务
                        std::stringstream task_debug_ss;
                        task_debug_ss << "找到可用任务: ID=" << task.id
                                     << ", 物料=" << task.material_id
                                     << ", 设备=" << task.start_device_id
                                     << ", 距离=" << std::fixed << std::setprecision(2) << dist << "mm";
                        add_debug_message(task_debug_ss.str());

                        if (dist < min_dist_to_pickup) {
                            min_dist_to_pickup = dist;
                            best_task_idx = task_idx;
                        }
                    }
                }
            }

            if (best_task_idx != -1) {
                shuttle.assigned_task_idx = best_task_idx;
                Task& assigned_task = tasks[best_task_idx];
                assigned_task.status = TaskStatus::ASSIGNED_TO_SHUTTLE;
                assigned_task.assigned_shuttle_id = shuttle.id;
                assigned_task.is_actively_handled_by_shuttle = true;

                shuttle.agent_state = ShuttleAgentState::MOVING_TO_PICKUP;
                shuttle.target_pos_mm = devices.at(assigned_task.start_device_id).position_on_track_mm;
                shuttle.has_target_pos = true;

                // 打印任务分配信息
                std::stringstream debug_ss;
                debug_ss << "任务分配: ID=" << assigned_task.id
                        << ", 物料=" << assigned_task.material_id
                        << ", 穿梭车=" << shuttle.id
                        << ", 起始设备=" << assigned_task.start_device_id
                        << ", 目标设备=" << assigned_task.end_device_id;
                add_debug_message(debug_ss.str());
            } else {
                // 如果没有找到任务，添加调试信息
                std::stringstream no_task_debug_ss;
                no_task_debug_ss << "穿梭车 S" << shuttle.id << " 没有找到可用任务，保持空闲状态";
                add_debug_message(no_task_debug_ss.str());

                // 为了防止穿梭车卡住，让所有空闲穿梭车随机移动到一个设备位置
                if (!shuttle.has_target_pos) {  // 处理所有空闲穿梭车
                    std::vector<int> device_ids;
                    for (const auto& pair : devices) {
                        device_ids.push_back(pair.first);
                    }

                    if (!device_ids.empty()) {
                        int random_device_id = device_ids[rand() % device_ids.size()];
                        shuttle.target_pos_mm = devices.at(random_device_id).position_on_track_mm;
                        shuttle.has_target_pos = true;

                        std::stringstream move_debug_ss;
                        move_debug_ss << "空闲穿梭车 S" << shuttle.id << " 移动到设备 " << random_device_id
                                     << " 位置，防止卡住";
                        add_debug_message(move_debug_ss.str());
                    }
                }
            }
        }
    }
}


// --- Drawing Functions ---
void draw_track() {
    // 定义轨道的颜色和样式
    const COLORREF TRACK_COLOR = RGB(120, 120, 120);       // 轨道主体颜色
    const COLORREF TRACK_HIGHLIGHT = RGB(180, 180, 180);   // 轨道高亮颜色
    const COLORREF TRACK_SHADOW = RGB(80, 80, 80);         // 轨道阴影颜色
    const COLORREF TRACK_MARKER = RGB(200, 200, 200);      // 轨道标记颜色

    // 绘制轨道阴影
    setlinecolor(TRACK_SHADOW);
    setlinestyle(PS_SOLID, 5);

    int prev_x, prev_y;
    map_track_pos_to_screen_xy(0, prev_x, prev_y);

    for (double pos_mm = 100; pos_mm <= TOTAL_TRACK_LENGTH; pos_mm += 100) {
        int curr_x, curr_y;
        map_track_pos_to_screen_xy(pos_mm, curr_x, curr_y);
        line(prev_x + 2, prev_y + 2, curr_x + 2, curr_y + 2);
        prev_x = curr_x;
        prev_y = curr_y;
    }

    int first_x, first_y;
    map_track_pos_to_screen_xy(0, first_x, first_y);
    line(prev_x + 2, prev_y + 2, first_x + 2, first_y + 2);

    // 绘制轨道主体
    setlinecolor(TRACK_COLOR);
    setlinestyle(PS_SOLID, 4);

    map_track_pos_to_screen_xy(0, prev_x, prev_y);

    for (double pos_mm = 100; pos_mm <= TOTAL_TRACK_LENGTH; pos_mm += 100) {
        int curr_x, curr_y;
        map_track_pos_to_screen_xy(pos_mm, curr_x, curr_y);
        line(prev_x, prev_y, curr_x, curr_y);
        prev_x = curr_x;
        prev_y = curr_y;
    }

    map_track_pos_to_screen_xy(0, first_x, first_y);
    line(prev_x, prev_y, first_x, first_y);

    // 绘制轨道高亮
    setlinecolor(TRACK_HIGHLIGHT);
    setlinestyle(PS_SOLID, 1);

    map_track_pos_to_screen_xy(0, prev_x, prev_y);

    for (double pos_mm = 100; pos_mm <= TOTAL_TRACK_LENGTH; pos_mm += 100) {
        int curr_x, curr_y;
        map_track_pos_to_screen_xy(pos_mm, curr_x, curr_y);
        line(prev_x - 1, prev_y - 1, curr_x - 1, curr_y - 1);
        prev_x = curr_x;
        prev_y = curr_y;
    }

    map_track_pos_to_screen_xy(0, first_x, first_y);
    line(prev_x - 1, prev_y - 1, first_x - 1, first_y - 1);

    // 绘制轨道标记点
    setfillcolor(TRACK_MARKER);

    // 在直线段和弯曲段的连接处绘制标记点
    double straight_end = TRACK_STRAIGHT_LENGTH;
    double curve_end = TRACK_STRAIGHT_LENGTH + TRACK_CURVE_LENGTH;
    double straight2_end = TRACK_STRAIGHT_LENGTH * 2 + TRACK_CURVE_LENGTH;
    double curve2_end = TRACK_STRAIGHT_LENGTH * 2 + TRACK_CURVE_LENGTH * 2;

    int mark_x, mark_y;

    // 标记直线段和弯曲段的连接处
    map_track_pos_to_screen_xy(0, mark_x, mark_y);
    solidcircle(mark_x, mark_y, 3);

    map_track_pos_to_screen_xy(straight_end, mark_x, mark_y);
    solidcircle(mark_x, mark_y, 3);

    map_track_pos_to_screen_xy(curve_end, mark_x, mark_y);
    solidcircle(mark_x, mark_y, 3);

    map_track_pos_to_screen_xy(straight2_end, mark_x, mark_y);
    solidcircle(mark_x, mark_y, 3);
}

void draw_devices(const std::map<int, Device>& devices_map, const std::vector<Task>& tasks_vec) {
    // 定义设备显示的颜色和样式
    const COLORREF DEV_BORDER_COLOR = RGB(100, 100, 100);  // 设备边框颜色
    const COLORREF DEV_IDLE_COLOR = RGB(80, 80, 80);        // 空闲设备颜色
    const COLORREF DEV_WAITING_COLOR = RGB(220, 180, 50);   // 等待状态颜色
    const COLORREF DEV_BUSY_COLOR = RGB(70, 130, 180);      // 忙碌状态颜色
    const COLORREF DEV_ACTIVE_COLOR = RGB(220, 120, 50);    // 活动状态颜色
    const COLORREF DEV_TEXT_DARK = RGB(30, 30, 30);         // 深色文本
    const COLORREF DEV_TEXT_LIGHT = RGB(240, 240, 240);     // 浅色文本
    const COLORREF DEV_STATUS_PENDING = RGB(180, 180, 180); // 待处理状态
    const COLORREF DEV_STATUS_READY = RGB(100, 200, 100);   // 就绪状态
    const COLORREF DEV_STATUS_ASSIGNED = RGB(100, 150, 250);// 已分配状态
    const COLORREF DEV_STATUS_COMPLETED = RGB(50, 200, 120);// 已完成状态

    for (const auto& pair : devices_map) {
        const Device& dev = pair.second;
        int sx, sy;
        map_track_pos_to_screen_xy(dev.position_on_track_mm, sx, sy);

        COLORREF device_color = DEV_IDLE_COLOR;
        COLORREF text_color = DEV_TEXT_LIGHT;
        bool has_goods_visual = false;
        int device_width = 18;
        int device_height = 10;

        // 根据设备状态设置颜色
        switch(dev.op_state) {
            case DeviceOperationalState::IDLE_EMPTY:
                device_color = DEV_IDLE_COLOR;
                break;
            case DeviceOperationalState::HAS_GOODS_WAITING_FOR_SHUTTLE:
            case DeviceOperationalState::HAS_GOODS_WAITING_FOR_CLEARANCE:
                device_color = DEV_WAITING_COLOR;
                has_goods_visual = true;
                break;
            case DeviceOperationalState::HAS_GOODS_BEING_ACCESSED_BY_SHUTTLE:
                device_color = DEV_ACTIVE_COLOR;
                has_goods_visual = true;
                break;
            case DeviceOperationalState::BUSY_SUPPLYING_NEXT_TASK:
            case DeviceOperationalState::BUSY_CLEARING_GOODS:
                device_color = DEV_BUSY_COLOR;
                if (dev.op_state == DeviceOperationalState::BUSY_CLEARING_GOODS ||
                   (dev.op_state == DeviceOperationalState::BUSY_SUPPLYING_NEXT_TASK &&
                    dev.current_task_idx_on_device != -1 &&
                    dev.current_task_idx_on_device < tasks_vec.size() &&
                    tasks_vec[dev.current_task_idx_on_device].status != TaskStatus::DEVICE_PREPARING)) {
                    has_goods_visual = true;
                }
                break;
            default:
                device_color = WHITE;
        }

        // 绘制设备底色
        setfillcolor(device_color);
        solidrectangle(sx - device_width, sy - device_height, sx + device_width, sy + device_height);

        // 绘制设备边框
        setlinecolor(DEV_BORDER_COLOR);
        rectangle(sx - device_width, sy - device_height, sx + device_width, sy + device_height);

        // 绘制设备ID
        settextcolor(text_color);
        setbkmode(TRANSPARENT);
        settextstyle(12, 0, "微软雅黑");

        char dev_id_str[5];
        snprintf(dev_id_str, sizeof(dev_id_str), "%d", dev.id);
        int text_width = textwidth(dev_id_str);
        outtextxy(sx - text_width/2, sy - 6, dev_id_str);

        // 如果设备有货物，显示货物信息
        if (has_goods_visual && dev.current_task_idx_on_device != -1 &&
            dev.current_task_idx_on_device < (int)tasks_vec.size()) {

            // 绘制信息背景框
            setfillcolor(RGB(240, 240, 240));
            solidrectangle(sx - 25, sy + device_height + 2, sx + 25, sy + device_height + 32);
            setlinecolor(RGB(150, 150, 150));
            rectangle(sx - 25, sy + device_height + 2, sx + 25, sy + device_height + 32);

            // 显示物料编号
            settextcolor(DEV_TEXT_DARK);
            settextstyle(10, 0, "微软雅黑");
            std::string material_info = tasks_vec[dev.current_task_idx_on_device].material_id;
            int material_width = textwidth(material_info.c_str());
            outtextxy(sx - material_width/2, sy + device_height + 4, material_info.c_str());

            // 显示任务状态
            std::string status_info;
            COLORREF status_color;

            switch(tasks_vec[dev.current_task_idx_on_device].status) {
                case TaskStatus::PENDING:
                    status_info = "待处理";
                    status_color = DEV_STATUS_PENDING;
                    break;
                case TaskStatus::DEVICE_PREPARING:
                    status_info = "准备中";
                    status_color = DEV_STATUS_PENDING;
                    break;
                case TaskStatus::READY_FOR_PICKUP:
                    status_info = "待取货";
                    status_color = DEV_STATUS_READY;
                    break;
                case TaskStatus::ASSIGNED_TO_SHUTTLE:
                    status_info = "已分配";
                    status_color = DEV_STATUS_ASSIGNED;
                    break;
                case TaskStatus::GOODS_AT_DEST_AWAITING_REMOVAL:
                    status_info = "待清理";
                    status_color = DEV_STATUS_READY;
                    break;
                case TaskStatus::COMPLETED:
                    status_info = "完成:" +
                        std::to_string((int)tasks_vec[dev.current_task_idx_on_device].time_goods_taken_from_dest_s) + "s";
                    status_color = DEV_STATUS_COMPLETED;
                    break;
                default:
                    status_info = "未知";
                    status_color = RGB(150, 150, 150);
            }

            settextcolor(status_color);
            int status_width = textwidth(status_info.c_str());
            outtextxy(sx - status_width/2, sy + device_height + 18, status_info.c_str());
        }
    }
}

void draw_shuttles(const std::vector<Shuttle>& shuttles_vec, const std::vector<Task>& tasks_vec) {
    for (const auto& shuttle : shuttles_vec) {
        int sx, sy;
        map_track_pos_to_screen_xy(shuttle.position_mm, sx, sy);

        // 绘制穿梭车阴影效果
        setfillcolor(RGB(50, 50, 50));
        solidcircle(sx + 2, sy + 2, 10);

        // 绘制穿梭车底色
        setfillcolor(shuttle.color);
        solidcircle(sx, sy, 10);

        // 绘制穿梭车边框
        setlinecolor(RGB(50, 50, 50));
        circle(sx, sy, 10);

        // 绘制穿梭车ID
        settextcolor(WHITE);
        setbkmode(TRANSPARENT);
        settextstyle(12, 0, "微软雅黑");
        char shuttle_id_str[5];
        snprintf(shuttle_id_str, sizeof(shuttle_id_str), "S%d", shuttle.id);
        int text_width = textwidth(shuttle_id_str);
        outtextxy(sx - text_width/2, sy - 6, shuttle_id_str);

        // 根据模式显示不同的信息
        if (!RUN_TASK1_MODE) {
            // 绘制状态信息背景
            std::string status_text = get_shuttle_state_str_cn(shuttle.agent_state);
            if (shuttle.assigned_task_idx != -1 && shuttle.assigned_task_idx < (int)tasks_vec.size()) {
                const Task& t = tasks_vec[shuttle.assigned_task_idx];
                status_text += ":T" + std::to_string(t.id);
            }

            int status_width = textwidth(status_text.c_str());
            setfillcolor(RGB(240, 240, 240));
            solidroundrect(sx + 12, sy - 14, sx + 12 + status_width + 6, sy - 14 + 16, 3, 3);

            // 绘制状态文本
            settextcolor(shuttle.color);
            settextstyle(10, 0, "微软雅黑");
            outtextxy(sx + 15, sy - 12, status_text.c_str());

            // 如果穿梭车有货物，显示货物标记
            if (shuttle.has_goods()) {
                // 绘制货物标记背景
                setfillcolor(RGB(0, 120, 0));
                solidroundrect(sx - 20, sy + 12, sx + 20, sy + 25, 3, 3);

                // 绘制货物标记文本
                settextcolor(WHITE);
                settextstyle(10, 0, "微软雅黑");
                std::string goods_text = "载货中";
                int goods_width = textwidth(goods_text.c_str());
                outtextxy(sx - goods_width/2, sy + 14, goods_text.c_str());

                // 在穿梭车中心绘制货物指示
                setfillcolor(RGB(0, 150, 0));
                solidcircle(sx, sy, 5);
            }
        } else {
            // 任务1模式下的状态显示
            std::string status_text = get_shuttle_state_str_cn(shuttle.agent_state);
            int status_width = textwidth(status_text.c_str());

            setfillcolor(RGB(240, 240, 240));
            solidroundrect(sx + 12, sy - 14, sx + 12 + status_width + 6, sy - 14 + 16, 3, 3);

            settextcolor(shuttle.color);
            settextstyle(10, 0, "微软雅黑");
            outtextxy(sx + 15, sy - 12, status_text.c_str());
        }

        // 如果穿梭车有目标位置，绘制目标指示线
        if (shuttle.has_target_pos) {
            int target_x, target_y;
            map_track_pos_to_screen_xy(shuttle.target_pos_mm, target_x, target_y);

            // 绘制虚线指示
            setlinestyle(PS_DOT, 1);
            setlinecolor(RGB(200, 200, 200));
            line(sx, sy, target_x, target_y);
            setlinestyle(PS_SOLID, 1);

            // 绘制目标点
            setfillcolor(RGB(255, 200, 100));
            solidcircle(target_x, target_y, 3);
        }
    }
}


void draw_ui_info(int num_shuttles) {
    // 定义UI区域的颜色和样式
    const COLORREF TITLE_COLOR = RGB(255, 255, 200);  // 浅黄色标题
    const COLORREF TEXT_COLOR = RGB(220, 220, 220);   // 浅灰色文本
    const COLORREF HIGHLIGHT_COLOR = RGB(120, 230, 180); // 高亮绿色
    const COLORREF WARNING_COLOR = RGB(255, 180, 120);   // 警告橙色
    const COLORREF SECTION_BG_COLOR = RGB(60, 60, 70);   // 深灰色背景
    const COLORREF HEADER_BG_COLOR = RGB(40, 40, 50);    // 更深的灰色标题背景
    const COLORREF LOG_INFO_COLOR = RGB(255, 220, 100);  // 日志信息颜色

    int y_pos = 10;
    char buffer[256];

    // 设置透明背景模式
    setbkmode(TRANSPARENT);

    // ===== 顶部信息区域 =====
    // 绘制标题背景
    setfillcolor(HEADER_BG_COLOR);
    solidrectangle(5, 5, SCREEN_WIDTH-5, 35);

    // 绘制标题
    settextcolor(TITLE_COLOR);
    settextstyle(18, 0, "微软雅黑");
    outtextxy(15, y_pos, "智能仓储环形轨道穿梭车系统仿真");

    // 绘制仿真信息
    settextcolor(TEXT_COLOR);
    settextstyle(14, 0, "微软雅黑");
    snprintf(buffer, sizeof(buffer), "仿真时间: %.2f 秒  |  倍速: %.2fx  |  穿梭车: %d  |  模式: %s",
             CURRENT_SIM_TIME_S, SIM_SPEED_MULTIPLIER, num_shuttles,
             RUN_TASK1_MODE ? "任务1(巡游)" : "任务2(调度)");
    outtextxy(SCREEN_WIDTH - 450, y_pos + 2, buffer);
    y_pos += 35;

    // 绘制控制信息背景
    setfillcolor(SECTION_BG_COLOR);
    solidrectangle(5, y_pos, SCREEN_WIDTH-5, y_pos+25);

    // 绘制控制信息
    settextcolor(TEXT_COLOR);
    settextstyle(14, 0, "微软雅黑");
    snprintf(buffer, sizeof(buffer), "控制: P-暂停/继续, +/- 调整倍速, M-切换模式(重启新局), Q-退出程序");
    outtextxy(15, y_pos+4, buffer);
    y_pos += 30;

    // ===== 任务统计区域 =====
    if (!RUN_TASK1_MODE && !all_tasks_global.empty()) {
        // 计算任务统计
        int tasks_completed = 0;
        int tasks_in_progress = 0;
        int tasks_pending_preparation = 0;
        int tasks_ready_for_pickup = 0;

        for(const auto& task : all_tasks_global) {
            if (task.status == TaskStatus::COMPLETED) tasks_completed++;
            else if (task.status == TaskStatus::PENDING || task.status == TaskStatus::DEVICE_PREPARING) tasks_pending_preparation++;
            else if (task.status == TaskStatus::READY_FOR_PICKUP) tasks_ready_for_pickup++;
            else tasks_in_progress++;
        }

        // 绘制任务统计背景
        setfillcolor(SECTION_BG_COLOR);
        solidrectangle(5, y_pos, SCREEN_WIDTH-5, y_pos+25);

        // 绘制任务统计标题
        settextcolor(HIGHLIGHT_COLOR);
        settextstyle(14, 0, "微软雅黑");
        outtextxy(15, y_pos+4, "任务统计:");

        // 绘制任务统计信息
        settextcolor(TEXT_COLOR);
        snprintf(buffer, sizeof(buffer), "总数: %d | 已完成: %d | 进行中: %d | 待准备: %d | 待拾取: %d",
                 (int)all_tasks_global.size(), tasks_completed, tasks_in_progress,
                 tasks_pending_preparation, tasks_ready_for_pickup);
        outtextxy(100, y_pos+4, buffer);
        y_pos += 30;
    }

    // ===== 穿梭车状态区域 =====
    // 绘制穿梭车状态背景
    setfillcolor(SECTION_BG_COLOR);
    int shuttle_section_start = y_pos;
    int shuttle_section_height = 25;
    if (shuttles_global.size() > 4) {
        shuttle_section_height = 45; // 如果穿梭车较多，增加区域高度
    }
    solidrectangle(5, y_pos, SCREEN_WIDTH-5, y_pos+shuttle_section_height);

    // 绘制穿梭车状态标题
    settextcolor(HIGHLIGHT_COLOR);
    settextstyle(14, 0, "微软雅黑");
    outtextxy(15, y_pos+4, "穿梭车状态:");

    // 绘制穿梭车状态信息
    int x_shuttle_info = 120;
    settextcolor(TEXT_COLOR);
    settextstyle(14, 0, "微软雅黑");

    for(size_t i = 0; i < shuttles_global.size(); ++i) {
        const auto& s = shuttles_global[i];
        std::string task_str_suffix = "";

        if (!RUN_TASK1_MODE && s.assigned_task_idx != -1 && s.assigned_task_idx < (int)all_tasks_global.size()) {
            task_str_suffix = " (T" + std::to_string(all_tasks_global[s.assigned_task_idx].id);
            if(s.has_goods()) task_str_suffix += "[货]";
            task_str_suffix += ")";
        }

        // 设置穿梭车颜色
        settextcolor(s.color);

        std::string shuttle_display_str = "S" + std::to_string(s.id) + ": " + get_shuttle_state_str_cn(s.agent_state) + task_str_suffix;
        outtextxy(x_shuttle_info, y_pos+4, shuttle_display_str.c_str());

        SIZE text_size;
        GetTextExtentPoint32A(GetImageHDC(), shuttle_display_str.c_str(), shuttle_display_str.length(), &text_size);
        x_shuttle_info += text_size.cx + 25; // 增加间距

        // 如果超出屏幕宽度，换行显示
        if (x_shuttle_info > SCREEN_WIDTH - 150 && i < shuttles_global.size() - 1) {
            x_shuttle_info = 120;
            y_pos += 20;
        }
    }

    y_pos = shuttle_section_start + shuttle_section_height + 5;

    // ===== 调试信息区域 =====
    if (!debug_messages.empty()) {
        // 绘制调试信息背景
        setfillcolor(RGB(30, 40, 50)); // 深蓝色背景
        solidrectangle(5, y_pos, SCREEN_WIDTH-5, y_pos+160); // 固定高度的调试区域

        // 绘制调试信息标题和边框
        setlinecolor(RGB(100, 120, 150));
        rectangle(5, y_pos, SCREEN_WIDTH-5, y_pos+160);

        settextcolor(TITLE_COLOR);
        settextstyle(16, 0, "微软雅黑");
        outtextxy(15, y_pos+5, "调试信息:");

        // 显示日志文件信息
        settextcolor(LOG_INFO_COLOR);
        settextstyle(12, 0, "微软雅黑");
        std::string log_info = "调试日志文件: " + current_log_filename;
        outtextxy(200, y_pos+5, log_info.c_str());

        y_pos += 25;

        // 绘制调试信息内容
        settextstyle(12, 0, "宋体");

        // 从最新的消息开始显示
        int max_messages_to_show = std::min(10, (int)debug_messages.size());
        for (int i = debug_messages.size() - 1; i >= (int)debug_messages.size() - max_messages_to_show; i--) {
            if (i >= 0) {
                // 根据消息类型设置不同颜色
                if (debug_messages[i].find("警告") != std::string::npos) {
                    settextcolor(WARNING_COLOR); // 警告信息用橙色
                } else if (debug_messages[i].find("完成") != std::string::npos) {
                    settextcolor(HIGHLIGHT_COLOR); // 完成信息用绿色
                } else {
                    settextcolor(TEXT_COLOR); // 普通信息用白色
                }

                outtextxy(15, y_pos, debug_messages[i].c_str());
                y_pos += 15;
            }
        }
    }
}


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

void output_task1_summary() {
    if (!RUN_TASK1_MODE) return;

    // 准备总结信息
    std::stringstream summary_info;
    summary_info << "\n\n==================================================" << std::endl;
    summary_info << "            任务1 (巡游) 总结" << std::endl;
    summary_info << "==================================================" << std::endl;
    summary_info << "仿真总时间: " << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << " 秒" << std::endl;
    summary_info << "穿梭车数量: " << shuttles_global.size() << std::endl;
    summary_info << "穿梭车参数:" << std::endl;
    summary_info << "  直线段最大速度: " << std::fixed << std::setprecision(2) << MAX_SPEED_STRAIGHT_MM_PER_S << " mm/s ("
                 << MAX_SPEED_STRAIGHT_MM_PER_S * 60.0 / 1000.0 << " m/min)" << std::endl;
    summary_info << "  弯道段最大速度: " << std::fixed << std::setprecision(2) << MAX_SPEED_CURVE_MM_PER_S << " mm/s ("
                 << MAX_SPEED_CURVE_MM_PER_S * 60.0 / 1000.0 << " m/min)" << std::endl;
    summary_info << "  加速度: " << std::fixed << std::setprecision(2) << ACCEL_MM_PER_S2 << " mm/s? ("
                 << ACCEL_MM_PER_S2 / 1000.0 << " m/s?)" << std::endl;
    summary_info << "  减速度: " << std::fixed << std::setprecision(2) << DECEL_MM_PER_S2 << " mm/s? ("
                 << DECEL_MM_PER_S2 / 1000.0 << " m/s?)" << std::endl;

    std::cout << summary_info.str();

    // 创建任务1总结文件
    std::ofstream summary_file("Task1Summary.txt");
    if (summary_file.is_open()) {
        summary_file << "--- 任务1 (巡游) 总结 ---" << std::endl;
        summary_file << "仿真总时间: " << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << " 秒" << std::endl;
        summary_file << "穿梭车数量: " << shuttles_global.size() << std::endl;
        summary_file << "穿梭车参数:" << std::endl;
        summary_file << "  直线段最大速度: " << std::fixed << std::setprecision(2) << MAX_SPEED_STRAIGHT_MM_PER_S << " mm/s ("
                     << MAX_SPEED_STRAIGHT_MM_PER_S * 60.0 / 1000.0 << " m/min)" << std::endl;
        summary_file << "  弯道段最大速度: " << std::fixed << std::setprecision(2) << MAX_SPEED_CURVE_MM_PER_S << " mm/s ("
                     << MAX_SPEED_CURVE_MM_PER_S * 60.0 / 1000.0 << " m/min)" << std::endl;
        summary_file << "  加速度: " << std::fixed << std::setprecision(2) << ACCEL_MM_PER_S2 << " mm/s? ("
                     << ACCEL_MM_PER_S2 / 1000.0 << " m/s?)" << std::endl;
        summary_file << "  减速度: " << std::fixed << std::setprecision(2) << DECEL_MM_PER_S2 << " mm/s? ("
                     << DECEL_MM_PER_S2 / 1000.0 << " m/s?)" << std::endl;
    }

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

        for (const auto& rec : shuttle.movement_log_task1) {
            shuttle_info << "    加减速记录 #" << (&rec - &shuttle.movement_log_task1[0] + 1) << ":" << std::endl;
            shuttle_info << "      起始时间: " << std::fixed << std::setprecision(2) << rec.start_time_s << " 秒" << std::endl;
            shuttle_info << "      起始速度: " << std::fixed << std::setprecision(2) << rec.start_speed_mm_s << " mm/s" << std::endl;
            shuttle_info << "      起始加速度: " << std::fixed << std::setprecision(2) << rec.acceleration_mm_s2 << " mm/s?" << std::endl;
            shuttle_info << "      终止时间: " << std::fixed << std::setprecision(2) << rec.end_time_s << " 秒" << std::endl;
            shuttle_info << "      终止速度: " << std::fixed << std::setprecision(2) << rec.end_speed_mm_s << " mm/s" << std::endl;
            shuttle_info << "      终止加速度: " << std::fixed << std::setprecision(2) << rec.acceleration_mm_s2 << " mm/s?" << std::endl;
        }

        // 输出到控制台
        std::cout << shuttle_info.str();

        // 写入到文件
        if (summary_file.is_open()) {
            summary_file << shuttle_info.str();
        }
    }

    if (summary_file.is_open()) {
        summary_file.close();
        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << "任务1总结已写入 Task1Summary.txt" << std::endl;
    }
}

void output_task2_summary_files() {
    if (RUN_TASK1_MODE) return;
    if (shuttles_global.empty() && !RUN_TASK1_MODE) {
        std::cout << "任务2总结：无穿梭车运行，不生成总结文件。" << std::endl;
        return;
    }
    if (all_tasks_global.empty() && !RUN_TASK1_MODE) {
        std::cout << "任务2总结：无任务列表，不生成总结文件。" << std::endl;
        return;
    }


    std::string suffix = "_T2_" + std::to_string(shuttles_global.size()) + "cars";
    std::ofstream summary_file("SimulationSummary" + suffix + ".txt");

    if (!summary_file.is_open()) {
        std::cerr << "错误: 无法打开 SimulationSummary" << suffix << ".txt" << std::endl;
        return;
    }

    // 准备总结信息
    std::stringstream summary_info;
    summary_info << "\n\n==================================================" << std::endl;
    summary_info << "            仿真总结 (任务 2 - " << shuttles_global.size() << "台车)" << std::endl;
    summary_info << "==================================================" << std::endl;
    summary_info << "完成所有任务总耗时: " << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << " 秒" << std::endl;
    summary_info << "穿梭车数量: " << shuttles_global.size() << std::endl;
    summary_info << "\n设备统计数据:" << std::endl;
    summary_info << "设备ID\t设备名称\t总空闲时间(秒)\t总有货时间(秒)" << std::endl;
    summary_info << "--------------------------------------------------" << std::endl;

    // 写入到文件
    summary_file << "--- 仿真总结 (任务 2) ---" << std::endl;
    summary_file << "完成所有任务总耗时: " << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << " 秒" << std::endl;
    summary_file << "穿梭车数量: " << shuttles_global.size() << std::endl;
    summary_file << "\n设备统计数据:\n";
    summary_file << "设备ID\t设备名称\t总空闲时间(秒)\t总有货时间(秒)\n";

    // 同时输出到控制台
    std::cout << summary_info.str();

    for (auto& pair : devices_global) {
        Device& dev = pair.second;
        double time_in_final_state = CURRENT_SIM_TIME_S - dev.last_state_change_time_s;
        if (dev.last_state_change_time_s < CURRENT_SIM_TIME_S - 1e-4) {
            if (dev.op_state == DeviceOperationalState::IDLE_EMPTY) {
                dev.total_idle_time_s += time_in_final_state;
            }
            if (dev.op_state == DeviceOperationalState::HAS_GOODS_WAITING_FOR_SHUTTLE ||
                dev.op_state == DeviceOperationalState::HAS_GOODS_BEING_ACCESSED_BY_SHUTTLE ||
                dev.op_state == DeviceOperationalState::HAS_GOODS_WAITING_FOR_CLEARANCE) {
                dev.total_has_goods_time_s += time_in_final_state;
            }
        }
        dev.last_state_change_time_s = CURRENT_SIM_TIME_S;

        // 写入到文件
        summary_file << dev.id << "\t" << dev.name << "\t"
                     << std::fixed << std::setprecision(2) << dev.total_idle_time_s << "\t"
                     << std::fixed << std::setprecision(2) << dev.total_has_goods_time_s << "\n";

        // 同时输出到控制台
        std::cout << dev.id << "\t" << dev.name << "\t"
                  << std::fixed << std::setprecision(2) << dev.total_idle_time_s << "\t"
                  << std::fixed << std::setprecision(2) << dev.total_has_goods_time_s << std::endl;
    }

    summary_file.close();
    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "任务2总结已写入 SimulationSummary" << suffix << ".txt" << std::endl;

    std::cout << "\n系统布局改进建议 (一般性建议，具体需分析日志 TaskExeLog.txt 和 DeviceStateLog.txt):" << std::endl;
    std::cout << "1. 瓶颈分析: 检查DeviceStateLog.txt中各设备（特别是接口设备和出入库口）的 total_has_goods_time_s 和 total_idle_time_s。"
              << "长时间处于 HAS_GOODS_WAITING_FOR_CLEARANCE 或 HAS_GOODS_WAITING_FOR_SHUTTLE 可能表示瓶颈。" << std::endl;
    std::cout << "2. 任务分配: 观察TaskExeLog.txt中各穿梭车的繁忙程度。如果部分车辆任务繁重而其他空闲，可能需要优化调度算法。" << std::endl;
    std::cout << "3. 出入库口位置: 根据图2, 出入库口(13-18)的位置已按图示设定。若日志显示大量小车行程集中在轨道某特定区域往返这些固定口，"
              << "且问题允许调整这些口在底部轨道上的相对位置（不改变轨道本身和40000mm总长），将其移近作业重心区或更均匀分布可能减少小车平均行程。"
              << "但当前实现是严格按照图2尺寸。" << std::endl;
    std::cout << "4. 接口设备数量/效率: 如果入库接口(1,3..11)或出库接口(2,4..12)的堆垛机作业时间(25s/50s)导致接口设备长时间占用，"
              << "成为系统瓶颈，则需关注堆垛机效率。本题中这些接口位置和作业时间固定。" << std::endl;
    std::cout << "5. 缓冲策略: 若特定接口前经常发生拥堵或等待，考虑是否可在轨道上增加临时停车/缓冲点（若允许修改轨道）。"
              << "题目不允许改轨道，但这是通用思路。" << std::endl;
    std::cout << "6. 穿梭车数量: 比较3, 5, 7台车时的总耗时和设备利用率。若增加车辆边际效益递减明显，说明系统瓶颈可能在别处（如设备处理能力）。" << std::endl;

}

// --- Main Simulation Loop ---
void run_simulation_loop(int num_shuttles_to_run) {
    init_drawing_parameters();
    init_devices();
    init_shuttles(num_shuttles_to_run);

    if (!RUN_TASK1_MODE) {
        load_tasks_from_markdown_data();
        init_task2_device_initial_tasks();
    } else {
        all_tasks_global.clear();
        pending_task_queues_by_start_device.clear();
    }

    open_log_files();

    CURRENT_SIM_TIME_S = 0.0;
    PAUSED = false;

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


    unsigned long last_frame_time_ms = GetTickCount();
    unsigned long sim_steps_this_frame = 0;
    bool simulation_complete_flag = false;

    while (true) {
        unsigned long current_frame_time_ms = GetTickCount();
        unsigned long elapsed_ms_real = current_frame_time_ms - last_frame_time_ms;

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


        if (sim_steps_this_frame > 0 && !simulation_complete_flag) {
            for (unsigned long step = 0; step < sim_steps_this_frame; ++step) {
                if (!RUN_TASK1_MODE) {
                    for (auto& pair : devices_global) {
                        update_device_state(pair.second, all_tasks_global, pending_task_queues_by_start_device);
                    }
                }

                if (!RUN_TASK1_MODE) {
                    schedule_tasks(shuttles_global, all_tasks_global, devices_global);
                }

                for (int i=0; i < (int)shuttles_global.size(); ++i) {
                    update_shuttle_physics_and_state(shuttles_global[i], i, shuttles_global, devices_global, all_tasks_global);
                }

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

                if (!RUN_TASK1_MODE && all_tasks_completed()) {
                    simulation_complete_flag = true;
                    PAUSED = true;
                    std::cout << "所有任务已完成 (" << num_shuttles_to_run << " 台穿梭车)。总耗时: "
                              << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << "秒" << std::endl;
                    output_task2_summary_files();
                    break;
                }
            }
        }

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

        draw_track();
        draw_devices(devices_global, all_tasks_global);
        draw_shuttles(shuttles_global, all_tasks_global);
        EndBatchDraw();

        if (_kbhit()) {
            char key = _getch();
            if (key == 'q' || key == 'Q') {
                if (RUN_TASK1_MODE && !shuttles_global.empty()) {
                    for(auto& s : shuttles_global) { if (s.in_logged_accel_decel_phase) log_shuttle_movement_phase_task1(s); }
                    output_task1_summary();
                } else if (!RUN_TASK1_MODE && simulation_complete_flag && !shuttles_global.empty()) {
                    // Summary already output if auto-paused.
                }
                close_log_files();
                return;
            }
            if (key == 'p' || key == 'P') PAUSED = !PAUSED;
            if (key == '+' || key == '=') SIM_SPEED_MULTIPLIER = std::min(256.0, SIM_SPEED_MULTIPLIER * 1.5);
            if (key == '-') SIM_SPEED_MULTIPLIER = std::max(0.05, SIM_SPEED_MULTIPLIER / 1.5);

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

                        switch(dev.op_state) {
                            case DeviceOperationalState::IDLE_EMPTY:
                                debug_file << "空闲无货"; break;
                            case DeviceOperationalState::HAS_GOODS_WAITING_FOR_SHUTTLE:
                                debug_file << "有货等待穿梭车"; break;
                            case DeviceOperationalState::HAS_GOODS_WAITING_FOR_CLEARANCE:
                                debug_file << "有货等待清理"; break;
                            case DeviceOperationalState::HAS_GOODS_BEING_ACCESSED_BY_SHUTTLE:
                                debug_file << "有货被穿梭车访问中"; break;
                            case DeviceOperationalState::BUSY_SUPPLYING_NEXT_TASK:
                                debug_file << "忙碌准备下一任务"; break;
                            case DeviceOperationalState::BUSY_CLEARING_GOODS:
                                debug_file << "忙碌清理货物"; break;
                            default:
                                debug_file << "未知状态";
                        }

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
                if (RUN_TASK1_MODE && !shuttles_global.empty()) {
                    for(auto& s : shuttles_global) { if (s.in_logged_accel_decel_phase) log_shuttle_movement_phase_task1(s); }
                    output_task1_summary();
                } else if (!RUN_TASK1_MODE && !shuttles_global.empty()){
                    if (simulation_complete_flag) {
                        // Already outputted.
                    } else {
                        std::cout << "任务2未完成，切换模式时不生成最终总结。" << std::endl;
                    }
                }
                RUN_TASK1_MODE = !RUN_TASK1_MODE;
                close_log_files();
                return;
            }
        }

        if (simulation_complete_flag && !PAUSED) {
            PAUSED = true;
        }

        // 任务2模式下，3小时超时
        if (!RUN_TASK1_MODE && CURRENT_SIM_TIME_S > 3600 * 3 && !simulation_complete_flag) {
            std::cout << "警告: 仿真时间超过3小时，可能存在死锁或任务无法完成。自动暂停。" << std::endl;
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

            // 我们不再检查停车次数，只关心车辆是否完成一圈

            // 满足条件：所有穿梭车完成一圈巡游，或者仿真时间超过10分钟
            if (all_shuttles_completed_circle || CURRENT_SIM_TIME_S > 600) {
                std::stringstream reason;
                if (all_shuttles_completed_circle) {
                    reason << "所有穿梭车已完成一整圈巡游";
                } else {
                    reason << "仿真时间已超过10分钟，但部分穿梭车未完成一整圈巡游";
                }

                std::cout << "任务1完成条件已满足（" << reason.str() << "），生成统计报告。" << std::endl;
                add_debug_message("任务1完成条件已满足（" + reason.str() + "），生成统计报告。");

                // 确保记录最后的加减速阶段
                for(auto& s : shuttles_global) {
                    if (s.in_logged_accel_decel_phase) log_shuttle_movement_phase_task1(s);
                }

                output_task1_summary();
                PAUSED = true;
                simulation_complete_flag = true;
            }
        }

        if (sim_steps_this_frame == 0 && !PAUSED) {
            Sleep(1);
        }
    }
}


// 创建调试日志文件

// 显示图形化主菜单
enum class MenuResult { START_SIMULATION, SWITCH_MODE, CHANGE_SHUTTLES, EXIT };

MenuResult show_graphical_menu(bool& run_task1_mode, int& num_shuttles) {
    // 清除屏幕
    cleardevice();

    // 绘制背景
    setbkcolor(RGB(30, 30, 40));
    cleardevice();

    // 绘制标题
    draw_title("智能仓储环形轨道穿梭车系统仿真", SCREEN_WIDTH / 2, 50);

    // 绘制当前模式信息
    std::string mode_text = "当前模式: " + std::string(run_task1_mode ? "任务1 (巡游与记录)" : "任务2 (任务调度与日志)");
    draw_info_text(mode_text, 50, 100, RGB(180, 220, 255));

    // 绘制穿梭车数量信息
    std::string shuttle_text;
    if (run_task1_mode) {
        shuttle_text = "穿梭车数量: 3 (固定)";
    } else {
        shuttle_text = "穿梭车数量: " + std::to_string(num_shuttles);
    }
    draw_info_text(shuttle_text, 50, 130, RGB(180, 220, 255));

    // 绘制日志文件信息
    draw_info_text("调试日志文件: " + current_log_filename, 50, 160, RGB(255, 220, 100));

    // 创建菜单按钮
    std::vector<MenuButton> buttons;
    buttons.push_back(MenuButton(SCREEN_WIDTH / 2 - 150, 220, 300, 50, "开始仿真", 'S'));
    buttons.push_back(MenuButton(SCREEN_WIDTH / 2 - 150, 290, 300, 50, "切换模式", 'M'));
    buttons.push_back(MenuButton(SCREEN_WIDTH / 2 - 150, 360, 300, 50, "更改穿梭车数量", 'N', !run_task1_mode));
    buttons.push_back(MenuButton(SCREEN_WIDTH / 2 - 150, 430, 300, 50, "退出程序", 'Q'));

    // 绘制说明信息
    draw_info_text("操作说明:", 50, 500, RGB(255, 255, 150));
    draw_info_text("1. 点击按钮或按下对应的热键 [S/M/N/Q] 选择操作", 70, 530);
    draw_info_text("2. 仿真过程中按 P 暂停/继续, +/- 调整速度, D 保存调试信息, Q 退出", 70, 560);
    draw_info_text("3. 所有调试信息将保存到日志文件中", 70, 590);

    // 绘制按钮
    int hover_button = -1;
    for (size_t i = 0; i < buttons.size(); i++) {
        draw_menu_button(buttons[i], false);
    }

    // 处理鼠标和键盘事件
    MOUSEMSG msg;
    bool menu_active = true;
    MenuResult result = MenuResult::EXIT;

    while (menu_active) {
        // 检查鼠标事件
        if (MouseHit()) {
            msg = GetMouseMsg();

            // 检查鼠标是否悬停在按钮上
            int new_hover = -1;
            for (size_t i = 0; i < buttons.size(); i++) {
                if (buttons[i].enabled && buttons[i].contains(msg.x, msg.y)) {
                    new_hover = i;
                    break;
                }
            }

            // 如果悬停状态改变，重绘按钮
            if (new_hover != hover_button) {
                // 重绘所有按钮
                for (size_t i = 0; i < buttons.size(); i++) {
                    draw_menu_button(buttons[i], i == new_hover);
                }
                hover_button = new_hover;
            }

            // 检查鼠标点击
            if (msg.uMsg == WM_LBUTTONDOWN && hover_button != -1) {
                switch (hover_button) {
                    case 0: // 开始仿真
                        result = MenuResult::START_SIMULATION;
                        menu_active = false;
                        break;
                    case 1: // 切换模式
                        run_task1_mode = !run_task1_mode;
                        // 更新模式信息
                        setfillcolor(RGB(30, 30, 40));
                        solidrectangle(50, 100, 500, 130);
                        mode_text = "当前模式: " + std::string(run_task1_mode ? "任务1 (巡游与记录)" : "任务2 (任务调度与日志)");
                        draw_info_text(mode_text, 50, 100, RGB(180, 220, 255));
                        // 更新穿梭车数量信息
                        setfillcolor(RGB(30, 30, 40));
                        solidrectangle(50, 130, 500, 160);
                        if (run_task1_mode) {
                            shuttle_text = "穿梭车数量: 3 (固定)";
                        } else {
                            shuttle_text = "穿梭车数量: " + std::to_string(num_shuttles);
                        }
                        draw_info_text(shuttle_text, 50, 130, RGB(180, 220, 255));
                        // 更新按钮状态
                        buttons[2].enabled = !run_task1_mode;
                        for (size_t i = 0; i < buttons.size(); i++) {
                            draw_menu_button(buttons[i], i == hover_button);
                        }
                        result = MenuResult::SWITCH_MODE;
                        break;
                    case 2: // 更改穿梭车数量
                        if (!run_task1_mode) {
                            // 循环切换穿梭车数量：3 -> 5 -> 7 -> 3
                            num_shuttles = (num_shuttles == 3) ? 5 : ((num_shuttles == 5) ? 7 : 3);
                            // 更新穿梭车数量信息
                            setfillcolor(RGB(30, 30, 40));
                            solidrectangle(50, 130, 500, 160);
                            shuttle_text = "穿梭车数量: " + std::to_string(num_shuttles);
                            draw_info_text(shuttle_text, 50, 130, RGB(180, 220, 255));
                            result = MenuResult::CHANGE_SHUTTLES;
                        }
                        break;
                    case 3: // 退出程序
                        result = MenuResult::EXIT;
                        menu_active = false;
                        break;
                }
            }
        }

        // 检查键盘事件
        if (_kbhit()) {
            char key = toupper(_getch());
            switch (key) {
                case 'S': // 开始仿真
                    result = MenuResult::START_SIMULATION;
                    menu_active = false;
                    break;
                case 'M': // 切换模式
                    run_task1_mode = !run_task1_mode;
                    // 更新模式信息
                    setfillcolor(RGB(30, 30, 40));
                    solidrectangle(50, 100, 500, 130);
                    mode_text = "当前模式: " + std::string(run_task1_mode ? "任务1 (巡游与记录)" : "任务2 (任务调度与日志)");
                    draw_info_text(mode_text, 50, 100, RGB(180, 220, 255));
                    // 更新穿梭车数量信息
                    setfillcolor(RGB(30, 30, 40));
                    solidrectangle(50, 130, 500, 160);
                    if (run_task1_mode) {
                        shuttle_text = "穿梭车数量: 3 (固定)";
                    } else {
                        shuttle_text = "穿梭车数量: " + std::to_string(num_shuttles);
                    }
                    draw_info_text(shuttle_text, 50, 130, RGB(180, 220, 255));
                    // 更新按钮状态
                    buttons[2].enabled = !run_task1_mode;
                    for (size_t i = 0; i < buttons.size(); i++) {
                        draw_menu_button(buttons[i], i == hover_button);
                    }
                    result = MenuResult::SWITCH_MODE;
                    break;
                case 'N': // 更改穿梭车数量
                    if (!run_task1_mode) {
                        // 循环切换穿梭车数量：3 -> 5 -> 7 -> 3
                        num_shuttles = (num_shuttles == 3) ? 5 : ((num_shuttles == 5) ? 7 : 3);
                        // 更新穿梭车数量信息
                        setfillcolor(RGB(30, 30, 40));
                        solidrectangle(50, 130, 500, 160);
                        shuttle_text = "穿梭车数量: " + std::to_string(num_shuttles);
                        draw_info_text(shuttle_text, 50, 130, RGB(180, 220, 255));
                        result = MenuResult::CHANGE_SHUTTLES;
                    }
                    break;
                case 'Q': // 退出程序
                    result = MenuResult::EXIT;
                    menu_active = false;
                    break;
            }
        }

        // 短暂延时，减少CPU使用
        Sleep(10);
    }

    return result;
}

void create_debug_log_file() {
    // 设置控制台输出为GBK编码
    SetConsoleOutputCP(936); // 936是GBK编码

    // 创建带有时间戳的日志文件名
    time_t now = time(0);
    struct tm* timeinfo = localtime(&now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", timeinfo);

    // 删除之前的调试日志文件
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile("DebugLog_*.txt", &findFileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            DeleteFile(findFileData.cFileName);
        } while (FindNextFile(hFind, &findFileData));
        FindClose(hFind);
    }

    current_log_filename = "DebugLog_" + std::string(buffer) + ".txt";

    // 直接打开文件，使用GBK编码
    debug_log_file.open(current_log_filename.c_str());

    if (debug_log_file.is_open()) {
        debug_log_file << "=== 调试日志开始 (" << buffer << ") ===" << std::endl;
        debug_log_file << "注意：由于EasyX图形库的限制，控制台可能无法正常显示文本" << std::endl;
        debug_log_file << "所有调试信息将保存到此日志文件中" << std::endl;
        debug_log_file << "=== 系统信息 ===" << std::endl;
        debug_log_file << "模式: " << (RUN_TASK1_MODE ? "任务1 (巡游)" : "任务2 (调度)") << std::endl;
        debug_log_file << "穿梭车数量: " << shuttles_global.size() << std::endl;
        debug_log_file << "=== 调试日志 ===" << std::endl;

        // 尝试在控制台输出，但可能不会显示
        std::cout << "调试日志将保存到: " << current_log_filename << std::endl;
    } else {
        // 尝试在控制台输出错误信息，但可能不会显示
        std::cerr << "无法创建调试日志文件!" << std::endl;
    }
}

int main() {
    // 设置控制台输出为GBK编码
    SetConsoleOutputCP(936); // 936是GBK编码
    // 设置控制台输入为GBK编码
    SetConsoleCP(936);

    setlocale(LC_ALL, "chs");

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

    closegraph();
    std::cout << "程序已退出。" << std::endl;
    return 0;
}

