// 确保Windows版本定义一致
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600

#include "Rendering.h"
#include "Shuttle.h"
#include "Device.h"
#include "Task.h"
#include "Simulation.h"
#include "Utils.h"  // 添加Utils.h，提供编码转换函数
#include <sstream>
#include <iomanip>
#include <cmath> // 添加数学函数头文件，提供 M_PI, sin, cos 等
#include <algorithm> // 提供std::min函数

// 屏幕和UI参数
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;
const int UI_INFO_PANE_HEIGHT = 200;
const int DRAW_AREA_WIDTH = SCREEN_WIDTH - 20;
const int DRAW_AREA_HEIGHT = SCREEN_HEIGHT - UI_INFO_PANE_HEIGHT - 20;
extern double DRAW_SCALE;
extern double DRAW_OFFSET_X;
extern double DRAW_OFFSET_Y;

// 颜色常量
const COLORREF BG_COLOR = RGB(240, 240, 245); // 与 main_new.cpp 中设置一致
const COLORREF TRACK_COLOR = RGB(100, 100, 100);
const COLORREF SECTION_BG_COLOR = RGB(220, 220, 220);
const COLORREF TEXT_COLOR = RGB(0, 0, 0);
const COLORREF HIGHLIGHT_COLOR = RGB(0, 100, 200);
const COLORREF GRID_COLOR = RGB(230, 230, 235); // 添加网格颜色
// 注意：以下颜色常量已在 EasyX 图形库中定义，不需要重新定义
// RED = RGB(255, 0, 0)
// GREEN = RGB(0, 255, 0)
// BLUE = RGB(0, 0, 255)
// YELLOW = RGB(255, 255, 0)
// MAGENTA = RGB(255, 0, 255)
// CYAN = RGB(0, 255, 255)
// BROWN = RGB(165, 42, 42)
// LIGHTBLUE = RGB(173, 216, 230)
// LIGHTGREEN = RGB(144, 238, 144)
// LIGHTCYAN = RGB(224, 255, 255)
// LIGHTRED = RGB(255, 182, 193)
// LIGHTMAGENTA = RGB(255, 182, 255)
// DARKGRAY = RGB(169, 169, 169)

// 初始化图形界面
void init_graphics() {
    // 不要在此函数中调用 initgraph，它已经在 main() 中被调用了
    setbkcolor(BG_COLOR);
    cleardevice();
    
    // 初始化绘图参数
    DRAW_SCALE = 1.0;
    DRAW_OFFSET_X = 0.0;
    DRAW_OFFSET_Y = 0.0;
    
    // 设置字体
    settextstyle(14, 0, "微软雅黑");
    setbkmode(TRANSPARENT);  // 设置文字背景透明
}

// 绘制轨道
void draw_track() {
    // 设置轨道颜色和样式
    const COLORREF TRACK_COLOR = RGB(120, 120, 120);       // 轨道主体颜色
    const COLORREF TRACK_HIGHLIGHT = RGB(180, 180, 180);   // 轨道高亮颜色
    const COLORREF TRACK_SHADOW = RGB(80, 80, 80);         // 轨道阴影颜色
    const COLORREF TRACK_MARKER = RGB(200, 200, 200);      // 轨道标记颜色
    
    // 绘制背景网格 (增加这部分以与 main_new.cpp 中的网格渲染保持一致)
    setlinecolor(GRID_COLOR);
    setlinestyle(PS_SOLID, 1);
    for (int x = 0; x < SCREEN_WIDTH; x += 50) {
        line(x, UI_INFO_PANE_HEIGHT, x, SCREEN_HEIGHT);
    }
    for (int y = UI_INFO_PANE_HEIGHT; y < SCREEN_HEIGHT; y += 50) {
        line(0, y, SCREEN_WIDTH, y);
    }

    // 绘制轨道阴影
    setlinecolor(TRACK_SHADOW);
    setlinestyle(PS_SOLID, 5);

    // 绘制上直道
    line(100 + 2, 200 + 2, SCREEN_WIDTH - 100 + 2, 200 + 2);

    // 绘制下直道
    line(100 + 2, 500 + 2, SCREEN_WIDTH - 100 + 2, 500 + 2);

    // 绘制左弯道
    arc(100 + 2, 350 + 2, 200 + 2, 200 + 2, 350 + 2, 500 + 2);

    // 绘制右弯道
    arc(SCREEN_WIDTH - 100 + 2, 350 + 2, SCREEN_WIDTH - 200 + 2, 200 + 2, SCREEN_WIDTH - 200 + 2, 500 + 2);

    // 绘制轨道主体
    setlinecolor(TRACK_COLOR);
    setlinestyle(PS_SOLID, 4);

    // 绘制上直道
    line(100, 200, SCREEN_WIDTH - 100, 200);

    // 绘制下直道
    line(100, 500, SCREEN_WIDTH - 100, 500);

    // 绘制左弯道
    arc(100, 350, 200, 200, 350, 500);

    // 绘制右弯道
    arc(SCREEN_WIDTH - 100, 350, SCREEN_WIDTH - 200, 200, SCREEN_WIDTH - 200, 500);

    // 绘制轨道高亮
    setlinecolor(TRACK_HIGHLIGHT);
    setlinestyle(PS_SOLID, 1);

    // 绘制上直道
    line(100 - 1, 200 - 1, SCREEN_WIDTH - 100 - 1, 200 - 1);

    // 绘制下直道
    line(100 - 1, 500 - 1, SCREEN_WIDTH - 100 - 1, 500 - 1);

    // 绘制左弯道
    arc(100 - 1, 350 - 1, 200 - 1, 200 - 1, 350 - 1, 500 - 1);

    // 绘制右弯道
    arc(SCREEN_WIDTH - 100 - 1, 350 - 1, SCREEN_WIDTH - 200 - 1, 200 - 1, SCREEN_WIDTH - 200 - 1, 500 - 1);

    // 绘制轨道标记点
    setfillcolor(TRACK_MARKER);

    // 绘制轨道上的标记点
    // 上直道起点
    solidcircle(100, 200, 3);

    // 右弯道起点
    solidcircle(SCREEN_WIDTH - 100, 200, 3);

    // 下直道起点
    solidcircle(SCREEN_WIDTH - 100, 500, 3);

    // 左弯道起点
    solidcircle(100, 500, 3);

    // 在上直道上绘制5个蓝色方块设备
    setfillcolor(BLUE);
    for (int i = 0; i < 5; i++) {
        int x = 100 + (SCREEN_WIDTH - 200) / 6 * (i + 1);
        solidrectangle(x - 5, 200 - 5, x + 5, 200 + 5);
    }

    // 在下直道上绘制4个红色方块设备和2个绿色方块设备
    setfillcolor(RED);
    for (int i = 0; i < 4; i++) {
        int x = SCREEN_WIDTH - 100 - (SCREEN_WIDTH - 200) / 6 * (i + 1);
        solidrectangle(x - 5, 500 - 5, x + 5, 500 + 5);
    }

    setfillcolor(GREEN);
    for (int i = 0; i < 2; i++) {
        int x = SCREEN_WIDTH - 100 - (SCREEN_WIDTH - 200) / 6 * (i + 5);
        solidrectangle(x - 5, 500 - 5, x + 5, 500 + 5);
    }
}

// 将轨道位置映射到屏幕坐标
void map_track_pos_to_screen_xy(double track_pos_mm, int& screen_x, int& screen_y) {
    // 对于轨道位置进行规范化，确保它在有效范围内
    track_pos_mm = normalize_track_pos(track_pos_mm);

    // 计算各段轨道的长度，与 Simulation.cpp 中保持一致
    const double top_straight_length = POS_FIRST_CURVE_START - POS_TOP_STRAIGHT_START;
    const double bottom_straight_length = POS_SECOND_CURVE_START - POS_BOTTOM_STRAIGHT_START;
    const double first_curve_length = POS_FIRST_CURVE_END - POS_FIRST_CURVE_START;
    const double second_curve_length = POS_SECOND_CURVE_END - POS_SECOND_CURVE_START;

    // 使用与 main.cpp 相同的绘图区域计算
    const int left_x = 80;
    const int right_x = DRAW_AREA_WIDTH - 80;
    const int top_y = 80 + UI_INFO_PANE_HEIGHT;
    const int bottom_y = SCREEN_HEIGHT - 80;
    const int curve_radius = 150;

    // 应用缩放和偏移
    int scaled_x, scaled_y;

    if (track_pos_mm >= POS_TOP_STRAIGHT_START && track_pos_mm < POS_FIRST_CURVE_START) {
        // 上直道
        double ratio = (track_pos_mm - POS_TOP_STRAIGHT_START) / top_straight_length;
        scaled_x = left_x + static_cast<int>((right_x - left_x) * ratio);
        scaled_y = top_y;

        // 应用缩放和偏移
        screen_x = static_cast<int>(scaled_x * DRAW_SCALE + DRAW_OFFSET_X);
        screen_y = static_cast<int>(scaled_y * DRAW_SCALE + DRAW_OFFSET_Y);
    } else if (track_pos_mm >= POS_FIRST_CURVE_START && track_pos_mm < POS_FIRST_CURVE_END) {
        // 右弯道
        double angle_rad = (track_pos_mm - POS_FIRST_CURVE_START) / first_curve_length * 3.14159265358979323846 / 2.0; // 使用 π 的值
        scaled_x = right_x - static_cast<int>(curve_radius * sin(angle_rad));
        scaled_y = top_y + static_cast<int>(curve_radius * (1.0 - cos(angle_rad)));

        // 应用缩放和偏移
        screen_x = static_cast<int>(scaled_x * DRAW_SCALE + DRAW_OFFSET_X);
        screen_y = static_cast<int>(scaled_y * DRAW_SCALE + DRAW_OFFSET_Y);
    } else if (track_pos_mm >= POS_BOTTOM_STRAIGHT_START && track_pos_mm < POS_SECOND_CURVE_START) {
        // 下直道
        double ratio = (track_pos_mm - POS_BOTTOM_STRAIGHT_START) / bottom_straight_length;
        scaled_x = right_x - static_cast<int>((right_x - left_x) * ratio);
        scaled_y = bottom_y;

        // 应用缩放和偏移
        screen_x = static_cast<int>(scaled_x * DRAW_SCALE + DRAW_OFFSET_X);
        screen_y = static_cast<int>(scaled_y * DRAW_SCALE + DRAW_OFFSET_Y);
    } else {
        // 左弯道
        double angle_rad = (track_pos_mm - POS_SECOND_CURVE_START) / second_curve_length * 3.14159265358979323846 / 2.0; // 使用 π 的值
        scaled_x = left_x + static_cast<int>(curve_radius * sin(angle_rad));
        scaled_y = bottom_y - static_cast<int>(curve_radius * (1.0 - cos(angle_rad)));

        // 应用缩放和偏移
        screen_x = static_cast<int>(scaled_x * DRAW_SCALE + DRAW_OFFSET_X);
        screen_y = static_cast<int>(scaled_y * DRAW_SCALE + DRAW_OFFSET_Y);
    }
}

// 绘制UI信息
void draw_ui_info(int num_shuttles) {
    // 设置UI面板的颜色和样式
    const COLORREF TITLE_COLOR = RGB(255, 255, 200);  // 浅黄色标题
    const COLORREF TEXT_COLOR = RGB(220, 220, 220);   // 浅灰色文本
    const COLORREF HIGHLIGHT_COLOR = RGB(120, 230, 180); // 高亮绿色
    const COLORREF WARNING_COLOR = RGB(255, 180, 120);   // 警告橙色
    const COLORREF SECTION_BG_COLOR = RGB(60, 60, 70);   // 深色背景
    const COLORREF HEADER_BG_COLOR = RGB(40, 40, 50);    // 更深的标题背景
    
    // 先绘制UI面板背景
    setfillcolor(RGB(50, 50, 50));
    solidrectangle(0, 0, SCREEN_WIDTH, UI_INFO_PANE_HEIGHT);

    int y_pos = 10;
    char buffer[256];

    // 设置透明背景模式
    setbkmode(TRANSPARENT);

    // ===== 顶部信息面板 =====
    // 绘制标题背景
    setfillcolor(HEADER_BG_COLOR);
    solidrectangle(5, 5, SCREEN_WIDTH-5, 35);

    // 绘制标题
    settextcolor(TITLE_COLOR);
    settextstyle(18, 0, "微软雅黑");
    std::string title_gbk = utf8_to_gbk("智能穿梭车轨道运输系统仿真");
    outtextxy(15, y_pos, title_gbk.c_str());

    // 绘制仿真信息
    settextcolor(TEXT_COLOR);
    settextstyle(14, 0, "微软雅黑");

    std::stringstream info_ss;
    info_ss << "仿真时间: " << std::fixed << std::setprecision(2) << CURRENT_SIM_TIME_S << " 秒  |  速度: "
            << std::fixed << std::setprecision(1) << SIM_SPEED_MULTIPLIER << "x  |  穿梭车: "
            << num_shuttles << "  |  模式: "
            << (RUN_TASK1_MODE ? "任务1(巡游)" : "任务2(调度)");
    std::string info_gbk = utf8_to_gbk(info_ss.str());
    outtextxy(SCREEN_WIDTH - 450, y_pos + 2, info_gbk.c_str());
    y_pos += 35;

    // 绘制控制信息面板
    setfillcolor(SECTION_BG_COLOR);
    solidrectangle(5, y_pos, SCREEN_WIDTH-5, y_pos+25);

    // 绘制控制信息
    settextcolor(TEXT_COLOR);
    settextstyle(14, 0, "微软雅黑");
    std::string control_gbk = utf8_to_gbk("控制: P-暂停/继续, +/- 调整速度, M-切换模式(重新开始), Q-退出仿真");
    outtextxy(15, y_pos+4, control_gbk.c_str());
    y_pos += 30;

    // ===== 穿梭车状态面板 =====
    // 绘制穿梭车状态面板
    setfillcolor(SECTION_BG_COLOR);
    int shuttle_section_start = y_pos;
    int shuttle_section_height = 25;
    if (shuttles_global.size() > 4) {
        shuttle_section_height = 45; // 如果穿梭车较多，增加面板高度
    }
    solidrectangle(5, y_pos, SCREEN_WIDTH-5, y_pos+shuttle_section_height);

    // 绘制穿梭车状态标题
    settextcolor(HIGHLIGHT_COLOR);
    settextstyle(14, 0, "微软雅黑");
    std::string shuttle_title_gbk = utf8_to_gbk("穿梭车状态:");
    outtextxy(15, y_pos+4, shuttle_title_gbk.c_str());

    // 绘制穿梭车状态信息
    int x_shuttle_info = 120;
    settextcolor(TEXT_COLOR);
    settextstyle(14, 0, "微软雅黑");

    for(size_t i = 0; i < shuttles_global.size(); ++i) {
        const auto& s = shuttles_global[i];
        std::string task_str_suffix = "";

        if (!RUN_TASK1_MODE && s.assigned_task_idx != -1 && s.assigned_task_idx < (int)all_tasks_global.size()) {
            task_str_suffix = " (T" + std::to_string(all_tasks_global[s.assigned_task_idx].id);
            if(s.has_goods()) task_str_suffix += "[有]";
            task_str_suffix += ")";
        }

        // 设置穿梭车颜色
        settextcolor(s.color);

        std::stringstream shuttle_ss;
        shuttle_ss << "S" << s.id << ": " << get_shuttle_state_str_cn(s.agent_state) << task_str_suffix;
        std::string shuttle_str_gbk = utf8_to_gbk(shuttle_ss.str());
        outtextxy(x_shuttle_info, y_pos+4, shuttle_str_gbk.c_str());

        SIZE text_size;
        GetTextExtentPoint32A(GetImageHDC(), shuttle_str_gbk.c_str(), shuttle_str_gbk.length(), &text_size);
        x_shuttle_info += text_size.cx + 25; // 添加间距

        // 如果超出屏幕宽度，换行显示
        if (x_shuttle_info > SCREEN_WIDTH - 150 && i < shuttles_global.size() - 1) {
            x_shuttle_info = 120;
            y_pos += 20;
        }
    }
}

// 绘制设备
void draw_devices(const std::map<int, Device>& devices_map, const std::vector<Task>& tasks_vec) {
    // 设置设备显示相关颜色和样式
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

    // 注意：这个函数在draw_track中已经绘制了设备的基本形状，这里只处理设备状态和信息显示

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
                    dev.current_task_idx_on_device < (int)tasks_vec.size() &&
                    tasks_vec[dev.current_task_idx_on_device].status != TaskStatus::DEVICE_PREPARING)) {
                    has_goods_visual = true;
                }
                break;
            default:
                device_color = WHITE;
        }

        // 绘制设备颜色
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

// 绘制穿梭车
void draw_shuttles(const std::vector<Shuttle>& shuttles_vec, const std::vector<Task>& tasks_vec) {
    // 先绘制轨道上的穿梭车预测位置标记（可选，增强可视化效果）
    for (const auto& shuttle : shuttles_vec) {
        if (shuttle.has_target_pos) {
            int tx, ty;
            map_track_pos_to_screen_xy(shuttle.target_pos_mm, tx, ty);
            
            // 绘制目标点标记
            setfillcolor(RGB(200, 200, 200));
            setlinecolor(shuttle.color);
            circle(tx, ty, 5);
            line(tx-7, ty, tx+7, ty);
            line(tx, ty-7, tx, ty+7);
        }
    }
    
    // 然后绘制穿梭车
    for (const auto& shuttle : shuttles_vec) {
        int sx, sy;
        map_track_pos_to_screen_xy(shuttle.position_mm, sx, sy);

        // 绘制穿梭车阴影效果
        setfillcolor(RGB(50, 50, 50));
        solidcircle(sx + 2, sy + 2, 12);

        // 绘制穿梭车颜色
        setfillcolor(shuttle.color);
        solidcircle(sx, sy, 12);

        // 绘制穿梭车边框
        setlinecolor(RGB(50, 50, 50));
        circle(sx, sy, 12);

        // 绘制穿梭车ID
        settextcolor(WHITE);
        setbkmode(TRANSPARENT);
        settextstyle(12, 0, "微软雅黑");
        char shuttle_id_str[5];
        snprintf(shuttle_id_str, sizeof(shuttle_id_str), "S%d", shuttle.id);
        int text_width = textwidth(shuttle_id_str);
        outtextxy(sx - text_width/2, sy - 6, shuttle_id_str);

        // 根据模式显示不同内容信息
        if (!RUN_TASK1_MODE) {
            // 绘制状态信息框
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

            // 如果穿梭车有货，显示货物
            if (shuttle.has_goods()) {
                // 绘制货物标记背景
                setfillcolor(RGB(0, 120, 0));
                solidroundrect(sx - 20, sy + 12, sx + 20, sy + 25, 3, 3);

                // 绘制货物标记文本
                settextcolor(WHITE);
                settextstyle(10, 0, "微软雅黑");
                std::string goods_text = "有货物";
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

            // 绘制连接线指示
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