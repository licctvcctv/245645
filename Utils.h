#ifndef UTILS_H
#define UTILS_H

// 确保Windows版本定义一致
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600

#include <string>
#include <vector>
#include <graphics.h> // 添加 EasyX 图形库头文件，提供 COLORREF 类型
#include <windows.h>  // 用于编码转换

// 菜单按钮结构
struct MenuButton {
    int x, y, width, height;
    std::string text;
    char hotkey;
    bool enabled;

    MenuButton(int _x, int _y, int _w, int _h, const std::string& _text, char _hotkey, bool _enabled = true);
    bool contains(int mx, int my) const;
};

// 菜单结果枚举
enum class MenuResult {
    START_SIMULATION,
    SWITCH_MODE,
    CHANGE_SHUTTLES,
    EXIT
};

// 调试信息相关
extern const int MAX_DEBUG_MESSAGES;
extern std::vector<std::string> debug_messages;
extern std::string current_log_filename;

// 编码转换函数声明
std::wstring utf8_to_wstring(const std::string& str);
std::string wstring_to_utf8(const std::wstring& wstr);
std::string utf8_to_gbk(const std::string& utf8_str);
std::string gbk_to_utf8(const std::string& gbk_str);

// 工具函数声明
void create_debug_log_file();
void draw_menu_button(const MenuButton& button, bool hover = false);
void draw_title(const std::string& title, int x, int y);
void draw_info_text(const std::string& text, int x, int y, COLORREF color);
MenuResult show_graphical_menu(bool& run_task1_mode, int& num_shuttles);
void log_exception(const std::string& exception_info);

// 设置字体函数
void set_text_style(int height, int width, const char* font_name);

#endif // UTILS_H
