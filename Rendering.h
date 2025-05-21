#ifndef RENDERING_H
#define RENDERING_H

// 确保Windows版本定义一致
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600

#include <graphics.h>  // EasyX图形库
#include <map>
#include <vector>

// 前向声明
class Device;
class Shuttle;
class Task;

// 屏幕和UI参数
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern const int UI_INFO_PANE_HEIGHT;
extern const int DRAW_AREA_WIDTH;
extern const int DRAW_AREA_HEIGHT;

// 绘图参数
extern double DRAW_SCALE;
extern double DRAW_OFFSET_X;
extern double DRAW_OFFSET_Y;

// 颜色常量
extern const COLORREF BG_COLOR;
extern const COLORREF TRACK_COLOR;
extern const COLORREF SECTION_BG_COLOR;
extern const COLORREF TEXT_COLOR;
extern const COLORREF HIGHLIGHT_COLOR;
// 注意：以下颜色常量已在 EasyX 图形库中定义，不需要重新声明
// RED, GREEN, BLUE, YELLOW, MAGENTA, CYAN, BROWN, LIGHTBLUE, LIGHTGREEN,
// LIGHTCYAN, LIGHTRED, LIGHTMAGENTA, DARKGRAY

// 渲染相关函数声明
void init_graphics();
void draw_track();
void draw_ui_info(int num_shuttles);
void map_track_pos_to_screen_xy(double track_pos_mm, int& screen_x, int& screen_y);
void draw_devices(const std::map<int, Device>& devices_map, const std::vector<Task>& tasks_vec);
void draw_shuttles(const std::vector<Shuttle>& shuttles_vec, const std::vector<Task>& tasks_vec);

#endif // RENDERING_H
