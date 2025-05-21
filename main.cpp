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

// ������Ϣ������
const int MAX_DEBUG_MESSAGES = 20;
std::vector<std::string> debug_messages;

// ȫ�ֱ������洢��ǰ��־�ļ���
std::string current_log_filename;

// ��������
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
double SIM_SPEED_MULTIPLIER = 10.0; // Ĭ����ߵ�10����
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

// --- ͼ�β˵���ؽṹ�ͺ��� ---
// �˵���ť�ṹ
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

// ���Ʋ˵���ť
void draw_menu_button(const MenuButton& button, bool hover = false) {
    // ��ť����
    COLORREF bg_color = button.enabled ?
        (hover ? RGB(80, 120, 170) : RGB(60, 80, 120)) :
        RGB(100, 100, 100);
    setfillcolor(bg_color);
    solidroundrect(button.x, button.y, button.x + button.width, button.y + button.height, 10, 10);

    // ��ť�߿�
    setlinecolor(RGB(150, 150, 150));
    roundrect(button.x, button.y, button.x + button.width, button.y + button.height, 10, 10);

    // ��ť�ı�
    settextcolor(WHITE);
    settextstyle(20, 0, "΢���ź�");
    int text_width = textwidth(button.text.c_str());
    int text_height = textheight(button.text.c_str());
    outtextxy(button.x + (button.width - text_width) / 2,
              button.y + (button.height - text_height) / 2,
              button.text.c_str());

    // �ȼ���ʾ
    std::string hotkey_text = std::string("[") + button.hotkey + "]";
    settextstyle(14, 0, "΢���ź�");
    outtextxy(button.x + button.width - 30, button.y + button.height - 20, hotkey_text.c_str());
}

// ���Ʊ���
void draw_title(const std::string& title, int x, int y) {
    settextcolor(RGB(255, 255, 200));
    settextstyle(28, 0, "΢���ź�");
    int text_width = textwidth(title.c_str());
    outtextxy(x - text_width / 2, y, title.c_str());
}

// ������Ϣ�ı�
void draw_info_text(const std::string& text, int x, int y, COLORREF color = RGB(220, 220, 220)) {
    settextcolor(color);
    settextstyle(16, 0, "΢���ź�");
    outtextxy(x, y, text.c_str());
}

// ������־�ļ�
std::ofstream debug_log_file;

// ��ӵ�����Ϣ������������־�ļ�
void add_debug_message(const std::string& message) {
    // ���ʱ���
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << "[" << CURRENT_SIM_TIME_S << "s] " << message;

    // ��ӵ�������
    debug_messages.push_back(ss.str());

    // ����������������ɾ����ɵ���Ϣ
    if (debug_messages.size() > MAX_DEBUG_MESSAGES) {
        debug_messages.erase(debug_messages.begin());
    }

    // ˢ�����������
    std::cout.flush();

    // ͬʱ���������̨������ɼ���
    std::cout << ss.str() << std::endl;

    // д�뵽������־�ļ�
    if (debug_log_file.is_open()) {
        debug_log_file << ss.str() << std::endl;
        debug_log_file.flush(); // ȷ������д���ļ�
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

    // ����һѲ��һȦ��ر���
    double initial_position_mm;  // ��ʼλ��
    bool completed_full_circle;  // �Ƿ����һ��Ȧ
    double distance_traveled_mm; // ����ʻ����

    // ��ײ�����ر���
    bool is_stopped_at_station;  // �Ƿ���վ��ֹͣ
    double stopped_position_mm;  // ֹͣλ��

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
    return type == TaskType::INBOUND ? "���" : (type == TaskType::OUTBOUND ? "����" : "N/A");
}

std::string get_shuttle_state_str_cn(ShuttleAgentState state) {
    switch (state) {
        case ShuttleAgentState::IDLE_EMPTY: return "����";
        case ShuttleAgentState::MOVING_TO_PICKUP: return "��ȡ����";
        case ShuttleAgentState::WAITING_FOR_PICKUP_AVAILABILITY: return "�ȴ�ȡ��";
        case ShuttleAgentState::LOADING: return "װ����";
        case ShuttleAgentState::MOVING_TO_DROPOFF: return "��ж����";
        case ShuttleAgentState::WAITING_FOR_DROPOFF_AVAILABILITY: return "�ȴ�ж��";
        case ShuttleAgentState::UNLOADING: return "ж����";
        case ShuttleAgentState::PATROLLING_CRUISE: return "Ѳ��";
        case ShuttleAgentState::PATROLLING_ACCEL: return "Ѳ��-����";
        case ShuttleAgentState::PATROLLING_DECEL: return "Ѳ��-����";
        case ShuttleAgentState::PATROLLING_STOPPED: return "Ѳ��-ֹͣ";
        default: return "δ֪״̬";
    }
}

std::string get_device_op_state_str_cn(DeviceOperationalState state) {
    switch (state) {
        case DeviceOperationalState::IDLE_EMPTY: return "�����޻�";
        case DeviceOperationalState::BUSY_SUPPLYING_NEXT_TASK: return "������";
        case DeviceOperationalState::HAS_GOODS_WAITING_FOR_SHUTTLE: return "�л�(��ȡ)";
        case DeviceOperationalState::HAS_GOODS_BEING_ACCESSED_BY_SHUTTLE: return "С��������";
        case DeviceOperationalState::HAS_GOODS_WAITING_FOR_CLEARANCE: return "�л�(����)";
        case DeviceOperationalState::BUSY_CLEARING_GOODS: return "�����";
        default: return "δ֪״̬";
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
        if (name_prefix == "���ӿ�" || name_prefix == "����ӿ�" || name_prefix == "����" || name_prefix == "�����"){
            d.name += std::to_string(id);
        }
        devices_global[id] = d;
    };

    std::cout << "--- Initializing Devices (Positions relative to start of their straight segment) ---" << std::endl;
    std::cout << "Track Straight Length: " << TRACK_STRAIGHT_LENGTH << " mm, Curve Radius: " << TRACK_CURVE_RADIUS << " mm" << std::endl;
    std::cout << "Total Track Length: " << TOTAL_TRACK_LENGTH << " mm" << std::endl;
    std::cout << "Top Straight: " << POS_TOP_STRAIGHT_START << " to " << POS_TOP_STRAIGHT_END << std::endl;
    std::cout << "Bottom Straight: " << POS_BOTTOM_STRAIGHT_START << " to " << POS_BOTTOM_STRAIGHT_END << std::endl;

    // ����ͼƬ���������豸λ��
    // �豸��ȳ���
    const double DEVICE_WIDTH = 1100.0;

    // �·�ֱ�߲��� - �Զ����ִ��� (�豸 1-12)
    // �豸1 - �������3250mm�������豸���ĵ㣩
    double current_pos_bottom = 3250.0;
    add_dev(1, DeviceModelType::IN_INTERFACE, current_pos_bottom, "���ӿ�", false);

    // �豸2 - �����豸1���ĵ�1300mm + �豸1��ȵ�һ�� + �豸2��ȵ�һ��
    current_pos_bottom += 1300.0 + DEVICE_WIDTH;
    add_dev(2, DeviceModelType::OUT_INTERFACE, current_pos_bottom, "����ӿ�", false);

    // �豸3 - �����豸2���ĵ�2500mm + �豸2��ȵ�һ�� + �豸3��ȵ�һ��
    current_pos_bottom += 2500.0 + DEVICE_WIDTH;
    add_dev(3, DeviceModelType::IN_INTERFACE, current_pos_bottom, "���ӿ�", false);

    // �豸4 - �����豸3���ĵ�1300mm + �豸3��ȵ�һ�� + �豸4��ȵ�һ��
    current_pos_bottom += 1300.0 + DEVICE_WIDTH;
    add_dev(4, DeviceModelType::OUT_INTERFACE, current_pos_bottom, "����ӿ�", false);

    // �豸5 - �����豸4���ĵ�2500mm + �豸4��ȵ�һ�� + �豸5��ȵ�һ��
    current_pos_bottom += 2500.0 + DEVICE_WIDTH;
    add_dev(5, DeviceModelType::IN_INTERFACE, current_pos_bottom, "���ӿ�", false);

    // �豸6 - �����豸5���ĵ�1300mm + �豸5��ȵ�һ�� + �豸6��ȵ�һ��
    current_pos_bottom += 1300.0 + DEVICE_WIDTH;
    add_dev(6, DeviceModelType::OUT_INTERFACE, current_pos_bottom, "����ӿ�", false);

    // �豸7 - �����豸6���ĵ�2500mm + �豸6��ȵ�һ�� + �豸7��ȵ�һ��
    current_pos_bottom += 2500.0 + DEVICE_WIDTH;
    add_dev(7, DeviceModelType::IN_INTERFACE, current_pos_bottom, "���ӿ�", false);

    // �豸8 - �����豸7���ĵ�1300mm + �豸7��ȵ�һ�� + �豸8��ȵ�һ��
    current_pos_bottom += 1300.0 + DEVICE_WIDTH;
    add_dev(8, DeviceModelType::OUT_INTERFACE, current_pos_bottom, "����ӿ�", false);

    // �豸9 - �����豸8���ĵ�2500mm + �豸8��ȵ�һ�� + �豸9��ȵ�һ��
    current_pos_bottom += 2500.0 + DEVICE_WIDTH;
    add_dev(9, DeviceModelType::IN_INTERFACE, current_pos_bottom, "���ӿ�", false);

    // �豸10 - �����豸9���ĵ�1300mm + �豸9��ȵ�һ�� + �豸10��ȵ�һ��
    current_pos_bottom += 1300.0 + DEVICE_WIDTH;
    add_dev(10, DeviceModelType::OUT_INTERFACE, current_pos_bottom, "����ӿ�", false);

    // �豸11 - �����豸10���ĵ�2500mm + �豸10��ȵ�һ�� + �豸11��ȵ�һ��
    current_pos_bottom += 2500.0 + DEVICE_WIDTH;
    add_dev(11, DeviceModelType::IN_INTERFACE, current_pos_bottom, "���ӿ�", false);

    // �豸12 - �����豸11���ĵ�1300mm + �豸11��ȵ�һ�� + �豸12��ȵ�һ��
    current_pos_bottom += 1300.0 + DEVICE_WIDTH;
    add_dev(12, DeviceModelType::OUT_INTERFACE, current_pos_bottom, "����ӿ�", false);

    // �Ϸ�ֱ�߲��� - �������ҵ�� (�豸 13-18)
    // �豸13 - �����Ҷ�7450mm (������������)
    double pos_dev13_rel = TRACK_STRAIGHT_LENGTH - 7450.0;
    add_dev(13, DeviceModelType::OUT_PORT, pos_dev13_rel, "�����", true);

    // �豸14 - �����豸13���ĵ�1900mm + �豸13��ȵ�һ�� + �豸14��ȵ�һ��
    double pos_dev14_rel = pos_dev13_rel - (1900.0 + DEVICE_WIDTH);
    add_dev(14, DeviceModelType::OUT_PORT, pos_dev14_rel, "�����", true);

    // �豸15 - �����豸14���ĵ�1900mm + �豸14��ȵ�һ�� + �豸15��ȵ�һ��
    double pos_dev15_rel = pos_dev14_rel - (1900.0 + DEVICE_WIDTH);
    add_dev(15, DeviceModelType::OUT_PORT, pos_dev15_rel, "�����", true);

    // �豸16 - �����豸15���ĵ�10900mm + �豸15��ȵ�һ�� + �豸16��ȵ�һ��
    double pos_dev16_rel = pos_dev15_rel - (10900.0 + DEVICE_WIDTH);
    add_dev(16, DeviceModelType::IN_PORT, pos_dev16_rel, "����", true);

    // �豸17 - �����豸16���ĵ�1900mm + �豸16��ȵ�һ�� + �豸17��ȵ�һ��
    double pos_dev17_rel = pos_dev16_rel - (1900.0 + DEVICE_WIDTH);
    add_dev(17, DeviceModelType::IN_PORT, pos_dev17_rel, "����", true);

    // �豸18 - �����豸17���ĵ�1900mm + �豸17��ȵ�һ�� + �豸18��ȵ�һ��
    double pos_dev18_rel = pos_dev17_rel - (1900.0 + DEVICE_WIDTH);
    add_dev(18, DeviceModelType::IN_PORT, pos_dev18_rel, "����", true);

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

    // ����15�ų����λ��
    double pos_dev15 = POS_BOTTOM_STRAIGHT_START;
    bool found_dev15 = false;

    if (devices_global.find(15) != devices_global.end()) {
        pos_dev15 = devices_global.at(15).position_on_track_mm;
        found_dev15 = true;
        std::cout << "�ҵ�15�ų���ڣ�λ��: " << pos_dev15 << " mm" << std::endl;
    } else {
        std::cerr << "����: ��ʼ������ʱδ�ҵ�15�ų����!" << std::endl;
        // �����ҵ�����������Ϊ����
        for(const auto& pair : devices_global){
            if(pair.second.model_type == DeviceModelType::OUT_PORT){
                pos_dev15 = pair.second.position_on_track_mm;
                found_dev15 = true;
                std::cout << "����: δ�ҵ�15�ų���ڣ�ʹ���豸 " << pair.first << " λ��: " << pos_dev15 << " mm" << std::endl;
                break;
            }
        }

        if (!found_dev15) {
            std::cout << "����: δ�ҵ��κγ���ڣ�ʹ��Ĭ��λ�á�" << std::endl;
        }
    }

    COLORREF colors[] = { RED, GREEN, BLUE, YELLOW, MAGENTA, CYAN, BROWN,
                          LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED, LIGHTMAGENTA, DARKGRAY };

    // ��������
    for (int i = 0; i < num_shuttles; ++i) {
        Shuttle s;
        s.id = i + 1;

        // ���ó�ʼλ�ã���1����15�ų���ھ�ȷ���룬���������������Ա�����ײ
        if (i == 0) {
            s.position_mm = pos_dev15;
        } else {
            // ���ӳ�����࣬������ײ
            double prev_shuttle_pos = shuttles_global[i - 1].position_mm;
            // ʹ�ø���ļ�ࣺ���� + ��ȫ�����3��
            double safe_distance = SHUTTLE_LENGTH_MM + MIN_INTER_SHUTTLE_DISTANCE_MM * 3.0;
            s.position_mm = normalize_track_pos(prev_shuttle_pos - safe_distance);
        }

        // ��¼��ϸ�ĳ�ʼλ����Ϣ
        std::stringstream pos_debug_ss;
        pos_debug_ss << "���� S" << s.id << " ��ʼλ������Ϊ: " << std::fixed << std::setprecision(2) << s.position_mm
                    << " mm����15�ų���ڵľ���: " << std::fixed << std::setprecision(2)
                    << distance_on_track(s.position_mm, pos_dev15) << " mm";
        add_debug_message(pos_debug_ss.str());

        // ��¼��ʼλ�ã������ж��Ƿ����һȦ
        s.initial_position_mm = s.position_mm;
        s.completed_full_circle = false;
        s.distance_traveled_mm = 0.0;

        // ��ʼ״̬�����д��󳵾�����ֹͣ״̬
        s.speed_mm_s = 0.0;
        s.current_acceleration_mm_s2 = 0.0;

        // ���ó�ʼ״̬
        if (RUN_TASK1_MODE) {
            // ����1ģʽ�����г���������Ϊ׼������״̬����������ʱ��
            s.agent_state = ShuttleAgentState::PATROLLING_ACCEL;
            // ���ø�������������ȷ������������ײ
            // ��һ����������������ĳ����ӳٸ���ʱ��
            if (i == 0) {
                s.operation_timer_s = 0.0; // ��һ������������
            } else {
                // �����㹻�������������ȷ��ǰ�����㹻ʱ������ʻ
                s.operation_timer_s = i * 10.0; // ÿ������10������
            }

            // ��ʼ�ٶ���Ϊ0�����г�����ֹͣ״̬��ʼ
            s.speed_mm_s = 0.0;

            // ���ó�ʼѲ�η��� - ��ÿ����ѡ��ͬ��Ŀ���
            std::vector<int> io_port_ids;

            // �ռ����г�����
            for(const auto& pair : devices_global) {
                if (pair.second.model_type == DeviceModelType::IN_PORT ||
                    pair.second.model_type == DeviceModelType::OUT_PORT) {
                    io_port_ids.push_back(pair.first);
                }
            }

            // ����ҵ��˳����ڣ�Ϊÿ����ѡ��ͬ��Ŀ��
            if (!io_port_ids.empty()) {
                // Ϊÿ����ѡ��ͬ��Ŀ��㣬�������г���ȥͬһ���ط�
                // ʹ�ó���ID��Ϊ������ӣ�ȷ����ͬ��ѡ��ͬĿ��
                srand(time(NULL) + s.id * 100);

                // ���ѡ��һ����������ΪĿ��
                int random_index = rand() % io_port_ids.size();
                int target_port_id = io_port_ids[random_index];

                // ������Ŀ��ľ���
                double target_dist = distance_on_track(s.position_mm, devices_global.at(target_port_id).position_on_track_mm);

                // ����Ŀ��
                s.target_pos_mm = devices_global.at(target_port_id).position_on_track_mm;
                s.has_target_pos = true;

                std::stringstream target_ss;
                target_ss << "���� S" << s.id << " ��ʼĿ������Ϊ: �豸 " << target_port_id
                         << " (" << (devices_global.at(target_port_id).model_type == DeviceModelType::IN_PORT ? "����" : "�����")
                         << ")������: " << std::fixed << std::setprecision(2) << target_dist << " mm";
                add_debug_message(target_ss.str());
            } else {
                // ���û���ҵ������ڣ�������Ŀ��
                s.has_target_pos = false;
            }

            std::stringstream debug_ss;
            debug_ss << "���� S" << s.id << " ���� " << s.operation_timer_s << " �����������ʼ�ٶ�: "
                    << std::fixed << std::setprecision(2) << s.speed_mm_s << " mm/s";
            add_debug_message(debug_ss.str());

            // ȷ�����󳵲�������ͣ��
            s.stop_count_task1 = 0;
        } else {
            // ����2ģʽ
            s.agent_state = ShuttleAgentState::IDLE_EMPTY;
            s.has_target_pos = false;
        }

        s.assigned_task_idx = -1;
        // ע�⣺��Ҫ����������has_target_pos����Ϊ���ǿ����Ѿ���������������
        s.color = colors[i % (sizeof(colors)/sizeof(COLORREF))];

        // ��ʼ������1���ͳ������
        s.total_run_time_task1_s = 0.0;
        s.stop_count_task1 = 0;
        s.movement_log_task1.clear();
        s.in_logged_accel_decel_phase = false;
        s.current_phase_start_time_s = 0.0;
        s.current_phase_start_speed_mm_s = 0.0;
        s.current_phase_acceleration_mm_s2 = 0.0;

        // ��ʼ����ײ�����ر���
        s.is_stopped_at_station = false;
        s.stopped_position_mm = 0.0;

        shuttles_global.push_back(s);

        // ��¼��ʼλ����Ϣ
        std::stringstream debug_ss;
        debug_ss << "��ʼ������ S" << s.id << " λ��: " << s.position_mm << " mm";
        add_debug_message(debug_ss.str());
    }
}

const std::string markdown_task_data = R"(
nan	1	TP001	���	16	1
nan	2	TP002	���	16	3
nan	3	TP003	���	16	5
nan	4	TP004	���	16	7
nan	5	TP005	���	16	9
nan	6	TP006	���	16	11
nan	7	TP007	���	16	1
nan	8	TP008	���	16	3
nan	9	TP009	���	16	5
nan	10	TP010	���	16	7
nan	11	TP011	���	16	9
nan	12	TP012	���	16	11
nan	13	TP013	���	16	1
nan	14	TP014	���	16	3
nan	15	TP015	���	16	5
nan	16	TP016	���	16	7
nan	17	TP017	���	16	9
nan	18	TP018	���	16	11
nan	19	TP019	���	17	1
nan	20	TP020	���	17	3
nan	21	TP021	���	17	5
nan	22	TP022	���	17	7
nan	23	TP023	���	17	9
nan	24	TP024	���	17	11
nan	25	TP025	���	17	1
nan	26	TP026	���	17	3
nan	27	TP027	���	17	5
nan	28	TP028	���	17	7
nan	29	TP029	���	17	9
nan	30	TP030	���	17	11
nan	31	TP031	���	17	1
nan	32	TP032	���	17	3
nan	33	TP033	���	17	5
nan	34	TP034	���	17	7
nan	35	TP035	���	17	9
nan	36	TP036	���	17	11
nan	37	TP037	���	18	1
nan	38	TP038	���	18	3
nan	39	TP039	���	18	5
nan	40	TP040	���	18	7
nan	41	TP041	���	18	9
nan	42	TP042	���	18	11
nan	43	TP043	���	18	1
nan	44	TP044	���	18	3
nan	45	TP045	���	18	5
nan	46	TP046	���	18	7
nan	47	TP047	���	18	9
nan	48	TP048	���	18	11
nan	49	TP049	���	18	1
nan	50	TP050	���	18	3
nan	51	TP051	���	18	5
nan	52	TP052	���	18	7
nan	53	TP053	���	18	9
nan	54	TP054	���	18	11
nan	55	TP055	����	2	13
nan	56	TP056	����	2	14
nan	57	TP057	����	2	15
nan	58	TP058	����	2	13
nan	59	TP059	����	2	14
nan	60	TP060	����	2	15
nan	61	TP061	����	2	13
nan	62	TP062	����	2	14
nan	63	TP063	����	2	15
nan	64	TP064	����	4	13
nan	65	TP065	����	4	14
nan	66	TP066	����	4	15
nan	67	TP067	����	4	13
nan	68	TP068	����	4	14
nan	69	TP069	����	4	15
nan	70	TP070	����	4	13
nan	71	TP071	����	4	14
nan	72	TP072	����	4	15
nan	73	TP073	����	6	13
nan	74	TP074	����	6	14
nan	75	TP075	����	6	15
nan	76	TP076	����	6	13
nan	77	TP077	����	6	14
nan	78	TP078	����	6	15
nan	79	TP079	����	6	13
nan	80	TP080	����	6	14
nan	81	TP081	����	6	15
nan	82	TP082	����	8	13
nan	83	TP083	����	8	14
nan	84	TP084	����	8	15
nan	85	TP085	����	8	13
nan	86	TP086	����	8	14
nan	87	TP087	����	8	15
nan	88	TP088	����	8	13
nan	89	TP089	����	8	14
nan	90	TP090	����	8	15
nan	91	TP091	����	10	13
nan	92	TP092	����	10	14
nan	93	TP093	����	10	15
nan	94	TP094	����	10	13
nan	95	TP095	����	10	14
nan	96	TP096	����	10	15
nan	97	TP097	����	10	13
nan	98	TP098	����	10	14
nan	99	TP099	����	10	15
nan	100	TP100	����	12	13
nan	101	TP101	����	12	14
nan	102	TP102	����	12	15
nan	103	TP103	����	12	13
nan	104	TP104	����	12	14
nan	105	TP105	����	12	15
nan	106	TP106	����	12	13
nan	107	TP107	����	12	14
nan	108	TP108	����	12	15
)";

void load_tasks_from_markdown_data() {
    all_tasks_global.clear();
    pending_task_queues_by_start_device.clear();

    std::stringstream ss(markdown_task_data);
    std::string line;
    int current_task_idx = 0;

    while (std::getline(ss, line)) {
        if (line.empty() || line.find("nan") == std::string::npos || line.find("������") != std::string::npos) {
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
            t.type = (task_type_str == "���") ? TaskType::INBOUND : TaskType::OUTBOUND;
            t.start_device_id = std::stoi(start_dev_str);
            t.end_device_id = std::stoi(end_dev_str);
            t.status = TaskStatus::PENDING;
            t.original_task_list_idx = current_task_idx;

            all_tasks_global.push_back(t);
            pending_task_queues_by_start_device[t.start_device_id].push_back(current_task_idx);
            current_task_idx++;
        } catch (const std::invalid_argument& ia) {
            std::cerr << "����: ������������ʱ������Ч����: " << ia.what() << " ��: " << line << std::endl;
        } catch (const std::out_of_range& oor) {
            std::cerr << "����: ������������ʱ����Խ�����: " << oor.what() << " ��: " << line << std::endl;
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

            log_device_state_change(dev.id, all_tasks_global[first_task_idx].material_id, "�޻���Ϊ�л�");
        }
    }
}


// --- Logging ---
void open_log_files() {
    // ���ÿ���̨���ΪGBK����
    SetConsoleOutputCP(936); // 936��GBK����

    std::string suffix = RUN_TASK1_MODE ? "_T1" : ("_T2_" + std::to_string(shuttles_global.size()) + "cars");
    std::string task_exe_filename = "TaskExeLog" + suffix + ".txt";
    std::string device_state_filename = "DeviceStateLog" + suffix + ".txt";

    // ɾ��֮ǰ����־�ļ�
    DeleteFile(task_exe_filename.c_str());
    DeleteFile(device_state_filename.c_str());

    // ֱ�Ӵ�����ִ����־�ļ�
    task_exe_log_file.open(task_exe_filename.c_str());

    if (task_exe_log_file.is_open()) {
        // ֱ��д�������
        task_exe_log_file << "������\t���ϱ��\t��������\t��ʼ�豸\tĿ���豸\t��ʼʱ��\t���󳵱��\tȡ�����ʱ��\t�Ż����ʱ��\t����ȡ��ʱ��\n";
    } else {
        std::cerr << "����: �޷��� " << task_exe_filename << std::endl;
    }

    // ֱ�Ӵ��豸״̬��־�ļ�
    device_state_log_file.open(device_state_filename.c_str());

    if (device_state_log_file.is_open()) {
        // ֱ��д�������
        device_state_log_file << "ʱ��\t�豸���\t������\t״̬\n";
    } else {
        std::cerr << "����: �޷��� " << device_state_filename << std::endl;
    }
}

void close_log_files() {
    if (task_exe_log_file.is_open()) task_exe_log_file.close();
    if (device_state_log_file.is_open()) device_state_log_file.close();
    if (debug_log_file.is_open()) {
        debug_log_file << "=== ������־���� ===" << std::endl;
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

    // ��¼�Ӽ�����Ϣ����־
    std::stringstream debug_ss;
    debug_ss << "���� S" << shuttle.id << " �Ӽ��ټ�¼: "
             << "��ʼʱ��=" << std::fixed << std::setprecision(2) << rec.start_time_s << "s, "
             << "����ʱ��=" << std::fixed << std::setprecision(2) << rec.end_time_s << "s, "
             << "���ٶ�=" << std::fixed << std::setprecision(2) << rec.start_speed_mm_s << "mm/s, "
             << "ĩ�ٶ�=" << std::fixed << std::setprecision(2) << rec.end_speed_mm_s << "mm/s, "
             << "���ٶ�=" << std::fixed << std::setprecision(2) << rec.acceleration_mm_s2 << "mm/s?";
    add_debug_message(debug_ss.str());

    // ����Ҫ���¼���мӼ������ݣ������˵�΢С�仯
    // ֻ�е����ٶȲ�Ϊ0���ٶȱ仯����ʱ�ż�¼
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

            // �����ͣ��״̬�������ָ�Ѳ��
            if (RUN_TASK1_MODE && shuttle.agent_state == ShuttleAgentState::PATROLLING_STOPPED) {
                shuttle.agent_state = ShuttleAgentState::PATROLLING_ACCEL;

                // ����ͣ����־
                shuttle.is_stopped_at_station = false;

                std::stringstream debug_ss;
                debug_ss << "���� S" << shuttle.id << " ͣ������������Ѳ��";
                add_debug_message(debug_ss.str());
            }

            // ������ӳ������Ĵ��󳵣���¼������Ϣ��ȷ������һ��Ŀ��
            if (RUN_TASK1_MODE && shuttle.agent_state == ShuttleAgentState::PATROLLING_ACCEL && shuttle.speed_mm_s < 1e-3) {
                std::stringstream debug_ss;
                debug_ss << "���� S" << shuttle.id << " ��ʼ����";
                add_debug_message(debug_ss.str());

                // �������û��Ŀ�꣬��������һ��Զ�뵱ǰλ�õ�Ŀ��
                if (!shuttle.has_target_pos) {
                    // Ѱ����Զ�ĳ�����
                    double max_distance = 0;
                    int farthest_port_id = -1;

                    for(const auto& pair : devices_global) {
                        if (pair.second.model_type == DeviceModelType::IN_PORT ||
                            pair.second.model_type == DeviceModelType::OUT_PORT) {
                            // �����뵱ǰλ�õľ���
                            double dist = distance_on_track(shuttle.position_mm, pair.second.position_on_track_mm);
                            if (dist > max_distance) {
                                max_distance = dist;
                                farthest_port_id = pair.first;
                            }
                        }
                    }

                    // ����ҵ�����Զ�ĳ����ڣ�������ΪĿ��
                    if (farthest_port_id != -1) {
                        shuttle.target_pos_mm = devices_global.at(farthest_port_id).position_on_track_mm;
                        shuttle.has_target_pos = true;

                        std::stringstream target_ss;
                        target_ss << "���� S" << shuttle.id << " ����ʱ����Ŀ��: �豸 " << farthest_port_id
                                << " (" << (devices_global.at(farthest_port_id).model_type == DeviceModelType::IN_PORT ? "����" : "�����")
                                << ")������: " << std::fixed << std::setprecision(2) << max_distance << " mm";
                        add_debug_message(target_ss.str());
                    }
                }
            }

            if (shuttle.agent_state == ShuttleAgentState::LOADING) {
                if (shuttle.assigned_task_idx != -1) {
                    Task& current_task = tasks[shuttle.assigned_task_idx];
                    current_task.time_pickup_complete_s = CURRENT_SIM_TIME_S;
                    log_device_state_change(shuttle.id, current_task.material_id, "�޻���Ϊ�л�", true);

                    Device& start_dev = devices.at(current_task.start_device_id);
                    start_dev.op_state = DeviceOperationalState::IDLE_EMPTY;
                    start_dev.last_state_change_time_s = CURRENT_SIM_TIME_S;
                    log_device_state_change(start_dev.id, current_task.material_id, "�л���Ϊ�޻�");
                    start_dev.current_task_idx_on_device = -1;

                    // ��ӡȡ�������Ϣ
                    std::stringstream debug_ss;
                    debug_ss << "ȡ�����: ID=" << current_task.id
                            << ", ����=" << current_task.material_id
                            << ", ����=" << shuttle.id
                            << ", �豸=" << current_task.start_device_id;
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
                    log_device_state_change(shuttle.id, current_task.material_id, "�л���Ϊ�޻�", true);

                    Device& end_dev = devices.at(current_task.end_device_id);
                    end_dev.op_state = DeviceOperationalState::HAS_GOODS_WAITING_FOR_CLEARANCE;
                    end_dev.last_state_change_time_s = CURRENT_SIM_TIME_S;
                    end_dev.current_task_idx_on_device = shuttle.assigned_task_idx;
                    log_device_state_change(end_dev.id, current_task.material_id, "�޻���Ϊ�л�");

                    // ��ӡ�Ż������Ϣ
                    std::stringstream debug_ss;
                    debug_ss << "�Ż����: ID=" << current_task.id
                            << ", ����=" << current_task.material_id
                            << ", ����=" << shuttle.id
                            << ", �豸=" << current_task.end_device_id;
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

    // ������󳵴����ӳ�����״̬���򲻽��м���
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

                    // ����ͣ��ʱ�䣬ȷ�����㹻ʱ���¼����
                    shuttle.operation_timer_s = 5.0; // �̶�5��ͣ��ʱ��
                    shuttle.stop_count_task1++;
                    shuttle.has_target_pos = false;

                    // ����ͣ����־�����������������
                    shuttle.is_stopped_at_station = true;
                    shuttle.stopped_position_mm = shuttle.position_mm;

                    // ��¼ͣ����Ϣ
                    std::stringstream debug_ss;
                    debug_ss << "���� S" << shuttle.id << " �ڳ�����ͣ�����ƻ�ͣ�� "
                            << std::fixed << std::setprecision(1) << shuttle.operation_timer_s
                            << " �룬���ǵ� " << shuttle.stop_count_task1 << " ��ͣ����λ��: "
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
               // ʹ������ٶȣ�������ٶȵ�60%-100%֮��
               double max_speed = get_max_speed_at_pos(shuttle.position_mm);
               double speed_factor = 0.6 + (static_cast<double>(rand()) / RAND_MAX) * 0.4; // 60%-100%���������
               desired_speed_mm_s = max_speed * speed_factor;
            } else if (shuttle.agent_state == ShuttleAgentState::PATROLLING_CRUISE) {
                // ʹ������ٶȣ�������ٶȵ�60%-100%֮��
                double max_speed = get_max_speed_at_pos(shuttle.position_mm);
                double speed_factor = 0.6 + (static_cast<double>(rand()) / RAND_MAX) * 0.4; // 60%-100%���������
                desired_speed_mm_s = max_speed * speed_factor;

                // ��Ѳ�ι����в����ͣ����ֻ�����һȦ��ͣ��
                // ���ﲻ��Ҫ�κ�ͣ���߼�����Ϊ�����ڼ�⵽���һȦʱ��ֱ��ͣ��
            } else {
                desired_speed_mm_s = 0;
            }
        } else {
            desired_speed_mm_s = 0;
        }
    }

    // ��ǿ����ײ���ͱ����߼�
    Shuttle* leader = nullptr;
    double dist_to_leader_rear_gap = TOTAL_TRACK_LENGTH + 1.0;

    // Ѱ��ǰ������Ĵ���
    for (size_t i = 0; i < all_shuttles.size(); ++i) {
        if (all_shuttles[i].id == shuttle.id) continue;

        // ����ǰ�����뵱ǰ��ǰ���ľ���
        double other_shuttle_rear_abs_pos = normalize_track_pos(all_shuttles[i].position_mm - SHUTTLE_LENGTH_MM / 2.0);
        double current_shuttle_front_abs_pos = normalize_track_pos(shuttle.position_mm + SHUTTLE_LENGTH_MM / 2.0);
        double current_gap = distance_on_track(current_shuttle_front_abs_pos, other_shuttle_rear_abs_pos);

        // �ҵ������ǰ��
        if (current_gap < dist_to_leader_rear_gap) {
            dist_to_leader_rear_gap = current_gap;
            leader = &all_shuttles[i];
        }
    }

    // ����ҵ���ǰ�������б��ô���
    if (leader) {
        // ������1ģʽ�£�ʹ�ø��ϸ�ĸ������룬��ֹ�����ص�
        if (RUN_TASK1_MODE) {
            // ������С��ȫ���룬��ֹ�ص�
            // ���ݳ��ٶ�̬������ȫ����
            double speed_factor = 1.0 + shuttle.speed_mm_s / 1000.0; // �ٶ�Խ�죬��ȫ����Խ��
            double min_safe_distance = MIN_INTER_SHUTTLE_DISTANCE_MM * 3.0 * speed_factor; // ������С��ȫ����
            double critical_distance = MIN_INTER_SHUTTLE_DISTANCE_MM * 1.5; // �ٽ����

            // ���ǰ��ֹͣ��ʹ�ø���İ�ȫ����
            if (leader->speed_mm_s < 1.0 || leader->agent_state == ShuttleAgentState::PATROLLING_STOPPED) {
                min_safe_distance *= 3.0; // ǰ��ֹͣʱ����200%�İ�ȫ����
                critical_distance *= 3.0; // ��������ٽ���룬ȷ�����翪ʼ����
            }

            // ��¼�������루���г�������¼��
            std::stringstream follow_debug_ss;
            follow_debug_ss << "���� S" << shuttle.id << " ��ǰ������: "
                    << std::fixed << std::setprecision(2) << dist_to_leader_rear_gap
                    << " mm��ǰ��ID: S" << leader->id
                    << "��ǰ��״̬: " << (leader->agent_state == ShuttleAgentState::PATROLLING_STOPPED ? "ֹͣ" : "�ƶ�")
                    << "��ǰ���ٶ�: " << std::fixed << std::setprecision(2) << leader->speed_mm_s
                    << " mm/s��ǰ��λ��: " << std::fixed << std::setprecision(2) << leader->position_mm
                    << " mm����ǰλ��: " << std::fixed << std::setprecision(2) << shuttle.position_mm << " mm";
            add_debug_message(follow_debug_ss.str());

            // ���ݾ�������ٶ�
            if (dist_to_leader_rear_gap < min_safe_distance) {
                // �ڰ�ȫ�����ڣ��𽥼���
                double speed_factor = dist_to_leader_rear_gap / min_safe_distance;

                // ���ǰ��ֹͣ����ǰ����Ҫ�������ؼ���ֱ��ֹͣ
                if (leader->speed_mm_s < 1.0 || leader->agent_state == ShuttleAgentState::PATROLLING_STOPPED) {
                    // ���ݾ����𽥼���ֱ��ֹͣ
                    if (dist_to_leader_rear_gap < critical_distance * 3.0) {
                        // �ڸ�Զ����Ϳ�ʼ�������
                        desired_speed_mm_s = 0;

                        // ��¼����ֹͣ��������Ϣ
                        if (shuttle.speed_mm_s > 10.0) { // ֻ���ٶȽϸ�ʱ��¼��������־����
                            std::stringstream avoid_ss;
                            avoid_ss << "���� S" << shuttle.id << " ��⵽ǰ������S" << leader->id
                                    << "��ֹͣ������: " << std::fixed << std::setprecision(2) << dist_to_leader_rear_gap
                                    << " mm��ִ�н�������";
                            add_debug_message(avoid_ss.str());
                        }
                    } else if (dist_to_leader_rear_gap < min_safe_distance) {
                        // �ڰ�ȫ�������𽥼���
                        double decel_factor = (dist_to_leader_rear_gap - critical_distance * 3.0) /
                                             (min_safe_distance - critical_distance * 3.0);
                        decel_factor = std::max(0.0, std::min(0.2, decel_factor)); // ������0-0.2֮�䣬������
                        desired_speed_mm_s = std::min(desired_speed_mm_s,
                                                     get_max_speed_at_pos(shuttle.position_mm) * decel_factor);
                    }
                } else {
                    // ǰ���ƶ��У�����ǰ���ٶȵ����ָ������
                    desired_speed_mm_s = std::min(desired_speed_mm_s, leader->speed_mm_s * speed_factor * 0.8); // ����20%�ٶ�
                }

                // ���ٽ�����ڣ��������
                if (dist_to_leader_rear_gap < critical_distance) {
                    desired_speed_mm_s = std::min(desired_speed_mm_s, std::max(0.0, leader->speed_mm_s - DECEL_MM_PER_S2 * SIM_TIME_STEP_S * 2.0));

                    // ��¼�ӽ�����
                    std::stringstream warning_ss;
                    warning_ss << "����: ���� S" << shuttle.id << " ��ǰ������ӽ��ٽ�ֵ ("
                            << std::fixed << std::setprecision(2) << dist_to_leader_rear_gap
                            << " mm)���������";
                    add_debug_message(warning_ss.str());

                    // �ڼ��Ƚӽ�ʱֹͣ
                    if (dist_to_leader_rear_gap < critical_distance * 0.5 && shuttle.speed_mm_s > 0) {
                        desired_speed_mm_s = 0;

                        // ��¼��ײ����
                        std::stringstream critical_ss;
                        critical_ss << "���ؾ���: ���� S" << shuttle.id << " ��ǰ��������� ("
                                << std::fixed << std::setprecision(2) << dist_to_leader_rear_gap
                                << " mm)������ͣ��";
                        add_debug_message(critical_ss.str());
                    }
                }
            }
        }
        // ����2ģʽ��ʹ��ԭ�еĸ��ϸ�ĸ����߼�
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

    // �����ƶ�����
    double distance_moved = shuttle.speed_mm_s * SIM_TIME_STEP_S;

    // ����λ��
    double old_position = shuttle.position_mm;
    shuttle.position_mm += distance_moved;
    shuttle.position_mm = normalize_track_pos(shuttle.position_mm);

    // �ۼ���ʻ����
    shuttle.distance_traveled_mm += distance_moved;

    // ����Ƿ����һȦ������һģʽ��
    if (RUN_TASK1_MODE && !shuttle.completed_full_circle) {
        // ������󳵾����򵽴��ʼλ�ã����Ѿ���ʻ���㹻�ľ��루����һȦ��
        if (shuttle.distance_traveled_mm >= TOTAL_TRACK_LENGTH) {
            // ����Ƿ񾭹��˳�ʼλ��
            bool crossed_initial_pos = false;

            // ���λ�÷�����"����"���ӹ��ĩ�˵���ʼ������Ҫ���⴦��
            if (old_position > shuttle.position_mm && old_position > TOTAL_TRACK_LENGTH * 0.9 && shuttle.position_mm < TOTAL_TRACK_LENGTH * 0.1) {
                // ����ʼλ���Ƿ����������������
                crossed_initial_pos = (shuttle.initial_position_mm > old_position || shuttle.initial_position_mm < shuttle.position_mm);
            } else {
                // �������������Ƿ񾭹��˳�ʼλ��
                crossed_initial_pos = (old_position <= shuttle.initial_position_mm && shuttle.position_mm >= shuttle.initial_position_mm) ||
                                     (old_position >= shuttle.initial_position_mm && shuttle.position_mm <= shuttle.initial_position_mm);
            }

            if (crossed_initial_pos) {
                shuttle.completed_full_circle = true;
                std::stringstream debug_ss;
                debug_ss << "���� S" << shuttle.id << " �����һ��ȦѲ�Σ�����ʻ����: "
                        << std::fixed << std::setprecision(2) << shuttle.distance_traveled_mm << " mm"
                        << "����ʼλ��: " << std::fixed << std::setprecision(2) << shuttle.initial_position_mm << " mm"
                        << "����ǰλ��: " << std::fixed << std::setprecision(2) << shuttle.position_mm << " mm"
                        << "��ͣ������: " << shuttle.stop_count_task1;
                add_debug_message(debug_ss.str());

                // ���һȦ������ͣ��
                if (shuttle.stop_count_task1 == 0) {
                    // ֱ���ڵ�ǰλ��ͣ��
                    shuttle.agent_state = ShuttleAgentState::PATROLLING_STOPPED;
                    shuttle.operation_timer_s = 999999.0; // ����һ���ܴ��ֵ��ʵ����������ֹͣ
                    shuttle.stop_count_task1++;
                    shuttle.has_target_pos = false;
                    shuttle.is_stopped_at_station = true;
                    shuttle.stopped_position_mm = shuttle.position_mm;

                    std::stringstream stop_ss;
                    stop_ss << "���� S" << shuttle.id << " �����һȦѲ�Σ�ֹͣ���С�����ʻ����: "
                           << std::fixed << std::setprecision(2) << shuttle.distance_traveled_mm << " mm";
                    add_debug_message(stop_ss.str());

                    // ��¼ͣ����Ϣ����־
                    std::stringstream log_ss;
                    log_ss << "���� S" << shuttle.id << " �������ֹͣ���С�"
                          << "������ʱ��: " << std::fixed << std::setprecision(2) << shuttle.total_run_time_task1_s << " ��, "
                          << "����ʻ����: " << std::fixed << std::setprecision(2) << shuttle.distance_traveled_mm / 1000.0 << " m";
                    add_debug_message(log_ss.str());
                }

                // ������д����Ƿ������һȦ
                bool all_completed = true;
                for (const auto& s : shuttles_global) {
                    if (!s.completed_full_circle) {
                        all_completed = false;
                        break;
                    }
                }

                if (all_completed) {
                    add_debug_message("���д��������һ��ȦѲ�Σ�");
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
                        log_device_state_change(device.id, material_id, "�޻���Ϊ�л�");

                        // ��ӡ������Ϣ
                        std::stringstream debug_ss;
                        debug_ss << "����׼�����: ID=" << task_id
                                << ", ����=" << material_id
                                << ", �豸=" << device.id;
                        add_debug_message(debug_ss.str());
                    } else {
                        device.op_state = DeviceOperationalState::IDLE_EMPTY;
                        device.current_task_idx_on_device = -1;
                    }
                } else {
                    device.op_state = DeviceOperationalState::IDLE_EMPTY;
                    log_device_state_change(device.id, "N/A", "��������(����Ч��������)ת����");
                    device.current_task_idx_on_device = -1;
                }
            } else if (device.op_state == DeviceOperationalState::BUSY_CLEARING_GOODS) {
                if (device.current_task_idx_on_device != -1 && device.current_task_idx_on_device < (int)tasks.size()) {
                    if (tasks[device.current_task_idx_on_device].status == TaskStatus::GOODS_AT_DEST_AWAITING_REMOVAL) {
                        // �������Ϊ���״̬
                        int task_id = tasks[device.current_task_idx_on_device].id;
                        std::string material_id = tasks[device.current_task_idx_on_device].material_id;

                        tasks[device.current_task_idx_on_device].status = TaskStatus::COMPLETED;
                        tasks[device.current_task_idx_on_device].time_goods_taken_from_dest_s = CURRENT_SIM_TIME_S;
                        log_task_event_completion(tasks[device.current_task_idx_on_device]);

                        device.op_state = DeviceOperationalState::IDLE_EMPTY;
                        log_device_state_change(device.id, material_id, "�л���Ϊ�޻�");

                        // ��ӡ������Ϣ
                        std::stringstream debug_ss;
                        debug_ss << "�������: ID=" << task_id
                                << ", ����=" << material_id
                                << ", �豸=" << device.id;
                        add_debug_message(debug_ss.str());

                        device.current_task_idx_on_device = -1;
                    } else if (tasks[device.current_task_idx_on_device].status == TaskStatus::COMPLETED) {
                        // �����Ѿ���ɣ������ظ�����
                        std::stringstream debug_ss;
                        debug_ss << "����: ���� " << device.current_task_idx_on_device << " �Ѿ������Ϊ��ɣ������ظ�����";
                        add_debug_message(debug_ss.str());
                        device.op_state = DeviceOperationalState::IDLE_EMPTY;
                        device.current_task_idx_on_device = -1;
                    } else {
                        device.op_state = DeviceOperationalState::IDLE_EMPTY;
                    }
                } else {
                    device.op_state = DeviceOperationalState::IDLE_EMPTY;
                    log_device_state_change(device.id, "N/A", "�������(����Ч��������)ת����");
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
                        // ��������ɣ��Ӷ������Ƴ�
                        std::stringstream debug_ss;
                        debug_ss << "���� " << task_to_prepare.id << " ����ɣ��Ӷ������Ƴ�";
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

            // ��ӵ�����Ϣ����¼���д��󳵵�״̬
            std::stringstream idle_debug_ss;
            idle_debug_ss << "���д���: S" << shuttle.id
                         << ", λ��=" << std::fixed << std::setprecision(2) << shuttle.position_mm
                         << "mm, ��ʼѰ������";
            add_debug_message(idle_debug_ss.str());

            for (int task_idx = 0; task_idx < (int)tasks.size(); ++task_idx) {
                Task& task = tasks[task_idx];
                // ֻ����״̬ΪREADY_FOR_PICKUP������
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

                        // ��ӵ�����Ϣ����¼�ҵ��Ŀ�������
                        std::stringstream task_debug_ss;
                        task_debug_ss << "�ҵ���������: ID=" << task.id
                                     << ", ����=" << task.material_id
                                     << ", �豸=" << task.start_device_id
                                     << ", ����=" << std::fixed << std::setprecision(2) << dist << "mm";
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

                // ��ӡ���������Ϣ
                std::stringstream debug_ss;
                debug_ss << "�������: ID=" << assigned_task.id
                        << ", ����=" << assigned_task.material_id
                        << ", ����=" << shuttle.id
                        << ", ��ʼ�豸=" << assigned_task.start_device_id
                        << ", Ŀ���豸=" << assigned_task.end_device_id;
                add_debug_message(debug_ss.str());
            } else {
                // ���û���ҵ�������ӵ�����Ϣ
                std::stringstream no_task_debug_ss;
                no_task_debug_ss << "���� S" << shuttle.id << " û���ҵ��������񣬱��ֿ���״̬";
                add_debug_message(no_task_debug_ss.str());

                // Ϊ�˷�ֹ���󳵿�ס�������п��д�������ƶ���һ���豸λ��
                if (!shuttle.has_target_pos) {  // �������п��д���
                    std::vector<int> device_ids;
                    for (const auto& pair : devices) {
                        device_ids.push_back(pair.first);
                    }

                    if (!device_ids.empty()) {
                        int random_device_id = device_ids[rand() % device_ids.size()];
                        shuttle.target_pos_mm = devices.at(random_device_id).position_on_track_mm;
                        shuttle.has_target_pos = true;

                        std::stringstream move_debug_ss;
                        move_debug_ss << "���д��� S" << shuttle.id << " �ƶ����豸 " << random_device_id
                                     << " λ�ã���ֹ��ס";
                        add_debug_message(move_debug_ss.str());
                    }
                }
            }
        }
    }
}


// --- Drawing Functions ---
void draw_track() {
    // ����������ɫ����ʽ
    const COLORREF TRACK_COLOR = RGB(120, 120, 120);       // ���������ɫ
    const COLORREF TRACK_HIGHLIGHT = RGB(180, 180, 180);   // ���������ɫ
    const COLORREF TRACK_SHADOW = RGB(80, 80, 80);         // �����Ӱ��ɫ
    const COLORREF TRACK_MARKER = RGB(200, 200, 200);      // ��������ɫ

    // ���ƹ����Ӱ
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

    // ���ƹ������
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

    // ���ƹ������
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

    // ���ƹ����ǵ�
    setfillcolor(TRACK_MARKER);

    // ��ֱ�߶κ������ε����Ӵ����Ʊ�ǵ�
    double straight_end = TRACK_STRAIGHT_LENGTH;
    double curve_end = TRACK_STRAIGHT_LENGTH + TRACK_CURVE_LENGTH;
    double straight2_end = TRACK_STRAIGHT_LENGTH * 2 + TRACK_CURVE_LENGTH;
    double curve2_end = TRACK_STRAIGHT_LENGTH * 2 + TRACK_CURVE_LENGTH * 2;

    int mark_x, mark_y;

    // ���ֱ�߶κ������ε����Ӵ�
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
    // �����豸��ʾ����ɫ����ʽ
    const COLORREF DEV_BORDER_COLOR = RGB(100, 100, 100);  // �豸�߿���ɫ
    const COLORREF DEV_IDLE_COLOR = RGB(80, 80, 80);        // �����豸��ɫ
    const COLORREF DEV_WAITING_COLOR = RGB(220, 180, 50);   // �ȴ�״̬��ɫ
    const COLORREF DEV_BUSY_COLOR = RGB(70, 130, 180);      // æµ״̬��ɫ
    const COLORREF DEV_ACTIVE_COLOR = RGB(220, 120, 50);    // �״̬��ɫ
    const COLORREF DEV_TEXT_DARK = RGB(30, 30, 30);         // ��ɫ�ı�
    const COLORREF DEV_TEXT_LIGHT = RGB(240, 240, 240);     // ǳɫ�ı�
    const COLORREF DEV_STATUS_PENDING = RGB(180, 180, 180); // ������״̬
    const COLORREF DEV_STATUS_READY = RGB(100, 200, 100);   // ����״̬
    const COLORREF DEV_STATUS_ASSIGNED = RGB(100, 150, 250);// �ѷ���״̬
    const COLORREF DEV_STATUS_COMPLETED = RGB(50, 200, 120);// �����״̬

    for (const auto& pair : devices_map) {
        const Device& dev = pair.second;
        int sx, sy;
        map_track_pos_to_screen_xy(dev.position_on_track_mm, sx, sy);

        COLORREF device_color = DEV_IDLE_COLOR;
        COLORREF text_color = DEV_TEXT_LIGHT;
        bool has_goods_visual = false;
        int device_width = 18;
        int device_height = 10;

        // �����豸״̬������ɫ
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

        // �����豸��ɫ
        setfillcolor(device_color);
        solidrectangle(sx - device_width, sy - device_height, sx + device_width, sy + device_height);

        // �����豸�߿�
        setlinecolor(DEV_BORDER_COLOR);
        rectangle(sx - device_width, sy - device_height, sx + device_width, sy + device_height);

        // �����豸ID
        settextcolor(text_color);
        setbkmode(TRANSPARENT);
        settextstyle(12, 0, "΢���ź�");

        char dev_id_str[5];
        snprintf(dev_id_str, sizeof(dev_id_str), "%d", dev.id);
        int text_width = textwidth(dev_id_str);
        outtextxy(sx - text_width/2, sy - 6, dev_id_str);

        // ����豸�л����ʾ������Ϣ
        if (has_goods_visual && dev.current_task_idx_on_device != -1 &&
            dev.current_task_idx_on_device < (int)tasks_vec.size()) {

            // ������Ϣ������
            setfillcolor(RGB(240, 240, 240));
            solidrectangle(sx - 25, sy + device_height + 2, sx + 25, sy + device_height + 32);
            setlinecolor(RGB(150, 150, 150));
            rectangle(sx - 25, sy + device_height + 2, sx + 25, sy + device_height + 32);

            // ��ʾ���ϱ��
            settextcolor(DEV_TEXT_DARK);
            settextstyle(10, 0, "΢���ź�");
            std::string material_info = tasks_vec[dev.current_task_idx_on_device].material_id;
            int material_width = textwidth(material_info.c_str());
            outtextxy(sx - material_width/2, sy + device_height + 4, material_info.c_str());

            // ��ʾ����״̬
            std::string status_info;
            COLORREF status_color;

            switch(tasks_vec[dev.current_task_idx_on_device].status) {
                case TaskStatus::PENDING:
                    status_info = "������";
                    status_color = DEV_STATUS_PENDING;
                    break;
                case TaskStatus::DEVICE_PREPARING:
                    status_info = "׼����";
                    status_color = DEV_STATUS_PENDING;
                    break;
                case TaskStatus::READY_FOR_PICKUP:
                    status_info = "��ȡ��";
                    status_color = DEV_STATUS_READY;
                    break;
                case TaskStatus::ASSIGNED_TO_SHUTTLE:
                    status_info = "�ѷ���";
                    status_color = DEV_STATUS_ASSIGNED;
                    break;
                case TaskStatus::GOODS_AT_DEST_AWAITING_REMOVAL:
                    status_info = "������";
                    status_color = DEV_STATUS_READY;
                    break;
                case TaskStatus::COMPLETED:
                    status_info = "���:" +
                        std::to_string((int)tasks_vec[dev.current_task_idx_on_device].time_goods_taken_from_dest_s) + "s";
                    status_color = DEV_STATUS_COMPLETED;
                    break;
                default:
                    status_info = "δ֪";
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

        // ���ƴ�����ӰЧ��
        setfillcolor(RGB(50, 50, 50));
        solidcircle(sx + 2, sy + 2, 10);

        // ���ƴ��󳵵�ɫ
        setfillcolor(shuttle.color);
        solidcircle(sx, sy, 10);

        // ���ƴ��󳵱߿�
        setlinecolor(RGB(50, 50, 50));
        circle(sx, sy, 10);

        // ���ƴ���ID
        settextcolor(WHITE);
        setbkmode(TRANSPARENT);
        settextstyle(12, 0, "΢���ź�");
        char shuttle_id_str[5];
        snprintf(shuttle_id_str, sizeof(shuttle_id_str), "S%d", shuttle.id);
        int text_width = textwidth(shuttle_id_str);
        outtextxy(sx - text_width/2, sy - 6, shuttle_id_str);

        // ����ģʽ��ʾ��ͬ����Ϣ
        if (!RUN_TASK1_MODE) {
            // ����״̬��Ϣ����
            std::string status_text = get_shuttle_state_str_cn(shuttle.agent_state);
            if (shuttle.assigned_task_idx != -1 && shuttle.assigned_task_idx < (int)tasks_vec.size()) {
                const Task& t = tasks_vec[shuttle.assigned_task_idx];
                status_text += ":T" + std::to_string(t.id);
            }

            int status_width = textwidth(status_text.c_str());
            setfillcolor(RGB(240, 240, 240));
            solidroundrect(sx + 12, sy - 14, sx + 12 + status_width + 6, sy - 14 + 16, 3, 3);

            // ����״̬�ı�
            settextcolor(shuttle.color);
            settextstyle(10, 0, "΢���ź�");
            outtextxy(sx + 15, sy - 12, status_text.c_str());

            // ��������л����ʾ������
            if (shuttle.has_goods()) {
                // ���ƻ����Ǳ���
                setfillcolor(RGB(0, 120, 0));
                solidroundrect(sx - 20, sy + 12, sx + 20, sy + 25, 3, 3);

                // ���ƻ������ı�
                settextcolor(WHITE);
                settextstyle(10, 0, "΢���ź�");
                std::string goods_text = "�ػ���";
                int goods_width = textwidth(goods_text.c_str());
                outtextxy(sx - goods_width/2, sy + 14, goods_text.c_str());

                // �ڴ������Ļ��ƻ���ָʾ
                setfillcolor(RGB(0, 150, 0));
                solidcircle(sx, sy, 5);
            }
        } else {
            // ����1ģʽ�µ�״̬��ʾ
            std::string status_text = get_shuttle_state_str_cn(shuttle.agent_state);
            int status_width = textwidth(status_text.c_str());

            setfillcolor(RGB(240, 240, 240));
            solidroundrect(sx + 12, sy - 14, sx + 12 + status_width + 6, sy - 14 + 16, 3, 3);

            settextcolor(shuttle.color);
            settextstyle(10, 0, "΢���ź�");
            outtextxy(sx + 15, sy - 12, status_text.c_str());
        }

        // ���������Ŀ��λ�ã�����Ŀ��ָʾ��
        if (shuttle.has_target_pos) {
            int target_x, target_y;
            map_track_pos_to_screen_xy(shuttle.target_pos_mm, target_x, target_y);

            // ��������ָʾ
            setlinestyle(PS_DOT, 1);
            setlinecolor(RGB(200, 200, 200));
            line(sx, sy, target_x, target_y);
            setlinestyle(PS_SOLID, 1);

            // ����Ŀ���
            setfillcolor(RGB(255, 200, 100));
            solidcircle(target_x, target_y, 3);
        }
    }
}


void draw_ui_info(int num_shuttles) {
    // ����UI�������ɫ����ʽ
    const COLORREF TITLE_COLOR = RGB(255, 255, 200);  // ǳ��ɫ����
    const COLORREF TEXT_COLOR = RGB(220, 220, 220);   // ǳ��ɫ�ı�
    const COLORREF HIGHLIGHT_COLOR = RGB(120, 230, 180); // ������ɫ
    const COLORREF WARNING_COLOR = RGB(255, 180, 120);   // �����ɫ
    const COLORREF SECTION_BG_COLOR = RGB(60, 60, 70);   // ���ɫ����
    const COLORREF HEADER_BG_COLOR = RGB(40, 40, 50);    // ����Ļ�ɫ���ⱳ��
    const COLORREF LOG_INFO_COLOR = RGB(255, 220, 100);  // ��־��Ϣ��ɫ

    int y_pos = 10;
    char buffer[256];

    // ����͸������ģʽ
    setbkmode(TRANSPARENT);

    // ===== ������Ϣ���� =====
    // ���Ʊ��ⱳ��
    setfillcolor(HEADER_BG_COLOR);
    solidrectangle(5, 5, SCREEN_WIDTH-5, 35);

    // ���Ʊ���
    settextcolor(TITLE_COLOR);
    settextstyle(18, 0, "΢���ź�");
    outtextxy(15, y_pos, "���ִܲ����ι������ϵͳ����");

    // ���Ʒ�����Ϣ
    settextcolor(TEXT_COLOR);
    settextstyle(14, 0, "΢���ź�");
    snprintf(buffer, sizeof(buffer), "����ʱ��: %.2f ��  |  ����: %.2fx  |  ����: %d  |  ģʽ: %s",
             CURRENT_SIM_TIME_S, SIM_SPEED_MULTIPLIER, num_shuttles,
             RUN_TASK1_MODE ? "����1(Ѳ��)" : "����2(����)");
    outtextxy(SCREEN_WIDTH - 450, y_pos + 2, buffer);
    y_pos += 35;

    // ���ƿ�����Ϣ����
    setfillcolor(SECTION_BG_COLOR);
    solidrectangle(5, y_pos, SCREEN_WIDTH-5, y_pos+25);

    // ���ƿ�����Ϣ
    settextcolor(TEXT_COLOR);
    settextstyle(14, 0, "΢���ź�");
    snprintf(buffer, sizeof(buffer), "����: P-��ͣ/����, +/- ��������, M-�л�ģʽ(�����¾�), Q-�˳�����");
    outtextxy(15, y_pos+4, buffer);
    y_pos += 30;

    // ===== ����ͳ������ =====
    if (!RUN_TASK1_MODE && !all_tasks_global.empty()) {
        // ��������ͳ��
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

        // ��������ͳ�Ʊ���
        setfillcolor(SECTION_BG_COLOR);
        solidrectangle(5, y_pos, SCREEN_WIDTH-5, y_pos+25);

        // ��������ͳ�Ʊ���
        settextcolor(HIGHLIGHT_COLOR);
        settextstyle(14, 0, "΢���ź�");
        outtextxy(15, y_pos+4, "����ͳ��:");

        // ��������ͳ����Ϣ
        settextcolor(TEXT_COLOR);
        snprintf(buffer, sizeof(buffer), "����: %d | �����: %d | ������: %d | ��׼��: %d | ��ʰȡ: %d",
                 (int)all_tasks_global.size(), tasks_completed, tasks_in_progress,
                 tasks_pending_preparation, tasks_ready_for_pickup);
        outtextxy(100, y_pos+4, buffer);
        y_pos += 30;
    }

    // ===== ����״̬���� =====
    // ���ƴ���״̬����
    setfillcolor(SECTION_BG_COLOR);
    int shuttle_section_start = y_pos;
    int shuttle_section_height = 25;
    if (shuttles_global.size() > 4) {
        shuttle_section_height = 45; // ������󳵽϶࣬��������߶�
    }
    solidrectangle(5, y_pos, SCREEN_WIDTH-5, y_pos+shuttle_section_height);

    // ���ƴ���״̬����
    settextcolor(HIGHLIGHT_COLOR);
    settextstyle(14, 0, "΢���ź�");
    outtextxy(15, y_pos+4, "����״̬:");

    // ���ƴ���״̬��Ϣ
    int x_shuttle_info = 120;
    settextcolor(TEXT_COLOR);
    settextstyle(14, 0, "΢���ź�");

    for(size_t i = 0; i < shuttles_global.size(); ++i) {
        const auto& s = shuttles_global[i];
        std::string task_str_suffix = "";

        if (!RUN_TASK1_MODE && s.assigned_task_idx != -1 && s.assigned_task_idx < (int)all_tasks_global.size()) {
            task_str_suffix = " (T" + std::to_string(all_tasks_global[s.assigned_task_idx].id);
            if(s.has_goods()) task_str_suffix += "[��]";
            task_str_suffix += ")";
        }

        // ���ô�����ɫ
        settextcolor(s.color);

        std::string shuttle_display_str = "S" + std::to_string(s.id) + ": " + get_shuttle_state_str_cn(s.agent_state) + task_str_suffix;
        outtextxy(x_shuttle_info, y_pos+4, shuttle_display_str.c_str());

        SIZE text_size;
        GetTextExtentPoint32A(GetImageHDC(), shuttle_display_str.c_str(), shuttle_display_str.length(), &text_size);
        x_shuttle_info += text_size.cx + 25; // ���Ӽ��

        // ���������Ļ��ȣ�������ʾ
        if (x_shuttle_info > SCREEN_WIDTH - 150 && i < shuttles_global.size() - 1) {
            x_shuttle_info = 120;
            y_pos += 20;
        }
    }

    y_pos = shuttle_section_start + shuttle_section_height + 5;

    // ===== ������Ϣ���� =====
    if (!debug_messages.empty()) {
        // ���Ƶ�����Ϣ����
        setfillcolor(RGB(30, 40, 50)); // ����ɫ����
        solidrectangle(5, y_pos, SCREEN_WIDTH-5, y_pos+160); // �̶��߶ȵĵ�������

        // ���Ƶ�����Ϣ����ͱ߿�
        setlinecolor(RGB(100, 120, 150));
        rectangle(5, y_pos, SCREEN_WIDTH-5, y_pos+160);

        settextcolor(TITLE_COLOR);
        settextstyle(16, 0, "΢���ź�");
        outtextxy(15, y_pos+5, "������Ϣ:");

        // ��ʾ��־�ļ���Ϣ
        settextcolor(LOG_INFO_COLOR);
        settextstyle(12, 0, "΢���ź�");
        std::string log_info = "������־�ļ�: " + current_log_filename;
        outtextxy(200, y_pos+5, log_info.c_str());

        y_pos += 25;

        // ���Ƶ�����Ϣ����
        settextstyle(12, 0, "����");

        // �����µ���Ϣ��ʼ��ʾ
        int max_messages_to_show = std::min(10, (int)debug_messages.size());
        for (int i = debug_messages.size() - 1; i >= (int)debug_messages.size() - max_messages_to_show; i--) {
            if (i >= 0) {
                // ������Ϣ�������ò�ͬ��ɫ
                if (debug_messages[i].find("����") != std::string::npos) {
                    settextcolor(WARNING_COLOR); // ������Ϣ�ó�ɫ
                } else if (debug_messages[i].find("���") != std::string::npos) {
                    settextcolor(HIGHLIGHT_COLOR); // �����Ϣ����ɫ
                } else {
                    settextcolor(TEXT_COLOR); // ��ͨ��Ϣ�ð�ɫ
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

    // ׼���ܽ���Ϣ
    std::stringstream summary_info;
    summary_info << "\n\n==================================================" << std::endl;
    summary_info << "            ����1 (Ѳ��) �ܽ�" << std::endl;
    summary_info << "==================================================" << std::endl;
    summary_info << "������ʱ��: " << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << " ��" << std::endl;
    summary_info << "��������: " << shuttles_global.size() << std::endl;
    summary_info << "���󳵲���:" << std::endl;
    summary_info << "  ֱ�߶�����ٶ�: " << std::fixed << std::setprecision(2) << MAX_SPEED_STRAIGHT_MM_PER_S << " mm/s ("
                 << MAX_SPEED_STRAIGHT_MM_PER_S * 60.0 / 1000.0 << " m/min)" << std::endl;
    summary_info << "  ���������ٶ�: " << std::fixed << std::setprecision(2) << MAX_SPEED_CURVE_MM_PER_S << " mm/s ("
                 << MAX_SPEED_CURVE_MM_PER_S * 60.0 / 1000.0 << " m/min)" << std::endl;
    summary_info << "  ���ٶ�: " << std::fixed << std::setprecision(2) << ACCEL_MM_PER_S2 << " mm/s? ("
                 << ACCEL_MM_PER_S2 / 1000.0 << " m/s?)" << std::endl;
    summary_info << "  ���ٶ�: " << std::fixed << std::setprecision(2) << DECEL_MM_PER_S2 << " mm/s? ("
                 << DECEL_MM_PER_S2 / 1000.0 << " m/s?)" << std::endl;

    std::cout << summary_info.str();

    // ��������1�ܽ��ļ�
    std::ofstream summary_file("Task1Summary.txt");
    if (summary_file.is_open()) {
        summary_file << "--- ����1 (Ѳ��) �ܽ� ---" << std::endl;
        summary_file << "������ʱ��: " << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << " ��" << std::endl;
        summary_file << "��������: " << shuttles_global.size() << std::endl;
        summary_file << "���󳵲���:" << std::endl;
        summary_file << "  ֱ�߶�����ٶ�: " << std::fixed << std::setprecision(2) << MAX_SPEED_STRAIGHT_MM_PER_S << " mm/s ("
                     << MAX_SPEED_STRAIGHT_MM_PER_S * 60.0 / 1000.0 << " m/min)" << std::endl;
        summary_file << "  ���������ٶ�: " << std::fixed << std::setprecision(2) << MAX_SPEED_CURVE_MM_PER_S << " mm/s ("
                     << MAX_SPEED_CURVE_MM_PER_S * 60.0 / 1000.0 << " m/min)" << std::endl;
        summary_file << "  ���ٶ�: " << std::fixed << std::setprecision(2) << ACCEL_MM_PER_S2 << " mm/s? ("
                     << ACCEL_MM_PER_S2 / 1000.0 << " m/s?)" << std::endl;
        summary_file << "  ���ٶ�: " << std::fixed << std::setprecision(2) << DECEL_MM_PER_S2 << " mm/s? ("
                     << DECEL_MM_PER_S2 / 1000.0 << " m/s?)" << std::endl;
    }

    for (const auto& shuttle : shuttles_global) {
        std::stringstream shuttle_info;
        shuttle_info << "\n���� " << shuttle.id << ":" << std::endl;
        shuttle_info << "  ������ʱ��: " << std::fixed << std::setprecision(2) << shuttle.total_run_time_task1_s << " ��" << std::endl;
        shuttle_info << "  ͣ������: " << shuttle.stop_count_task1 << std::endl;
        shuttle_info << "  ����ʻ����: " << std::fixed << std::setprecision(2) << shuttle.distance_traveled_mm << " mm ("
                     << std::fixed << std::setprecision(2) << shuttle.distance_traveled_mm / 1000.0 << " m)" << std::endl;
        shuttle_info << "  �Ƿ����һ��ȦѲ��: " << (shuttle.completed_full_circle ? "��" : "��") << std::endl;
        shuttle_info << "  �Ӽ��ټ�¼:" << std::endl;

        if (shuttle.movement_log_task1.empty()) {
            shuttle_info << "    �޼Ӽ��ټ�¼��" << std::endl;
        }

        for (const auto& rec : shuttle.movement_log_task1) {
            shuttle_info << "    �Ӽ��ټ�¼ #" << (&rec - &shuttle.movement_log_task1[0] + 1) << ":" << std::endl;
            shuttle_info << "      ��ʼʱ��: " << std::fixed << std::setprecision(2) << rec.start_time_s << " ��" << std::endl;
            shuttle_info << "      ��ʼ�ٶ�: " << std::fixed << std::setprecision(2) << rec.start_speed_mm_s << " mm/s" << std::endl;
            shuttle_info << "      ��ʼ���ٶ�: " << std::fixed << std::setprecision(2) << rec.acceleration_mm_s2 << " mm/s?" << std::endl;
            shuttle_info << "      ��ֹʱ��: " << std::fixed << std::setprecision(2) << rec.end_time_s << " ��" << std::endl;
            shuttle_info << "      ��ֹ�ٶ�: " << std::fixed << std::setprecision(2) << rec.end_speed_mm_s << " mm/s" << std::endl;
            shuttle_info << "      ��ֹ���ٶ�: " << std::fixed << std::setprecision(2) << rec.acceleration_mm_s2 << " mm/s?" << std::endl;
        }

        // ���������̨
        std::cout << shuttle_info.str();

        // д�뵽�ļ�
        if (summary_file.is_open()) {
            summary_file << shuttle_info.str();
        }
    }

    if (summary_file.is_open()) {
        summary_file.close();
        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << "����1�ܽ���д�� Task1Summary.txt" << std::endl;
    }
}

void output_task2_summary_files() {
    if (RUN_TASK1_MODE) return;
    if (shuttles_global.empty() && !RUN_TASK1_MODE) {
        std::cout << "����2�ܽ᣺�޴������У��������ܽ��ļ���" << std::endl;
        return;
    }
    if (all_tasks_global.empty() && !RUN_TASK1_MODE) {
        std::cout << "����2�ܽ᣺�������б��������ܽ��ļ���" << std::endl;
        return;
    }


    std::string suffix = "_T2_" + std::to_string(shuttles_global.size()) + "cars";
    std::ofstream summary_file("SimulationSummary" + suffix + ".txt");

    if (!summary_file.is_open()) {
        std::cerr << "����: �޷��� SimulationSummary" << suffix << ".txt" << std::endl;
        return;
    }

    // ׼���ܽ���Ϣ
    std::stringstream summary_info;
    summary_info << "\n\n==================================================" << std::endl;
    summary_info << "            �����ܽ� (���� 2 - " << shuttles_global.size() << "̨��)" << std::endl;
    summary_info << "==================================================" << std::endl;
    summary_info << "������������ܺ�ʱ: " << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << " ��" << std::endl;
    summary_info << "��������: " << shuttles_global.size() << std::endl;
    summary_info << "\n�豸ͳ������:" << std::endl;
    summary_info << "�豸ID\t�豸����\t�ܿ���ʱ��(��)\t���л�ʱ��(��)" << std::endl;
    summary_info << "--------------------------------------------------" << std::endl;

    // д�뵽�ļ�
    summary_file << "--- �����ܽ� (���� 2) ---" << std::endl;
    summary_file << "������������ܺ�ʱ: " << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << " ��" << std::endl;
    summary_file << "��������: " << shuttles_global.size() << std::endl;
    summary_file << "\n�豸ͳ������:\n";
    summary_file << "�豸ID\t�豸����\t�ܿ���ʱ��(��)\t���л�ʱ��(��)\n";

    // ͬʱ���������̨
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

        // д�뵽�ļ�
        summary_file << dev.id << "\t" << dev.name << "\t"
                     << std::fixed << std::setprecision(2) << dev.total_idle_time_s << "\t"
                     << std::fixed << std::setprecision(2) << dev.total_has_goods_time_s << "\n";

        // ͬʱ���������̨
        std::cout << dev.id << "\t" << dev.name << "\t"
                  << std::fixed << std::setprecision(2) << dev.total_idle_time_s << "\t"
                  << std::fixed << std::setprecision(2) << dev.total_has_goods_time_s << std::endl;
    }

    summary_file.close();
    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "����2�ܽ���д�� SimulationSummary" << suffix << ".txt" << std::endl;

    std::cout << "\nϵͳ���ָĽ����� (һ���Խ��飬�����������־ TaskExeLog.txt �� DeviceStateLog.txt):" << std::endl;
    std::cout << "1. ƿ������: ���DeviceStateLog.txt�и��豸���ر��ǽӿ��豸�ͳ����ڣ��� total_has_goods_time_s �� total_idle_time_s��"
              << "��ʱ�䴦�� HAS_GOODS_WAITING_FOR_CLEARANCE �� HAS_GOODS_WAITING_FOR_SHUTTLE ���ܱ�ʾƿ����" << std::endl;
    std::cout << "2. �������: �۲�TaskExeLog.txt�и����󳵵ķ�æ�̶ȡ�������ֳ��������ض��������У�������Ҫ�Ż������㷨��" << std::endl;
    std::cout << "3. ������λ��: ����ͼ2, ������(13-18)��λ���Ѱ�ͼʾ�趨������־��ʾ����С���г̼����ڹ��ĳ�ض�����������Щ�̶��ڣ�"
              << "���������������Щ���ڵײ�����ϵ����λ�ã����ı��������40000mm�ܳ����������ƽ���ҵ������������ȷֲ����ܼ���С��ƽ���г̡�"
              << "����ǰʵ�����ϸ���ͼ2�ߴ硣" << std::endl;
    std::cout << "4. �ӿ��豸����/Ч��: ������ӿ�(1,3..11)�����ӿ�(2,4..12)�ĶѶ����ҵʱ��(25s/50s)���½ӿ��豸��ʱ��ռ�ã�"
              << "��Ϊϵͳƿ���������ע�Ѷ��Ч�ʡ���������Щ�ӿ�λ�ú���ҵʱ��̶���" << std::endl;
    std::cout << "5. �������: ���ض��ӿ�ǰ��������ӵ�»�ȴ��������Ƿ���ڹ����������ʱͣ��/����㣨�������޸Ĺ������"
              << "��Ŀ������Ĺ����������ͨ��˼·��" << std::endl;
    std::cout << "6. ��������: �Ƚ�3, 5, 7̨��ʱ���ܺ�ʱ���豸�����ʡ������ӳ����߼�Ч��ݼ����ԣ�˵��ϵͳƿ�������ڱ𴦣����豸������������" << std::endl;

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

                // ������1ģʽ�£�ÿ10���¼һ�����д��󳵵�λ����Ϣ
                if (RUN_TASK1_MODE && fmod(CURRENT_SIM_TIME_S, 10.0) < SIM_TIME_STEP_S) {
                    std::stringstream status_ss;
                    status_ss << "=== ����״̬���� (ʱ��: " << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << "s) ===";
                    add_debug_message(status_ss.str());

                    for (const auto& s : shuttles_global) {
                        std::stringstream shuttle_ss;
                        shuttle_ss << "���� S" << s.id
                                << " λ��: " << std::fixed << std::setprecision(2) << s.position_mm << " mm"
                                << ", �ٶ�: " << std::fixed << std::setprecision(2) << s.speed_mm_s << " mm/s"
                                << ", ״̬: " << get_shuttle_state_str_cn(s.agent_state)
                                << ", ��ʻ����: " << std::fixed << std::setprecision(2) << s.distance_traveled_mm << " mm"
                                << ", ͣ������: " << s.stop_count_task1
                                << ", ���һȦ: " << (s.completed_full_circle ? "��" : "��");
                        add_debug_message(shuttle_ss.str());
                    }
                }

                if (!RUN_TASK1_MODE && all_tasks_completed()) {
                    simulation_complete_flag = true;
                    PAUSED = true;
                    std::cout << "������������� (" << num_shuttles_to_run << " ̨����)���ܺ�ʱ: "
                              << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << "��" << std::endl;
                    output_task2_summary_files();
                    break;
                }
            }
        }

        BeginBatchDraw();
        // ���ñ�����ɫΪǳ��ɫ����������
        setbkcolor(RGB(240, 240, 245));
        cleardevice();

        // ���Ʊ�������
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

            // ��D������ǰ������Ϣ���浽�ļ�
            if (key == 'd' || key == 'D') {
                std::string debug_filename = "DebugInfo_" + std::to_string((int)CURRENT_SIM_TIME_S) + "s.txt";
                std::ofstream debug_file(debug_filename);
                if (debug_file.is_open()) {
                    debug_file << "=== ������Ϣ (����ʱ��: " << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << "s) ===" << std::endl;
                    debug_file << "ģʽ: " << (RUN_TASK1_MODE ? "����1 (Ѳ��)" : "����2 (����)") << std::endl;
                    debug_file << "��������: " << shuttles_global.size() << std::endl;
                    debug_file << "\n--- ����״̬ ---" << std::endl;

                    for (const auto& shuttle : shuttles_global) {
                        debug_file << "���� " << shuttle.id << ": "
                                  << get_shuttle_state_str_cn(shuttle.agent_state)
                                  << ", λ��: " << std::fixed << std::setprecision(2) << shuttle.position_mm << "mm";

                        if (shuttle.assigned_task_idx != -1 && shuttle.assigned_task_idx < (int)all_tasks_global.size()) {
                            const Task& t = all_tasks_global[shuttle.assigned_task_idx];
                            debug_file << ", ����: " << t.id << " (" << t.material_id << ")";
                        }

                        debug_file << ", �л�: " << (shuttle.has_goods() ? "��" : "��") << std::endl;
                    }

                    debug_file << "\n--- �豸״̬ ---" << std::endl;
                    for (const auto& pair : devices_global) {
                        const Device& dev = pair.second;
                        debug_file << "�豸 " << dev.id << " (" << dev.name << "): ";

                        switch(dev.op_state) {
                            case DeviceOperationalState::IDLE_EMPTY:
                                debug_file << "�����޻�"; break;
                            case DeviceOperationalState::HAS_GOODS_WAITING_FOR_SHUTTLE:
                                debug_file << "�л��ȴ�����"; break;
                            case DeviceOperationalState::HAS_GOODS_WAITING_FOR_CLEARANCE:
                                debug_file << "�л��ȴ�����"; break;
                            case DeviceOperationalState::HAS_GOODS_BEING_ACCESSED_BY_SHUTTLE:
                                debug_file << "�л������󳵷�����"; break;
                            case DeviceOperationalState::BUSY_SUPPLYING_NEXT_TASK:
                                debug_file << "æµ׼����һ����"; break;
                            case DeviceOperationalState::BUSY_CLEARING_GOODS:
                                debug_file << "æµ�������"; break;
                            default:
                                debug_file << "δ֪״̬";
                        }

                        if (dev.current_task_idx_on_device != -1 &&
                            dev.current_task_idx_on_device < (int)all_tasks_global.size()) {
                            const Task& t = all_tasks_global[dev.current_task_idx_on_device];
                            debug_file << ", ����: " << t.id << " (" << t.material_id << ")";
                        }

                        debug_file << std::endl;
                    }

                    debug_file << "\n--- ���������Ϣ ---" << std::endl;
                    for (const auto& msg : debug_messages) {
                        debug_file << msg << std::endl;
                    }

                    debug_file.close();
                    std::cout << "\n������Ϣ�ѱ��浽 " << debug_filename << std::endl;
                    add_debug_message("������Ϣ�ѱ��浽 " + debug_filename);
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
                        std::cout << "����2δ��ɣ��л�ģʽʱ�����������ܽᡣ" << std::endl;
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

        // ����2ģʽ�£�3Сʱ��ʱ
        if (!RUN_TASK1_MODE && CURRENT_SIM_TIME_S > 3600 * 3 && !simulation_complete_flag) {
            std::cout << "����: ����ʱ�䳬��3Сʱ�����ܴ��������������޷���ɡ��Զ���ͣ��" << std::endl;
            PAUSED = true;
            simulation_complete_flag = true;
            output_task2_summary_files();
        }

        // ����1ģʽ�£�����������
        if (RUN_TASK1_MODE && !simulation_complete_flag) {
            // ������д����Ƿ������һ��ȦѲ��
            bool all_shuttles_completed_circle = true;
            for (const auto& shuttle : shuttles_global) {
                if (!shuttle.completed_full_circle) {
                    all_shuttles_completed_circle = false;
                    break;
                }
            }

            // ���ǲ��ټ��ͣ��������ֻ���ĳ����Ƿ����һȦ

            // �������������д������һȦѲ�Σ����߷���ʱ�䳬��10����
            if (all_shuttles_completed_circle || CURRENT_SIM_TIME_S > 600) {
                std::stringstream reason;
                if (all_shuttles_completed_circle) {
                    reason << "���д��������һ��ȦѲ��";
                } else {
                    reason << "����ʱ���ѳ���10���ӣ������ִ���δ���һ��ȦѲ��";
                }

                std::cout << "����1������������㣨" << reason.str() << "��������ͳ�Ʊ��档" << std::endl;
                add_debug_message("����1������������㣨" + reason.str() + "��������ͳ�Ʊ��档");

                // ȷ����¼���ļӼ��ٽ׶�
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


// ����������־�ļ�

// ��ʾͼ�λ����˵�
enum class MenuResult { START_SIMULATION, SWITCH_MODE, CHANGE_SHUTTLES, EXIT };

MenuResult show_graphical_menu(bool& run_task1_mode, int& num_shuttles) {
    // �����Ļ
    cleardevice();

    // ���Ʊ���
    setbkcolor(RGB(30, 30, 40));
    cleardevice();

    // ���Ʊ���
    draw_title("���ִܲ����ι������ϵͳ����", SCREEN_WIDTH / 2, 50);

    // ���Ƶ�ǰģʽ��Ϣ
    std::string mode_text = "��ǰģʽ: " + std::string(run_task1_mode ? "����1 (Ѳ�����¼)" : "����2 (�����������־)");
    draw_info_text(mode_text, 50, 100, RGB(180, 220, 255));

    // ���ƴ���������Ϣ
    std::string shuttle_text;
    if (run_task1_mode) {
        shuttle_text = "��������: 3 (�̶�)";
    } else {
        shuttle_text = "��������: " + std::to_string(num_shuttles);
    }
    draw_info_text(shuttle_text, 50, 130, RGB(180, 220, 255));

    // ������־�ļ���Ϣ
    draw_info_text("������־�ļ�: " + current_log_filename, 50, 160, RGB(255, 220, 100));

    // �����˵���ť
    std::vector<MenuButton> buttons;
    buttons.push_back(MenuButton(SCREEN_WIDTH / 2 - 150, 220, 300, 50, "��ʼ����", 'S'));
    buttons.push_back(MenuButton(SCREEN_WIDTH / 2 - 150, 290, 300, 50, "�л�ģʽ", 'M'));
    buttons.push_back(MenuButton(SCREEN_WIDTH / 2 - 150, 360, 300, 50, "���Ĵ�������", 'N', !run_task1_mode));
    buttons.push_back(MenuButton(SCREEN_WIDTH / 2 - 150, 430, 300, 50, "�˳�����", 'Q'));

    // ����˵����Ϣ
    draw_info_text("����˵��:", 50, 500, RGB(255, 255, 150));
    draw_info_text("1. �����ť���¶�Ӧ���ȼ� [S/M/N/Q] ѡ�����", 70, 530);
    draw_info_text("2. ��������а� P ��ͣ/����, +/- �����ٶ�, D ���������Ϣ, Q �˳�", 70, 560);
    draw_info_text("3. ���е�����Ϣ�����浽��־�ļ���", 70, 590);

    // ���ư�ť
    int hover_button = -1;
    for (size_t i = 0; i < buttons.size(); i++) {
        draw_menu_button(buttons[i], false);
    }

    // �������ͼ����¼�
    MOUSEMSG msg;
    bool menu_active = true;
    MenuResult result = MenuResult::EXIT;

    while (menu_active) {
        // �������¼�
        if (MouseHit()) {
            msg = GetMouseMsg();

            // �������Ƿ���ͣ�ڰ�ť��
            int new_hover = -1;
            for (size_t i = 0; i < buttons.size(); i++) {
                if (buttons[i].enabled && buttons[i].contains(msg.x, msg.y)) {
                    new_hover = i;
                    break;
                }
            }

            // �����ͣ״̬�ı䣬�ػ水ť
            if (new_hover != hover_button) {
                // �ػ����а�ť
                for (size_t i = 0; i < buttons.size(); i++) {
                    draw_menu_button(buttons[i], i == new_hover);
                }
                hover_button = new_hover;
            }

            // ��������
            if (msg.uMsg == WM_LBUTTONDOWN && hover_button != -1) {
                switch (hover_button) {
                    case 0: // ��ʼ����
                        result = MenuResult::START_SIMULATION;
                        menu_active = false;
                        break;
                    case 1: // �л�ģʽ
                        run_task1_mode = !run_task1_mode;
                        // ����ģʽ��Ϣ
                        setfillcolor(RGB(30, 30, 40));
                        solidrectangle(50, 100, 500, 130);
                        mode_text = "��ǰģʽ: " + std::string(run_task1_mode ? "����1 (Ѳ�����¼)" : "����2 (�����������־)");
                        draw_info_text(mode_text, 50, 100, RGB(180, 220, 255));
                        // ���´���������Ϣ
                        setfillcolor(RGB(30, 30, 40));
                        solidrectangle(50, 130, 500, 160);
                        if (run_task1_mode) {
                            shuttle_text = "��������: 3 (�̶�)";
                        } else {
                            shuttle_text = "��������: " + std::to_string(num_shuttles);
                        }
                        draw_info_text(shuttle_text, 50, 130, RGB(180, 220, 255));
                        // ���°�ť״̬
                        buttons[2].enabled = !run_task1_mode;
                        for (size_t i = 0; i < buttons.size(); i++) {
                            draw_menu_button(buttons[i], i == hover_button);
                        }
                        result = MenuResult::SWITCH_MODE;
                        break;
                    case 2: // ���Ĵ�������
                        if (!run_task1_mode) {
                            // ѭ���л�����������3 -> 5 -> 7 -> 3
                            num_shuttles = (num_shuttles == 3) ? 5 : ((num_shuttles == 5) ? 7 : 3);
                            // ���´���������Ϣ
                            setfillcolor(RGB(30, 30, 40));
                            solidrectangle(50, 130, 500, 160);
                            shuttle_text = "��������: " + std::to_string(num_shuttles);
                            draw_info_text(shuttle_text, 50, 130, RGB(180, 220, 255));
                            result = MenuResult::CHANGE_SHUTTLES;
                        }
                        break;
                    case 3: // �˳�����
                        result = MenuResult::EXIT;
                        menu_active = false;
                        break;
                }
            }
        }

        // �������¼�
        if (_kbhit()) {
            char key = toupper(_getch());
            switch (key) {
                case 'S': // ��ʼ����
                    result = MenuResult::START_SIMULATION;
                    menu_active = false;
                    break;
                case 'M': // �л�ģʽ
                    run_task1_mode = !run_task1_mode;
                    // ����ģʽ��Ϣ
                    setfillcolor(RGB(30, 30, 40));
                    solidrectangle(50, 100, 500, 130);
                    mode_text = "��ǰģʽ: " + std::string(run_task1_mode ? "����1 (Ѳ�����¼)" : "����2 (�����������־)");
                    draw_info_text(mode_text, 50, 100, RGB(180, 220, 255));
                    // ���´���������Ϣ
                    setfillcolor(RGB(30, 30, 40));
                    solidrectangle(50, 130, 500, 160);
                    if (run_task1_mode) {
                        shuttle_text = "��������: 3 (�̶�)";
                    } else {
                        shuttle_text = "��������: " + std::to_string(num_shuttles);
                    }
                    draw_info_text(shuttle_text, 50, 130, RGB(180, 220, 255));
                    // ���°�ť״̬
                    buttons[2].enabled = !run_task1_mode;
                    for (size_t i = 0; i < buttons.size(); i++) {
                        draw_menu_button(buttons[i], i == hover_button);
                    }
                    result = MenuResult::SWITCH_MODE;
                    break;
                case 'N': // ���Ĵ�������
                    if (!run_task1_mode) {
                        // ѭ���л�����������3 -> 5 -> 7 -> 3
                        num_shuttles = (num_shuttles == 3) ? 5 : ((num_shuttles == 5) ? 7 : 3);
                        // ���´���������Ϣ
                        setfillcolor(RGB(30, 30, 40));
                        solidrectangle(50, 130, 500, 160);
                        shuttle_text = "��������: " + std::to_string(num_shuttles);
                        draw_info_text(shuttle_text, 50, 130, RGB(180, 220, 255));
                        result = MenuResult::CHANGE_SHUTTLES;
                    }
                    break;
                case 'Q': // �˳�����
                    result = MenuResult::EXIT;
                    menu_active = false;
                    break;
            }
        }

        // ������ʱ������CPUʹ��
        Sleep(10);
    }

    return result;
}

void create_debug_log_file() {
    // ���ÿ���̨���ΪGBK����
    SetConsoleOutputCP(936); // 936��GBK����

    // ��������ʱ�������־�ļ���
    time_t now = time(0);
    struct tm* timeinfo = localtime(&now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", timeinfo);

    // ɾ��֮ǰ�ĵ�����־�ļ�
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile("DebugLog_*.txt", &findFileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            DeleteFile(findFileData.cFileName);
        } while (FindNextFile(hFind, &findFileData));
        FindClose(hFind);
    }

    current_log_filename = "DebugLog_" + std::string(buffer) + ".txt";

    // ֱ�Ӵ��ļ���ʹ��GBK����
    debug_log_file.open(current_log_filename.c_str());

    if (debug_log_file.is_open()) {
        debug_log_file << "=== ������־��ʼ (" << buffer << ") ===" << std::endl;
        debug_log_file << "ע�⣺����EasyXͼ�ο�����ƣ�����̨�����޷�������ʾ�ı�" << std::endl;
        debug_log_file << "���е�����Ϣ�����浽����־�ļ���" << std::endl;
        debug_log_file << "=== ϵͳ��Ϣ ===" << std::endl;
        debug_log_file << "ģʽ: " << (RUN_TASK1_MODE ? "����1 (Ѳ��)" : "����2 (����)") << std::endl;
        debug_log_file << "��������: " << shuttles_global.size() << std::endl;
        debug_log_file << "=== ������־ ===" << std::endl;

        // �����ڿ���̨����������ܲ�����ʾ
        std::cout << "������־�����浽: " << current_log_filename << std::endl;
    } else {
        // �����ڿ���̨���������Ϣ�������ܲ�����ʾ
        std::cerr << "�޷�����������־�ļ�!" << std::endl;
    }
}

int main() {
    // ���ÿ���̨���ΪGBK����
    SetConsoleOutputCP(936); // 936��GBK����
    // ���ÿ���̨����ΪGBK����
    SetConsoleCP(936);

    setlocale(LC_ALL, "chs");

    // ��ʼ��ͼ�δ���
    initgraph(SCREEN_WIDTH, SCREEN_HEIGHT, EW_SHOWCONSOLE);
    if (GetHWnd() == NULL) {
        std::cout << "ͼ�γ�ʼ��ʧ��! �޷���ȡ���ھ����" << std::endl;
        std::cin.get();
        return 1;
    }

    // ����������־�ļ�
    create_debug_log_file();

    // ��¼������Ϣ
    add_debug_message("��������");
    add_debug_message("ͼ�δ��ڳ�ʼ���ɹ�");
    add_debug_message("������־�ļ������ɹ�: " + current_log_filename);

    srand(static_cast<unsigned int>(time(NULL)));

    int num_shuttles_for_current_run = 3;
    bool program_running = true;

    while(program_running) {
        // ��ʾͼ�λ��˵�
        MenuResult menu_result = show_graphical_menu(RUN_TASK1_MODE, num_shuttles_for_current_run);

        switch (menu_result) {
            case MenuResult::START_SIMULATION: {
                int shuttles_for_this_specific_run = RUN_TASK1_MODE ? 3 : num_shuttles_for_current_run;

                // ��¼������Ϣ
                if (RUN_TASK1_MODE) {
                    add_debug_message("��ʼ����1 (3̨����)");
                } else {
                    add_debug_message("��ʼ����2 (" + std::to_string(shuttles_for_this_specific_run) + "̨����)");
                }

                // ���з���
                run_simulation_loop(shuttles_for_this_specific_run);

                add_debug_message("�����ѽ������ѷ������˵�");
                break;
            }
            case MenuResult::SWITCH_MODE:
                add_debug_message("ģʽ���л�����ǰģʽ: " + std::string(RUN_TASK1_MODE ? "����1" : "����2"));
                break;
            case MenuResult::CHANGE_SHUTTLES:
                add_debug_message("���������Ѹ���Ϊ: " + std::to_string(num_shuttles_for_current_run));
                break;
            case MenuResult::EXIT:
                program_running = false;
                add_debug_message("�û�ѡ���˳�����");
                break;
        }
    }

    closegraph();
    std::cout << "�������˳���" << std::endl;
    return 0;
}

