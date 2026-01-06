#ifndef __MENU_H
#define __MENU_H

#ifndef NULL
#define NULL ((void *)0)
#endif

#include <stdint.h>
#include "input.h"

/* 自动息屏配置 */
#define AUTO_SLEEP_ENABLED 1        // 是否启用自动息屏 (1=启用, 0=禁用)
/* 配置菜单 */
#define MENU_X 0       // 菜单位置X
#define MENU_Y 0       // 菜单位置Y
#define MENU_WIDTH 128 // 菜单宽度
#define MENU_HEIGHT 64 // 菜单高度

#define MENU_LINE_H 20 // 行高
#define MENU_PADDING 2 // 内边距
#define MENU_MARGIN 2  // 外边距

#define MENU_FONT_W 8  // 字体宽度
#define MENU_FONT_H 16 // 字体高度

#define MENU_BORDER 1         // 边框线条尺寸
#define IS_CENTERED 1         // 是否居中
#define IS_OVERSHOOT 1        // 是否过冲 (果冻效果)
#define OVERSHOOT 0.15       // 过冲量 0 < 范围 < 1;
#define ANIMATION_SPEED 0.5 // 动画速度 0 < 范围 <= 1;

#define CURSOR_CEILING (((MENU_HEIGHT - MENU_MARGIN - MENU_MARGIN) / MENU_LINE_H) - 1) // 光标限位

/* 全局变量声明 */
extern uint32_t last_activity_time;    // 最后活动时间
extern uint8_t screen_sleeping;        // 屏幕睡眠状态
extern uint8_t sleep_before_brightness; // 睡眠前的亮度值
extern uint8_t oled_brightness;        // 当前亮度值（0~255）
extern uint16_t auto_sleep_seconds;    // 可调节的自动睡眠时间(秒)

enum _MENU_StrVarType // 选项字符串附带的变量的数据类型枚举
{
    INT8 = 0,
    UINT8,
    INT16,
    UINT16,
    INT32,
    UINT32,
    STRING,
    CHAR,
    FLOAT,
};

enum _menu_command
{
    BUFFER_DISPLAY, // 无参无返
    BUFFER_CLEAR,   // 无参无返
    SHOW_STRING,    // 可变参数列表对应顺序: x, y, string
    SHOW_CURSOR,    // 可变参数列表对应顺序: x, y, width, height;
    DRAW_FRAME,     // 可变参数列表对应顺序: x, y, width, height;
};

typedef struct _MENU_OptionTypeDef // 选项结构体
{
    char *String;                     // 选项字符串
    void (*func)(void);               // 函数指针
    void *StrVarPointer;              // 附带变量 的指针
    enum _MENU_StrVarType StrVarType; // 附带变量 的类型
    uint8_t StrLen;                   // 字符串长度
} MENU_OptionTypeDef;

typedef struct _MENU_HandleTypeDef // 选项结构体
{
    MENU_OptionTypeDef *OptionList; // 选项列表指针

    int16_t Catch_i;              // 选中下标
    int16_t Cursor_i;             // 光标下标
    int16_t Show_i;               // 显示(遍历)起始下标
    int16_t Option_Max_i;         // 选项列表长度
    int16_t Show_i_Previous;      // 上一次的显示下标
    int16_t Wheel_Event;          // 菜单滚动事件
    uint8_t AnimationUpdateEvent; // 动画更新事件
    uint8_t isRun;                // 运行标志
    uint8_t isInitialized;        // 已初始化标志
    // uint8_t isPlayingAnimation; // 正在播放动画标志

} MENU_HandleTypeDef;

typedef enum UI_State
{
    UI_CLOCK,
    UI_MENU,


} UI_State;

/**********************************************************/
/* 宏函数 */

#define COORD_CHANGE_SIZE(sta, end) (((end) - (sta)) + 1)   // 坐标转换成尺寸 COORD_CHANGE_SIZE
#define SIZE_CHANGE_COORD(sta, size) (((sta) + (size)) - 1) // 尺寸转换成坐标 SIZE_CHANGE_COORD

// 逐步接近目标, actual当前, target目标, step_size步长
#define STEPWISE_TO_TARGET(actual, target, step_size)                                                                          \
    ((((target) - (actual)) > (0.0625))    ? ((actual) + (0.0625) + (((target) - (actual)) * (step_size)))                     \
     : (((target) - (actual)) < -(0.0625)) ? ((actual) - (0.0625) + (((target) - (actual)) * (step_size)))                     \
                                           : ((target)))

/**********************************************************/
/* driver */

int menu_command_callback(enum _menu_command command, ...);
void MENU_RunMenu(MENU_HandleTypeDef *hMENU);
void MENU_HandleInit(MENU_HandleTypeDef *hMENU);
void MENU_Event_and_Action(MENU_HandleTypeDef *hMENU);
void MENU_UpdateIndex(MENU_HandleTypeDef *hMENU);
void MENU_ShowOptionList(MENU_HandleTypeDef *hMENU);
uint8_t MENU_ShowOption(int16_t X, int16_t Y, MENU_OptionTypeDef *Option);
void MENU_ShowCursor(MENU_HandleTypeDef *hMENU);
void MENU_ShowBorder(MENU_HandleTypeDef *hMENU);
void MENU_DrawScrollBar(MENU_HandleTypeDef *hMENU);
void MENU_DrawProgressBar(int16_t x, int16_t y, uint8_t width, uint8_t height, uint8_t value);
void CLOCK_Draw(void);

/* 自动息屏功能 */
void MENU_UpdateActivity(void);
void MENU_CheckAutoSleep(void);

/* 倒计时功能声明已迁移到 time_task.h，避免 MENU.h 过度耦合 */

/**********************************************************/
/* use */

void MENU_RunMainMenu(void);
void MENU_RunToolsMenu(void);
void MENU_RunGamesMenu(void);
void MENU_Information(void);
void MENU_RunSettingMenu(void);
void MENU_DisplaySetting(void);
void MENU_SleepSetting(void);
void MENU_TimerSetting(void);
void MENU_SystemSetting(void);
void MENU_AboutSetting(void);
void MENU_WIFISetting(void);
void MENU_RunTestLongMenu(void);
void MENU_RunWeatherMenu(void);
void Update_FPS_Counter(void);
InputEvent MENU_ReceiveInputEvent(void);
uint32_t Get_Current_FPS(void);

/* 游戏相关函数声明 */
 void Game_Dino_Init(void);

/**********************************************************/
#endif
