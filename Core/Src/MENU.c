
#include "MENU.h"
#include "time_task.h"
#include "weather.h"
#include "stdio.h"
#include "main.h"
#include "string.h"
#include <stdarg.h>


/* 自动息屏相关变量 */
uint32_t last_activity_time = 0;        // 最后活动时间
uint8_t screen_sleeping = 0;             // 屏幕睡眠状态 (0=正常, 1=睡眠)
uint8_t sleep_before_brightness = 128;   // 睡眠前的亮度值
uint8_t oled_brightness = 128;           // 当前亮度值（0~255）
uint16_t auto_sleep_seconds = 120;        // 可调节的自动睡眠时间(秒) - 默认120秒

/* FPS 显示开关 (1=显示, 0=关闭) */
#define SHOW_FPS 1

#if SHOW_FPS
/* FPS相关变量 */
static uint32_t frame_count = 0;
static uint32_t last_fps_time = 0;
static uint32_t current_fps = 0;
#endif



/** Port 移植接口 * **************************************************************/
/* 依赖头文件 */

#include "TIM_EC11.h"
#include "OLED.h"
#include "Game_Snake.h"
#include "Game_Dino.h"
#include "input.h"
#include "cmsis_os.h"

/* 外部队列句柄 - 用于接收输入事件 */
extern osMessageQueueId_t InputEventQueueHandle;
extern osMessageQueueId_t TimeQueueHandle;

/* 函数前向声明 */
InputEvent MENU_ReceiveInputEvent(void);


/*
 * 说明（阅读指南）：
 * 1) 本菜单系统通过 menu_command_callback 作为“统一指令入口”来解耦菜单逻辑和硬件驱动。
 *    - 菜单层：只发命令（显示/光标/边框/读取输入），不关心具体硬件如何实现。
 *    - 设备层：在回调内部把命令映射到 OLED/按键/编码器等驱动函数。
 * 2) 输入事件（确认/返回/旋钮）被采集为标准化的返回值，供菜单状态机消化。

 */
/// @brief 菜单指令回调函数（使用标准 stdarg.h 实现）
/// @param  command 指令
/// @param  ... 可变参数列表根据指令定义
/// @return  返回值根据指令定义
int menu_command_callback(enum _menu_command command, ...)
{
    int retval = 0;
    va_list args;
    va_start(args, command);

    switch (command)
    {
        /* Output */
    case BUFFER_DISPLAY: // 无参无返
    {
        // 统一刷新：将显存缓冲一次性推送到 OLED（OLED_Update）
#if SHOW_FPS
        // 显示FPS在左上角
        uint32_t fps = Get_Current_FPS();
        char fps_str[8];
        sprintf(fps_str, "%lu", fps);
        OLED_ShowString(0, 0, fps_str, OLED_6X8);
#endif
        
        // 显示倒计时（如果启用）
        if (timer_enabled) {
            MENU_ShowTimer(90, 5);
        }
        
        OLED_Update();
#if SHOW_FPS
        Update_FPS_Counter(); // 更新FPS计数器
#endif
    }
    break;

    case BUFFER_CLEAR: // 无参无返
    {
        OLED_Clear();
    }
    break;

    case SHOW_STRING: // 参数:( int x, int y, char *str, int font_size ); 返回: uint8_t 字符串长度;
    {
        /*
         * 使用标准 stdarg.h 提取参数：
         * 注意：va_arg 对于小于 int 的类型会被提升为 int
         * 所以 int16_t, uint8_t 等都应该用 int 来提取
         */
        int show_x = va_arg(args, int);
        int show_y = va_arg(args, int);
        char *show_string = va_arg(args, char *);
        int font_size = va_arg(args, int);
        
        // 如果font_size为0，使用默认值
        if (font_size == 0) {
            font_size = OLED_8X16;
        }

        /* 按需使用参数 */
        OLED_ShowString(show_x, show_y, show_string, (uint8_t)font_size);
        retval = strlen(show_string); // 返回字符串长度
    }
    break;

    case SHOW_CURSOR: // 参数:( int xsta, int ysta, int xend, int yend ); 返回: 无;
    {
        /* 使用 va_arg 提取参数 */
        int cursor_xsta = va_arg(args, int);
        int cursor_ysta = va_arg(args, int);
        int cursor_xend = va_arg(args, int);
        int cursor_yend = va_arg(args, int);

        /* 按需使用参数 */
        OLED_ReverseArea(cursor_xsta, cursor_ysta, COORD_CHANGE_SIZE(cursor_xsta, cursor_xend),
                         COORD_CHANGE_SIZE(cursor_ysta, cursor_yend));
    }
    break;

    case DRAW_FRAME: // 参数:( int x, int y, int width, int height ); 返回: 无;
    {
        /* 使用 va_arg 提取参数 */
        int frame_x = va_arg(args, int);
        int frame_y = va_arg(args, int);
        int frame_width = va_arg(args, int);
        int frame_height = va_arg(args, int);

        /* 按需使用参数 */
        OLED_DrawRectangle(frame_x, frame_y, frame_width, frame_height, 0);
    }
    break;

    default:
        break;
    }

    va_end(args);
    return retval;
}


/**
 * @brief 更新最后活动时间
 */
void MENU_UpdateActivity(void)
{
    last_activity_time = HAL_GetTick();
    
    // 如果屏幕处于睡眠状态，则唤醒
    if (screen_sleeping) {
    // 唤醒策略：恢复进入睡眠前的亮度，清除睡眠标记
        screen_sleeping = 0;
        oled_brightness = sleep_before_brightness;
        OLED_SetBrightness(oled_brightness);
    }
}

/**
 * @brief 检查是否需要自动息屏
 */
void MENU_CheckAutoSleep(void)
{
#if AUTO_SLEEP_ENABLED
    uint32_t current_time = HAL_GetTick();
    uint32_t sleep_time_ms = auto_sleep_seconds * 1000; // 转换为毫秒
    
    // 检查是否超过息屏时间且屏幕未处于睡眠状态
    if (!screen_sleeping && (current_time - last_activity_time) >= sleep_time_ms) {
    // 进入睡眠：保存当前亮度，拉低亮度至 0，并标记睡眠状态
    sleep_before_brightness = oled_brightness;  // 保存当前亮度
        oled_brightness = 0;
        screen_sleeping = 1;
        OLED_SetBrightness(0);  // 关闭屏幕
    }
#endif
}

/* 倒计时相关实现已迁移到 time_task.c */

/* ******************************************************** */

/// @brief 菜单运行函数
/// @param hMENU 菜单句柄
void MENU_RunMenu(MENU_HandleTypeDef *hMENU)
{
    MENU_HandleInit(hMENU); // 初始化
    
    // 初始化最后活动时间
    MENU_UpdateActivity();

    while (hMENU->isRun)
    {
    // 1) 息屏判定：优先检查是否需要息屏
        MENU_CheckAutoSleep();
        
    // 2) 定时器：处理倒计时结束弹窗（阻塞等待确认）
        if (MENU_UpdateTimer()) {
            // 定时器结束，显示"时间到"提示
            MENU_ShowTimeUpAlert();
        }
        
    // 3) 显示：仅在屏幕未睡眠时渲染菜单帧
        if (!screen_sleeping) {
            menu_command_callback(BUFFER_CLEAR); // 擦除缓冲区

            MENU_ShowOptionList(hMENU); /* 显示选项列表 */
            MENU_ShowCursor(hMENU);     /* 显示光标 */
           // MENU_ShowBorder(hMENU);     // 显示边框
            MENU_DrawScrollBar(hMENU);  // 绘制垂直滚动条

            menu_command_callback(BUFFER_DISPLAY); // 缓冲区更新至显示器
        }
    MENU_Event_and_Action(hMENU); // 检查事件及作相应操作

    osDelay(8);  // 使用 osDelay 让出 CPU
    }
}

void MENU_HandleInit(MENU_HandleTypeDef *hMENU)
{
    hMENU->isRun = 1;                // 运行标志
    hMENU->AnimationUpdateEvent = 1; // 动画更新事件
    hMENU->Catch_i = 1;              // 选中下标默认为1,(因为hMENU->OptionList[0]为"<<<")
    hMENU->Cursor_i = 1;             // 光标下标对应选中项
    hMENU->Show_i = 0;               // 显示(遍历)起始下标
    hMENU->Show_i_Previous = 0;      // 上一次循环的显示下标
    hMENU->Option_Max_i = 0;         // 选项列表长度
    hMENU->Wheel_Event = 0;          // 初始化滚轮事件

    for (hMENU->Option_Max_i = 0; hMENU->OptionList[hMENU->Option_Max_i].String[0] != '.';
         hMENU->Option_Max_i++) // 计算选项列表长度
    {
        hMENU->OptionList[hMENU->Option_Max_i].StrLen =
            MENU_ShowOption(0, 0, &hMENU->OptionList[hMENU->Option_Max_i]); // 获取字符串长度
    }
    hMENU->Option_Max_i--; // 不显示".."
}

/**
 * @brief 从队列接收输入事件（非阻塞）
 * @return InputEvent 结构体，如果没有事件则 type = INPUT_NONE
 */
InputEvent MENU_ReceiveInputEvent(void)
{
    InputEvent event;
    event.type = INPUT_NONE;
    event.value = 0;
    
    // 非阻塞方式从队列接收事件 (timeout = 0)
    if (osMessageQueueGet(InputEventQueueHandle, &event, NULL, 0) == osOK)
    {
        return event;
    }
    
    return event;
}

void MENU_Event_and_Action(MENU_HandleTypeDef *hMENU)
{
    // 从队列接收事件，而不是直接调用 Input_GetEvent()
    InputEvent event = MENU_ReceiveInputEvent();
    switch(event.type)
    {
        case INPUT_ENTER: /* 确定事件 */
        {
            MENU_UpdateActivity(); // 更新活动时间
            if (hMENU->OptionList[hMENU->Catch_i].func != NULL)
            {
                hMENU->OptionList[hMENU->Catch_i].func();
            }
            else
            {
                hMENU->isRun = 0; // 退出
            }
            hMENU->AnimationUpdateEvent = 1;
        }
        break;

        case INPUT_BACK: /* 返回事件 */
        {
            MENU_UpdateActivity(); // 更新活动时间
            hMENU->isRun = 0;
        }
        break;

        case INPUT_DOWN: /* 顺时针滚动 (向下) */
        {
            MENU_UpdateActivity();
            // 直接使用编码器的旋转格数，实现快速滚动
            hMENU->Wheel_Event = event.value; 
            
            MENU_UpdateIndex(hMENU);
            hMENU->AnimationUpdateEvent = 1;
        }
        break;

        case INPUT_UP: /* 逆时针滚动 (向上) */
        {
            MENU_UpdateActivity();
            // 向上滚动取负值
            hMENU->Wheel_Event = -event.value;
            
            MENU_UpdateIndex(hMENU);
            hMENU->AnimationUpdateEvent = 1;
        }
        break;

        default:
            hMENU->Wheel_Event = 0;
            break;
    }
}

void MENU_UpdateIndex(MENU_HandleTypeDef *hMENU)
{
    // 选中项（Catch_i）根据滚动事件累加，并支持首尾循环
    /* 更新选中下标 */
    hMENU->Catch_i += hMENU->Wheel_Event;

    /* 限制选中下标 - 支持循环滚动 */
    if (hMENU->Catch_i > hMENU->Option_Max_i) {
        hMENU->Catch_i = 0;  // 超出最大值时回到0
    }
    else if (hMENU->Catch_i < 0) {
        hMENU->Catch_i = hMENU->Option_Max_i;  // 小于0时回到最大值
    }

    /* 重新计算光标位置（Cursor_i）：
     * - 若总项数不超过可视行数，光标直接跟随选中项。
     * - 若超过，分三段处理：顶部区域/底部区域/中间区域（中间时光标固定在中央）。
     */
    // 如果菜单项数量少于等于屏幕可显示数量，光标直接跟随选中项
    if (hMENU->Option_Max_i <= CURSOR_CEILING) {
        hMENU->Cursor_i = hMENU->Catch_i;
    }
    else {
        // 如果菜单项较多，需要滚动显示
        if (hMENU->Catch_i <= CURSOR_CEILING) {
            // 在顶部区域，光标跟随选中项
            hMENU->Cursor_i = hMENU->Catch_i;
        }
        else if (hMENU->Catch_i >= (hMENU->Option_Max_i - CURSOR_CEILING)) {
            // 在底部区域，光标相对底部固定
            hMENU->Cursor_i = CURSOR_CEILING - (hMENU->Option_Max_i - hMENU->Catch_i);
        }
        else {
            // 在中间区域，光标固定在中央
            hMENU->Cursor_i = CURSOR_CEILING / 2;
        }
    }

    /* 确保光标不越界 */
    if (hMENU->Cursor_i < 0) hMENU->Cursor_i = 0;
    if (hMENU->Cursor_i > CURSOR_CEILING) hMENU->Cursor_i = CURSOR_CEILING;
    if (hMENU->Cursor_i > hMENU->Option_Max_i) hMENU->Cursor_i = hMENU->Option_Max_i;
}

void MENU_ShowOptionList(MENU_HandleTypeDef *hMENU)
{
    static float VerticalOffsetBuffer; // 垂直偏移缓冲

    /* 计算显示起始下标 */
    // 显示窗口起点：当前选中项 - 光标位置（让光标位置处的项落在窗口内）
    hMENU->Show_i = hMENU->Catch_i - hMENU->Cursor_i; // 详解 https://www.bilibili.com/read/cv32114635/?jump_opus=1

    if (hMENU->Show_i_Previous != hMENU->Show_i) // 如果显示下标有变化
    {
        // 计算垂直偏移缓冲量：以像素为单位，正负代表向上/向下平移，随后逐帧缓动归零
        VerticalOffsetBuffer = ((hMENU->Show_i - hMENU->Show_i_Previous) * MENU_LINE_H);
        hMENU->Show_i_Previous = hMENU->Show_i;
    }

    if (VerticalOffsetBuffer)
    {
        // 行显示偏移量逐渐归零，形成丝滑滚动感
        VerticalOffsetBuffer = STEPWISE_TO_TARGET(VerticalOffsetBuffer, 0, ANIMATION_SPEED);
    }

    for (int16_t i = -1; i <= CURSOR_CEILING + 1; i++) // 遍历显示 选项
    {
        if (hMENU->Show_i + i < 0)
            continue;

        if (hMENU->Show_i + i > hMENU->Option_Max_i)
            break;

#if (IS_CENTERED != 0)
        int16_t x = MENU_X + ((MENU_WIDTH - (hMENU->OptionList[hMENU->Show_i + i].StrLen * MENU_FONT_W)) / 2); // 水平居中
#else
        int16_t x = MENU_X + MENU_MARGIN + MENU_PADDING; // 左对齐加边距
#endif

        int16_t y = MENU_Y + MENU_MARGIN + (i * MENU_LINE_H) + ((MENU_LINE_H - MENU_FONT_H) / 2) + (int)VerticalOffsetBuffer;

        /* 显示选项, 并记录长度 */
        hMENU->OptionList[hMENU->Show_i + i].StrLen = MENU_ShowOption(x, y, &hMENU->OptionList[hMENU->Show_i + i]);
    }
}

uint8_t MENU_ShowOption(int16_t X, int16_t Y, MENU_OptionTypeDef *Option)
{
    char String[64]; // 定义字符数组

    switch (Option->StrVarType)
    {
    case INT8:
    // 根据变量类型选择合适的格式化方式；模板字符串在 Option->String 中
        sprintf(String, Option->String, *(int8_t *)Option->StrVarPointer);
        break;

    case UINT8:
        sprintf(String, Option->String, *(uint8_t *)Option->StrVarPointer);
        break;

    case INT16:
        sprintf(String, Option->String, *(int16_t *)Option->StrVarPointer);
        break;

    case UINT16:
        sprintf(String, Option->String, *(uint16_t *)Option->StrVarPointer);
        break;

    case INT32:
        sprintf(String, Option->String, *(int32_t *)Option->StrVarPointer);
        break;

    case UINT32:
        sprintf(String, Option->String, *(uint32_t *)Option->StrVarPointer);
        break;

    case CHAR:
        sprintf(String, Option->String, *(char *)Option->StrVarPointer);
        break;

    case STRING:
        sprintf(String, Option->String, (char *)Option->StrVarPointer);
        break;

    case FLOAT:
        sprintf(String, Option->String, *(float *)Option->StrVarPointer);
        break;

    default:
        // 兜底：按指针打印（不建议常用）。如需更强类型安全，考虑引入类型化渲染函数。
        sprintf(String, Option->String, (void *)Option->StrVarPointer);
        break;
    }

    // 注意：String 长度上限 64，如模板+数据可能超出，建议在模板设计时控制长度。
    return menu_command_callback(SHOW_STRING, X, Y, String, OLED_8X16); // 显示字符数组（字符串），使用标准字体
}

void MENU_ShowCursor(MENU_HandleTypeDef *hMENU)
{
    static float actual_xsta, actual_ysta, actual_xend, actual_yend; // actual
    static float target_xsta, target_ysta, target_xend, target_yend; // target

#if (IS_OVERSHOOT != 0)
    static float bounce_xsta, bounce_ysta, bounce_xend, bounce_yend;                   // bounce
    static uint8_t bounce_cnt_xsta, bounce_cnt_ysta, bounce_cnt_xend, bounce_cnt_yend; // bounce

#endif

    // static uint16_t Catch_i_Previous = 1, Cursor_i_Previous = 1; // 上一循环的状态

    if (hMENU->AnimationUpdateEvent)
    {
        hMENU->AnimationUpdateEvent = 0;

        // 光标框宽度基于当前选中项字符串长度计算：左右各留 MENU_PADDING 像素
        uint16_t cursor_width = (MENU_PADDING + (hMENU->OptionList[hMENU->Catch_i].StrLen * MENU_FONT_W) + MENU_PADDING);
        uint16_t cursor_height = MENU_LINE_H;

#if (IS_CENTERED != 0)
        target_xsta = MENU_X + ((MENU_WIDTH - cursor_width) / 2);
#else
        target_xsta = MENU_X + MENU_MARGIN;
#endif
        target_ysta = MENU_Y + MENU_MARGIN + (hMENU->Cursor_i * MENU_LINE_H);
    // 宏 SIZE_CHANGE_COORD(a, size) 将起点与宽/高转换为终点坐标，便于后续绘制
    target_xend = SIZE_CHANGE_COORD(target_xsta, cursor_width);
    target_yend = SIZE_CHANGE_COORD(target_ysta, cursor_height);

#if (IS_OVERSHOOT != 0)
        bounce_xsta = target_xsta + (target_xsta - actual_xsta) * OVERSHOOT;
        bounce_ysta = target_ysta + (target_ysta - actual_ysta) * OVERSHOOT;
        bounce_xend = target_xend + (target_xend - actual_xend) * OVERSHOOT;
        bounce_yend = target_yend + (target_yend - actual_yend) * OVERSHOOT;

        bounce_cnt_xsta = 2; // 反弹次数
        bounce_cnt_ysta = 2;
        bounce_cnt_xend = 2;
        bounce_cnt_yend = 2;
#endif
    }

#if (IS_OVERSHOOT != 0)

    if (bounce_xsta == actual_xsta)
    {
        if (bounce_cnt_xsta--)
            bounce_xsta = target_xsta + (target_xsta - actual_xsta) * OVERSHOOT;
        else
            bounce_xsta = target_xsta;
    }
    if (bounce_ysta == actual_ysta)
    {
        if (bounce_cnt_ysta--)
            bounce_ysta = target_ysta + (target_ysta - actual_ysta) * OVERSHOOT;
        else
            bounce_ysta = target_ysta;
    }
    if (bounce_xend == actual_xend)
    {
        if (bounce_cnt_xend--)
            bounce_xend = target_xend + (target_xend - actual_xend) * OVERSHOOT;
        else
            bounce_xend = target_xend;
    }
    if (bounce_yend == actual_yend)
    {
        if (bounce_cnt_yend--)
            bounce_yend = target_yend + (target_yend - actual_yend) * OVERSHOOT;
        else
            bounce_yend = target_yend;
    }

    // 使用统一的缓动函数 STEPWISE_TO_TARGET 实现平滑/果冻效果
    actual_xsta = STEPWISE_TO_TARGET(actual_xsta, bounce_xsta, ANIMATION_SPEED);
    actual_ysta = STEPWISE_TO_TARGET(actual_ysta, bounce_ysta, ANIMATION_SPEED);
    actual_xend = STEPWISE_TO_TARGET(actual_xend, bounce_xend, ANIMATION_SPEED);
    actual_yend = STEPWISE_TO_TARGET(actual_yend, bounce_yend, ANIMATION_SPEED);

#else

    actual_xsta = STEPWISE_TO_TARGET(actual_xsta, target_xsta, ANIMATION_SPEED);
    actual_ysta = STEPWISE_TO_TARGET(actual_ysta, target_ysta, ANIMATION_SPEED);
    actual_xend = STEPWISE_TO_TARGET(actual_xend, target_xend, ANIMATION_SPEED);
    actual_yend = STEPWISE_TO_TARGET(actual_yend, target_yend, ANIMATION_SPEED);

#endif

    // 四舍五入转换为整数像素坐标后绘制反色块作为光标
    menu_command_callback(SHOW_CURSOR, (int)(actual_xsta + 0.5), (int)(actual_ysta + 0.5), (int)(actual_xend + 0.5),
                          (int)(actual_yend + 0.5));
}

void MENU_ShowBorder(MENU_HandleTypeDef *hMENU) // 显示边框
{
    for (int16_t i = 0; i < MENU_BORDER; i++)
    {
        menu_command_callback(DRAW_FRAME, MENU_X + i, MENU_Y + i, MENU_WIDTH - i - i, MENU_HEIGHT - i - i);
    }
}

/* ******************************************************** */

/* ******************************************************** */
/* 应用示例 */

void MENU_RunMainMenu(void)
{
    static MENU_OptionTypeDef MENU_OptionList[] = {{"<<<"},
                                                   {"Tools", MENU_RunToolsMenu},        // 工具
                                                   {"Games", MENU_RunGamesMenu},        // 游戏
                                                   {"Setting", MENU_RunSettingMenu},    // 设置
                                                   {"Information", MENU_Information},   // 信息
                                                   {"Test Menu", MENU_RunTestLongMenu}, // 测试长菜单
                                                   {"Weather", MENU_RunWeatherMenu},    // 天气
                                                   {".."}};

    static MENU_HandleTypeDef MENU = {.OptionList = MENU_OptionList};

    MENU_RunMenu(&MENU);
}

void MENU_RunToolsMenu(void)
{
    static MENU_OptionTypeDef MENU_OptionList[] = {{"<<<"},
                                                   {"Timer", MENU_TimerSetting},       // 定时器
                                                   {"Serial Port", NULL},              // 串口
                                                   {"Oscilloscope", NULL},             // 示波器
                                                   {"PWM Output", NULL},               // PWM 输出
                                                   {"PWM Input", NULL},                // PWM 输入
                                                   {"ADC Input", NULL},                // ADC 输入
                                                   {"I2C Scanner", NULL},              // I2C扫描
                                                   {"SPI Test", NULL},                 // SPI测试
                                                   {"Timer Config", NULL},             // 定时器配置
                                                   {"GPIO Control", NULL},             // GPIO控制
                                                   {"UART Logger", NULL},              // UART日志
                                                   {"Frequency Gen", NULL},            // 频率发生器
                                                   {"Voltage Meter", NULL},            // 电压计
                                                   {"Logic Analyzer", NULL},           // 逻辑分析仪
                                                   {"Signal Gen", NULL},               // 信号发生器
                                                   {"Memory Test", NULL},              // 内存测试
                                                   {".."}};

    static MENU_HandleTypeDef MENU = {.OptionList = MENU_OptionList};

    MENU_RunMenu(&MENU);
}

void MENU_RunGamesMenu(void)
{
    static MENU_OptionTypeDef MENU_OptionList[] = {{"<<<"},
                                                   {"Snake Game", Game_Snake_Init},     // 贪吃蛇游戏
                                                   {"Dino Game", Game_Dino_Init},       // 恐龙跳跃游戏
                                                   {"Tetris", NULL},                    // 俄罗斯方块（占位）
                                                   {"2048", NULL},                      // 2048游戏（占位）
                                                   {"Pong", NULL},                      // 乒乓球游戏（占位）
                                                   {"Breakout", NULL},                  // 打砖块游戏（占位）
                                                   {".."}};

    static MENU_HandleTypeDef MENU = {.OptionList = MENU_OptionList};

    MENU_RunMenu(&MENU);
}

/**
 * @brief 测试长菜单（演示滚动条）
 */
void MENU_RunTestLongMenu(void)
{
    static MENU_OptionTypeDef MENU_OptionList[] = {{"<<<"},
                                                   {"Test Item 01", NULL},
                                                   {"Test Item 02", NULL},
                                                   {"Test Item 03", NULL},
                                                   {"Test Item 04", NULL},
                                                   {"Test Item 05", NULL},
                                                   {"Test Item 06", NULL},
                                                   {"Test Item 07", NULL},
                                                   {"Test Item 08", NULL},
                                                   {"Test Item 09", NULL},
                                                   {"Test Item 10", NULL},
                                                   {"Test Item 11", NULL},
                                                   {"Test Item 12", NULL},
                                                   {"Test Item 13", NULL},
                                                   {"Test Item 14", NULL},
                                                   {"Test Item 15", NULL},
                                                   {".."}};

    static MENU_HandleTypeDef MENU = {.OptionList = MENU_OptionList};

    MENU_RunMenu(&MENU);
}

void MENU_RunWeatherMenu(void)
{
    Weather_Run();
}


void MENU_Information(void)
{
    menu_command_callback(BUFFER_CLEAR);
    menu_command_callback(SHOW_STRING, 5, 16, "Menu v2.0", OLED_8X16);
    menu_command_callback(SHOW_STRING, 5, 32, "By: Harvey", OLED_8X16);
    
    // 显示定时器状态（如果启用）
    if (timer_enabled) {
        char timer_info[32];
        sprintf(timer_info, "Timer: %ds running", timer_current);
        menu_command_callback(SHOW_STRING, 5, 48, timer_info, OLED_8X16);
    }
    
    menu_command_callback(BUFFER_DISPLAY);

    while (1)
    {
        InputEvent event = MENU_ReceiveInputEvent();
        if (event.type == INPUT_ENTER || event.type == INPUT_BACK)
        {
            MENU_UpdateActivity(); // 更新活动时间
            return;
        }
        osDelay(10);
    }
}

/**********************************************************/

/**
 * @brief 运行设置菜单
 */
void MENU_RunSettingMenu(void)
{
    static MENU_OptionTypeDef MENU_OptionList[] = {{"<<<"},
                                                   {"Display", MENU_DisplaySetting},     // 显示设置
                                                   {"Sleep", MENU_SleepSetting},         // 睡眠设置
                                                   {"System", MENU_SystemSetting},       // 系统设置
                                                   {"About",  MENU_AboutSetting},
												   {"WIFI" ,  MENU_WIFISetting},          // 关于
                                                   {".."}};

    static MENU_HandleTypeDef MENU = {.OptionList = MENU_OptionList};

    MENU_RunMenu(&MENU);
}

/**
 * @brief 显示设置菜单 - 交互式亮度调节
 */
void MENU_DisplaySetting(void)
{
    uint8_t brightness_percent = (oled_brightness * 100) / 255; // 转换为百分比
    char brightness_str[16];
    
    while (1)
    {
        menu_command_callback(BUFFER_CLEAR);
        

        
        // 显示亮度标签
        menu_command_callback(SHOW_STRING, 5, 20, "Brightness:", OLED_8X16);
        
        // 显示亮度百分比
        sprintf(brightness_str, "%d%%", brightness_percent);
        menu_command_callback(SHOW_STRING, 90, 20, brightness_str, OLED_8X16);
        
        // 绘制亮度进度条
        MENU_DrawProgressBar(5, 35, 118, 8, brightness_percent);
        
        // 显示操作提示
        
        menu_command_callback(BUFFER_DISPLAY);
        
        // 处理输入事件
        InputEvent event = MENU_ReceiveInputEvent();
        if (event.type == INPUT_UP || event.type == INPUT_DOWN)
        {
            MENU_UpdateActivity(); // 更新活动时间
            
            // 调整亮度 (每次调整5%) - 反转方向使顺时针增加亮度
            // 下滚(顺时针) = 增加亮度
            int16_t wheel_value = (event.type == INPUT_DOWN) ? event.value : -event.value;
            int16_t new_brightness = brightness_percent + (wheel_value * 5);
            
            // 限制范围
            if (new_brightness < 0) new_brightness = 0;
            if (new_brightness > 100) new_brightness = 100;
            
            brightness_percent = new_brightness;
            
            // 更新实际亮度值
            oled_brightness = (brightness_percent * 255) / 100;
            
            // 立即应用亮度设置
            OLED_SetBrightness(oled_brightness);
        }
        
        // 检查退出事件
        if (event.type == INPUT_ENTER || event.type == INPUT_BACK)
        {
            MENU_UpdateActivity(); // 更新活动时间
            return;
        }
    }
}

/**
 * @brief 睡眠时间设置菜单 - 交互式时间调节
 */
void MENU_SleepSetting(void)
{
    char sleep_str[32];
    
    while (1)
    {
        menu_command_callback(BUFFER_CLEAR);
        
        // 显示标题
        menu_command_callback(SHOW_STRING, 20, 0, "Sleep Setting", OLED_8X16);
        
        // 显示睡眠时间标签
        menu_command_callback(SHOW_STRING, 5, 20, "Auto Sleep:", OLED_8X16);
        
        // 显示睡眠时间（人性化格式化：<60s 显示秒，>=60s 显示 m/s 组合，0 表示 OFF）
        if (auto_sleep_seconds == 0) {
            sprintf(sleep_str, "OFF");
        } else if (auto_sleep_seconds < 60) {
            sprintf(sleep_str, "%ds", auto_sleep_seconds);
        } else {
            uint16_t minutes = auto_sleep_seconds / 60;
            uint16_t seconds = auto_sleep_seconds % 60;
            if (seconds == 0) {
                sprintf(sleep_str, "%dm", minutes);
            } else {
                sprintf(sleep_str, "%dm%ds", minutes, seconds);
            }
        }
        menu_command_callback(SHOW_STRING, 90, 20, sleep_str, OLED_8X16);
        
        // 绘制时间设置进度条 (0-300秒 = 5分钟)
        // 注意：进度条仅用于可视反馈，真正的息屏阈值来自 auto_sleep_seconds。
        uint8_t progress_percent = (auto_sleep_seconds * 100) / 300;
        if (progress_percent > 100) progress_percent = 100;
        MENU_DrawProgressBar(5, 35, 118, 8, progress_percent);
        
        // 显示操作提示（可根据硬件按键/旋钮自定义文案）
        menu_command_callback(SHOW_STRING, 5, 48, "Turn to adjust", OLED_8X16);
        
        menu_command_callback(BUFFER_DISPLAY);
        
        InputEvent event = MENU_ReceiveInputEvent();
        if (event.type == INPUT_UP || event.type == INPUT_DOWN)
        {
            // 任意有效输入都视为"活动"，刷新 last_activity_time；若在睡眠则顺便唤醒
            MENU_UpdateActivity(); // 更新活动时间
            
            // 调整睡眠时间
            int16_t wheel_value = (event.type == INPUT_DOWN) ? event.value : -event.value;
            int32_t new_sleep_time = auto_sleep_seconds + (wheel_value * 5); // 每次调整5秒
            
            // 限制范围 (0-300秒)
            if (new_sleep_time < 0) new_sleep_time = 0;
            if (new_sleep_time > 300) new_sleep_time = 300;
            
            auto_sleep_seconds = new_sleep_time;
        }
        
        // 检查退出事件：确认或返回均退出本设置页并保留最新 auto_sleep_seconds
        if (event.type == INPUT_ENTER || event.type == INPUT_BACK)
        {
            MENU_UpdateActivity(); // 更新活动时间
            return;
        }
        osDelay(10);  // 使用 osDelay 让出 CPU
    }
}
/**
 * @brief 系统设置菜单
 */
void MENU_SystemSetting(void)
{
    char auto_sleep_str[32];
    
    menu_command_callback(BUFFER_CLEAR);
    
    
#if AUTO_SLEEP_ENABLED
    if (auto_sleep_seconds == 0) {
        sprintf(auto_sleep_str, "Auto Sleep: OFF");
    } else if (auto_sleep_seconds < 60) {
        sprintf(auto_sleep_str, "Auto Sleep: %ds", auto_sleep_seconds);
    } else {
        uint16_t minutes = auto_sleep_seconds / 60;
        uint16_t seconds = auto_sleep_seconds % 60;
        if (seconds == 0) {
            sprintf(auto_sleep_str, "Auto Sleep: %dm", minutes);
        } else {
            sprintf(auto_sleep_str, "Auto Sleep: %dm%ds", minutes, seconds);
        }
    }
#else
    sprintf(auto_sleep_str, "Auto Sleep: DISABLED");
#endif
    menu_command_callback(SHOW_STRING, 5, 16, auto_sleep_str, OLED_8X16);
    
    menu_command_callback(SHOW_STRING, 5, 32, "Language: ENG", OLED_8X16);
    menu_command_callback(SHOW_STRING, 5, 48, "Press to return", OLED_8X16);
    
    menu_command_callback(BUFFER_DISPLAY);
    
    while (1)
    {
        InputEvent event = MENU_ReceiveInputEvent();
        if (event.type == INPUT_ENTER || event.type == INPUT_BACK)
        {
            MENU_UpdateActivity(); // 仅在有输入时更新活动时间
            return;
        }
        osDelay(10);  // 使用 osDelay 让出 CPU
    }
}

/**
 * @brief 关于设置菜单
 */
void MENU_AboutSetting(void)
{
    menu_command_callback(BUFFER_CLEAR);
    
    menu_command_callback(SHOW_STRING, 20, 0, "About Device", OLED_8X16);
    menu_command_callback(SHOW_STRING, 5, 16, "Version: v2.0", OLED_8X16);
    menu_command_callback(SHOW_STRING, 5, 32, "Build: 2025", OLED_8X16);
    menu_command_callback(SHOW_STRING, 5, 48, "Press to return", OLED_8X16);
    
    menu_command_callback(BUFFER_DISPLAY);
    
    while (1)
    {
        InputEvent event = MENU_ReceiveInputEvent();
        if (event.type == INPUT_ENTER || event.type == INPUT_BACK)
        {
            MENU_UpdateActivity(); // 更新活动时间
            return;
        }
        osDelay(10);  // 使用 osDelay 让出 CPU
    }
}

void MENU_WIFISetting(void)
{
    menu_command_callback(BUFFER_CLEAR);
    menu_command_callback(SHOW_STRING, 20, 0, "WIFI Name", OLED_8X16);
    menu_command_callback(SHOW_STRING, 5, 16, "Version: v2.0", OLED_8X16);
    menu_command_callback(SHOW_STRING, 5, 32, "WIFI Status", OLED_8X16);
    menu_command_callback(SHOW_STRING, 5, 48, "Press to return", OLED_8X16);
    menu_command_callback(BUFFER_DISPLAY);

    while (1)
    {
        InputEvent event = MENU_ReceiveInputEvent();
        if (event.type == INPUT_ENTER || event.type == INPUT_BACK)
        {
            MENU_UpdateActivity(); // 更新活动时间
            return;
        }
        osDelay(10);  // 使用 osDelay 让出 CPU
    }
}

/* 滚动条平滑动画参数（使用菜单系统自带的动画机制） */
static float scrollbar_current_height = 0.0f;      // 当前滚动条高度（平滑值）
static float scrollbar_target_height = 0.0f;       // 目标滚动条高度


void MENU_DrawScrollBar(MENU_HandleTypeDef *hMENU)
{
    // 滚动条配置 - 优化后的设计
    const uint8_t scrollbar_width = 4;      // 滚动条总宽度
    const uint8_t scrollbar_x = 123;        // 滚动条X位置 (右侧)
    const uint8_t scrollbar_y = 3;          // 滚动条起始Y位置
    const uint8_t scrollbar_height = 58;    // 滚动条总高度
    
    // 只有当菜单项超过可显示数量时才显示滚动条
    if (hMENU->Option_Max_i <= CURSOR_CEILING) {
        // 重置动画参数
        scrollbar_current_height = 0.0f;
        scrollbar_target_height = 0.0f;
        return;
    }
    
    // 计算参数
    uint8_t total_items = hMENU->Option_Max_i + 1;     // 总项目数
    
    // 核心：滑块高度直接反映当前选中项在整个列表中的位置比例
    // 完全按照OLED_UI思想：ScrollBarHeight = TotalHeight * (CurrentItem+1) / TotalItems
    float position_ratio = (float)(hMENU->Catch_i + 1) / (float)total_items;
    scrollbar_target_height = scrollbar_height * position_ratio;
    
    // 限制滑块高度范围（保证最小可见性）
    if (scrollbar_target_height < 6.0f) scrollbar_target_height = 6.0f;
    if (scrollbar_target_height > scrollbar_height - 2.0f) {
        scrollbar_target_height = scrollbar_height - 2.0f;
    }
    
    // 使用菜单系统自带的动画机制（与光标动画保持一致）
    scrollbar_current_height = STEPWISE_TO_TARGET(scrollbar_current_height, scrollbar_target_height, ANIMATION_SPEED);
    
    // 绘制滚动条轨道（简洁的细线样式）
    OLED_DrawLine(scrollbar_x + 1, scrollbar_y, scrollbar_x + 1, scrollbar_y + scrollbar_height - 1);
    
    // 计算实际绘制的滑块
    uint8_t thumb_height = (uint8_t)(scrollbar_current_height + 0.5f); // 四舍五入
    if (thumb_height < 1) thumb_height = 1;
    
    // 滑块始终从顶部开始，高度表示位置（完全按照OLED_UI思想）
    OLED_DrawRectangle(scrollbar_x, scrollbar_y, scrollbar_width, thumb_height, OLED_FILLED);
}

/**
 * @brief 绘制进度条
 * @param x 进度条左上角X坐标
 * @param y 进度条左上角Y坐标  
 * @param width 进度条总宽度
 * @param height 进度条高度
 * @param value 当前值 (0-100)
 */
void MENU_DrawProgressBar(int16_t x, int16_t y, uint8_t width, uint8_t height, uint8_t value)
{
    // 限制value范围
    if(value > 100) value = 100;
    
    // 绘制进度条边框
    OLED_DrawRectangle(x, y, width, height, OLED_UNFILLED);
    
    // 计算填充宽度 (减去边框的2像素)
    uint8_t fill_width = ((width - 2) * value) / 100;
    
    // 绘制填充部分
    if(fill_width > 0) {
        OLED_DrawRectangle(x + 1, y + 1, fill_width, height - 2, OLED_FILLED);
    }
}

#if SHOW_FPS
void Update_FPS_Counter(void)
{
    frame_count++;
    uint32_t current_time = HAL_GetTick();
    
    // 每1000ms计算一次FPS
    if(current_time - last_fps_time >= 1000) {
        current_fps = frame_count;
        frame_count = 0;
        last_fps_time = current_time;
    }
}

uint32_t Get_Current_FPS(void)
{
    return current_fps;
}
#endif /* SHOW_FPS */

void CLOCK_Draw(void)
{
    static SNTP_Time_t t;
    static uint8_t has_time = 0;

    if (TimeQueueHandle != NULL) {
        /* 把队列里可能堆积的多个时间全部取出，仅保留最新的一条 */
        while (osMessageQueueGet(TimeQueueHandle, &t, NULL, 0) == osOK) {
            has_time = 1;
        }
    }

    if(has_time) {
        OLED_Clear();
        OLED_ShowString(45, 0, "Clock", OLED_8X16);
        
        // 使用 OLED_ShowNum 直接显示数字
        OLED_ShowNum(0, 16, t.year, 4, OLED_8X16);
        OLED_ShowString(32, 16, "/", OLED_8X16);
        OLED_ShowNum(40, 16, t.month, 2, OLED_8X16);
        OLED_ShowString(56, 16, "/", OLED_8X16);
        OLED_ShowNum(64, 16, t.day, 2, OLED_8X16);
        
        OLED_ShowNum(0, 32, t.hour, 2, OLED_8X16);
        OLED_ShowString(16, 32, ":", OLED_8X16);
        OLED_ShowNum(24, 32, t.minute, 2, OLED_8X16);

        
        OLED_Update();
    } else {
        OLED_Clear();
        OLED_ShowString(10, 24, "Syncing time...", OLED_6X8);
        OLED_Update();
    }
}

