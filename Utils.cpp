#include "Utils.h"
#include "Simulation.h"
#include "Rendering.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <windows.h>
#include <conio.h>
#include <iostream> // 添加 iostream 头文件，提供 std::cout, std::cerr 等
#include <codecvt>  // 用于编码转换
#include <locale>   // 用于编码转换

// 全局变量定义
const int MAX_DEBUG_MESSAGES = 20;
std::vector<std::string> debug_messages;
std::string current_log_filename;

// UTF-8 转 wstring
std::wstring utf8_to_wstring(const std::string& str) {
    if (str.empty()) return std::wstring();

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

// wstring 转 UTF-8
std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}

// UTF-8 转 GBK
std::string utf8_to_gbk(const std::string& utf8_str) {
    // UTF-8 -> Unicode
    std::wstring unicode = utf8_to_wstring(utf8_str);

    // Unicode -> GBK
    int len = WideCharToMultiByte(936, 0, unicode.c_str(), -1, NULL, 0, NULL, NULL);
    char* gbk_buffer = new char[len + 1];
    memset(gbk_buffer, 0, len + 1);
    WideCharToMultiByte(936, 0, unicode.c_str(), -1, gbk_buffer, len, NULL, NULL);

    std::string gbk_str(gbk_buffer);
    delete[] gbk_buffer;

    return gbk_str;
}

// GBK 转 UTF-8
std::string gbk_to_utf8(const std::string& gbk_str) {
    // GBK -> Unicode
    int len = MultiByteToWideChar(936, 0, gbk_str.c_str(), -1, NULL, 0);
    wchar_t* unicode_buffer = new wchar_t[len + 1];
    memset(unicode_buffer, 0, sizeof(wchar_t) * (len + 1));
    MultiByteToWideChar(936, 0, gbk_str.c_str(), -1, unicode_buffer, len);

    // Unicode -> UTF-8
    std::wstring unicode(unicode_buffer);
    delete[] unicode_buffer;

    return wstring_to_utf8(unicode);
}

// 设置文本样式
void set_text_style(int height, int width, const char* font_name) {
    settextstyle(height, width, font_name);
}

// MenuButton构造函数实现
MenuButton::MenuButton(int _x, int _y, int _w, int _h, const std::string& _text, char _hotkey, bool _enabled)
    : x(_x), y(_y), width(_w), height(_h), text(_text), hotkey(_hotkey), enabled(_enabled) {}

// 判断点是否在按钮内
bool MenuButton::contains(int mx, int my) const {
    return mx >= x && mx <= x + width && my >= y && my <= y + height;
}

// 创建调试日志文件
void create_debug_log_file() {
    // 设置控制台输出为GBK编码
    SetConsoleOutputCP(936); // 936是GBK编码

    // 创建带有时间戳的日志文件名
    time_t now = time(0);
    struct tm* timeinfo = localtime(&now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", timeinfo);

    // 删除之前的调试日志文件（根据用户偏好）
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile("DebugLog_*.txt", &findFileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            DeleteFile(findFileData.cFileName);
        } while (FindNextFile(hFind, &findFileData));
        FindClose(hFind);
    }

    // 删除之前的崩溃日志文件
    DeleteFile("crash_log.txt");

    // 创建新的日志文件名
    current_log_filename = "DebugLog_" + std::string(buffer) + ".txt";

    // 直接打开文件，使用GBK编码
    std::ofstream debug_log_file(current_log_filename.c_str());

    if (debug_log_file.is_open()) {
        debug_log_file << "=== 调试日志开始 (" << buffer << ") ===" << std::endl;
        debug_log_file << "注意：由于EasyX图形库的限制，控制台可能无法正常显示文本" << std::endl;
        debug_log_file << "所有调试信息将保存到此日志文件中" << std::endl;
        debug_log_file << "=== 系统信息 ===" << std::endl;
        debug_log_file << "模式: " << (RUN_TASK1_MODE ? "任务1 (巡游)" : "任务2 (调度)") << std::endl;
        debug_log_file << "仿真速度倍率: " << SIM_SPEED_MULTIPLIER << std::endl;
        debug_log_file << "仿真时间步长: " << SIM_TIME_STEP_S << " 秒" << std::endl;
        debug_log_file << "=== 调试日志 ===" << std::endl;
        debug_log_file.close();

        // 创建崩溃日志文件头
        std::ofstream crash_log("crash_log.txt");
        if (crash_log.is_open()) {
            crash_log << "=== 崩溃日志文件 (" << buffer << ") ===" << std::endl;
            crash_log << "此文件记录程序运行过程中的异常情况" << std::endl;
            crash_log << "如果程序正常退出，此文件应该为空" << std::endl << std::endl;
            crash_log.close();
        }

        // 尝试在控制台输出，但可能不会显示
        std::cout << "调试日志将保存到: " << current_log_filename << std::endl;
        std::cout << "崩溃日志将保存到: crash_log.txt" << std::endl;
    } else {
        // 尝试在控制台输出错误信息，但可能不会显示
        std::cerr << "无法创建调试日志文件!" << std::endl;
    }
}

// 绘制菜单按钮
void draw_menu_button(const MenuButton& button, bool hover) {
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

    // 转换文本编码 UTF-8 -> GBK
    std::string gbk_text = utf8_to_gbk(button.text);

    int text_width = textwidth(gbk_text.c_str());
    int text_height = textheight(gbk_text.c_str());
    outtextxy(button.x + (button.width - text_width) / 2,
              button.y + (button.height - text_height) / 2,
              gbk_text.c_str());

    // 热键提示
    std::string hotkey_text = std::string("[") + button.hotkey + "]";
    settextstyle(14, 0, "微软雅黑");
    outtextxy(button.x + button.width - 30, button.y + button.height - 20, hotkey_text.c_str());
}

// 绘制标题
void draw_title(const std::string& title, int x, int y) {
    settextcolor(RGB(255, 255, 200));
    settextstyle(28, 0, "微软雅黑");

    // 转换文本编码 UTF-8 -> GBK
    std::string gbk_title = utf8_to_gbk(title);

    int text_width = textwidth(gbk_title.c_str());
    outtextxy(x - text_width / 2, y, gbk_title.c_str());
}

// 绘制信息文本
void draw_info_text(const std::string& text, int x, int y, COLORREF color) {
    settextcolor(color);
    settextstyle(16, 0, "微软雅黑");

    // 转换文本编码 UTF-8 -> GBK
    std::string gbk_text = utf8_to_gbk(text);

    outtextxy(x, y, gbk_text.c_str());
}

// 显示图形化菜单
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
    draw_info_text("1. 点击按钮或按下对应的热键 [S/M/N/Q] 选择操作", 70, 530, WHITE);
    draw_info_text("2. 仿真过程中按 P 暂停/继续, +/- 调整速度, D 保存调试信息, Q 退出", 70, 560, WHITE);
    draw_info_text("3. 所有调试信息将保存到日志文件中", 70, 590, WHITE);

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
