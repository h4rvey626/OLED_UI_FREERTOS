/*
 * time_task.c
 *
 *  Created on: Jan 4, 2026
 *      Author: 10637
 */

#include "time_task.h"
#include "esp_at.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "MENU.h"
#include "OLED.h"

extern osMessageQueueId_t InputEventQueueHandle;

/* ========= 倒计时功能相关变量（从 MENU.c 迁移到此） ========= */
uint8_t timer_enabled = 0;               // 定时器启用标志 (0=关闭, 1=启用)
uint16_t timer_seconds = 60;             // 定时器时间(秒)
uint16_t timer_current = 0;              // 当前定时器剩余时间
uint32_t timer_start_time = 0;           // 定时器开始时间

/**
 * @brief 启动定时器
 * @param seconds 定时器秒数
 */
void MENU_StartTimer(uint16_t seconds)
{
    timer_enabled = 1;
    timer_seconds = seconds;
    timer_current = seconds;
    timer_start_time = HAL_GetTick();
}

/**
 * @brief 停止定时器
 */
void MENU_StopTimer(void)
{
    timer_enabled = 0;
    timer_current = 0;
}

/**
 * @brief 更新定时器状态
 * @return 1表示定时器结束，0表示继续
 */
uint8_t MENU_UpdateTimer(void)
{
    if (!timer_enabled) return 0;

    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed_seconds = (current_time - timer_start_time) / 1000;

    if (elapsed_seconds >= timer_seconds) {
        // 定时器结束
        timer_enabled = 0;
        timer_current = 0;
        return 1;
    }

    timer_current = timer_seconds - elapsed_seconds;
    return 0;
}

/**
 * @brief 显示倒计时
 * @param x X坐标
 * @param y Y坐标
 */
void MENU_ShowTimer(int16_t x, int16_t y)
{
    if (!timer_enabled) return;

    char timer_str[16];
    uint16_t minutes = timer_current / 60;
    uint16_t seconds = timer_current % 60;

    if (minutes > 0) {
        sprintf(timer_str, "%02d:%02d", minutes, seconds);
    } else {
        sprintf(timer_str, "00:%02d", seconds);
    }

    menu_command_callback(SHOW_STRING, x, y, timer_str, OLED_6X8); // 使用小字体显示定时器
}

/**
 * @brief 显示"时间到"提示
 */
void MENU_ShowTimeUpAlert(void)
{
    // 清除屏幕
    menu_command_callback(BUFFER_CLEAR);

    // 显示"时间到"提示
    menu_command_callback(SHOW_STRING, 32, 20, "TIME UP!", OLED_8X16);

    // 绘制简洁边框
    OLED_DrawRectangle(5, 15, 118, 35, OLED_UNFILLED);

    menu_command_callback(BUFFER_DISPLAY);

    // 闪烁提示效果（减少闪烁次数）
    for (int i = 0; i < 2; i++) {
        osDelay(300);  // 使用 osDelay 让出 CPU

        // 反转显示区域创建闪烁效果
        OLED_ReverseArea(6, 16, 116, 33);
        OLED_Update();

        osDelay(300);  // 使用 osDelay 让出 CPU

        // 恢复正常显示
        OLED_ReverseArea(6, 16, 116, 33);
        OLED_Update();
    }

    // 等待按键确认
    while (1)
    {
        InputEvent event = MENU_ReceiveInputEvent();
        if (event.type == INPUT_ENTER ||
            event.type == INPUT_BACK ||
            event.type == INPUT_UP ||
            event.type == INPUT_DOWN)
        {
            MENU_UpdateActivity(); // 更新活动时间
            break;
        }
        osDelay(50);  // 使用 osDelay 让出 CPU
    }
}

/* 月份字符串转数字 */
static uint8_t parse_month(const char* mon) {
    static const char* months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    for (int i = 0; i < 12; i++) {
        if (strncmp(mon, months[i], 3) == 0) {
            return (uint8_t)(i + 1);
        }
    }
    return 0;
}

/**
 * @brief  配置 SNTP
 * @retval true 配置成功, false 失败
 */
bool SNTP_Config(void) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CIPSNTPCFG=1,%d,\"%s\"", SNTP_TIMEZONE, SNTP_SERVER);       //TCP 连接NTP服务器
    
    if (!ESP_AT_SendWaitFor(cmd, "OK", 3000)) {
        return false;
    }
    
    // 等待 SNTP 同步（首次同步可能需要几秒）
    osDelay(2000);
    return true;
}

/**
 * @brief  从网络获取当前时间
 * @param  time: 输出时间结构体指针
 * @retval true 获取成功, false 失败
 * 
 * 响应格式: +CIPSNTPTIME:Sun Jan  4 16:34:45 2026\r\nOK
 */
bool SNTP_GetTime(SNTP_Time_t* time) {
    if (!time) return false;
    
    // 使用 ESP_AT_SendWaitFor 发送命令并等待 OK（确保收到完整响应）
    if (!ESP_AT_SendWaitFor("AT+CIPSNTPTIME?", "OK", 3000)) {
        return false;
    }
    
    // 获取响应内容并解析
    const char* rx = ESP_AT_GetLastRxSnippet();
    if (!rx) return false;
    
    // 查找 +CIPSNTPTIME: 后的内容
    const char* p = strstr(rx, "+CIPSNTPTIME:");
    if (!p) return false;
    p += 13; // 跳过 "+CIPSNTPTIME:"
    
    // 跳过前导空格
    while (*p == ' ') p++;
    
    char weekday[4] = {0};
    char month[4] = {0};
    int day, hour, minute, second, year;
    
    // 使用 sscanf 解析
    int parsed = sscanf(p, "%3s %3s %d %d:%d:%d %d",
                        weekday, month, &day, &hour, &minute, &second, &year);
    
    if (parsed != 7) {
        return false;
    }
    
    // 填充结构体
    strncpy(time->weekday, weekday, 3);
    time->weekday[3] = '\0';
    time->year = (uint16_t)year;
    time->month = parse_month(month);
    time->day = (uint8_t)day;
    time->hour = (uint8_t)hour;
    time->minute = (uint8_t)minute;
    time->second = (uint8_t)second;
    
    return (time->month != 0);  // 月份解析失败返回 false
}

/**
 * @brief  获取时间并格式化为字符串
 * @param  buf: 输出缓冲区
 * @param  buf_size: 缓冲区大小
 * @retval true 成功, false 失败
 */
bool SNTP_GetTimeString(char* buf, size_t buf_size) {
    if (!buf || buf_size < 20) return false;
    
    SNTP_Time_t time;
    if (!SNTP_GetTime(&time)) {
        return false;
    }
    
    // 格式化为 "2026-01-04 16:34:45"
    snprintf(buf, buf_size, "%04d-%02d-%02d %02d:%02d:%02d",
             time.year, time.month, time.day,
             time.hour, time.minute, time.second);
    
    return true;
}

/**
 * @brief 定时器设置菜单 - 标准菜单格式
 */
void MENU_TimerSetting(void)
{

    static MENU_OptionTypeDef MENU_OptionList[] = {
        {"<<<", NULL},                    // 返回
        {"Time: 60s", NULL},              // 时间调节
        {"Start Timer", NULL},            // 启动定时器
        {"Stop Timer", NULL},             // 停止定时器
        {"..", NULL}                      // 结束标记
    };

    static MENU_HandleTypeDef MENU = {
        .OptionList = MENU_OptionList,
        .Option_Max_i = 3,                // 手动设置最大索引值（不包括".."）
        .isRun = 1                        // 默认运行状态为开启
    };

    // 动态字符串缓冲区
    static char time_str[32];
    static char start_str[32];
    static char stop_str[32] = "Stop Timer";

    // 时间调节模式标志
    static uint8_t time_adjust_mode = 0;  // 0=正常模式, 1=时间调节模式

    // 手动更新菜单选项内容
    while (1)
    {
        // 静态变量跟踪上次的状态和时间值，避免不必要的更新
        static uint8_t last_time_adjust_mode = 0xFF; // 初始无效值
        static uint16_t last_seconds_shown = 0xFFFF; // 初始无效值
        static uint8_t last_selection = 0xFF;        // 初始无效值

        // 只有在时间值、调整模式或选中项变化时才更新字符串
    if (last_seconds_shown != timer_seconds ||
            last_time_adjust_mode != time_adjust_mode ||
            last_selection != MENU.Catch_i) {

            last_seconds_shown = timer_seconds;
            last_time_adjust_mode = time_adjust_mode;
            last_selection = MENU.Catch_i;

            // 更新时间显示，如果在时间调节模式则添加箭头标识
            if (time_adjust_mode && MENU.Catch_i == 1) {
                // 在时间调节模式下显示箭头
                if (timer_seconds < 60) {
                    sprintf(time_str, "Time: %ds >", timer_seconds);
                } else {
                    uint16_t minutes = timer_seconds / 60;
                    uint16_t seconds = timer_seconds % 60;
                    if (seconds == 0) {
                        sprintf(time_str, "Time: %dm >", minutes);
                    } else {
                        sprintf(time_str, "Time: %dm%ds >", minutes, seconds);
                    }
                }
            } else {
                // 正常模式下不显示箭头
                if (timer_seconds < 60) {
                    sprintf(time_str, "Time: %ds", timer_seconds);
                } else {
                    uint16_t minutes = timer_seconds / 60;
                    uint16_t seconds = timer_seconds % 60;
                    if (seconds == 0) {
                        sprintf(time_str, "Time: %dm", minutes);
                    } else {
                        sprintf(time_str, "Time: %dm%ds", minutes, seconds);
                    }
                }
            }
        }

        // 启动按钮显示（只有状态变化或定时器变化时才更新）
        static uint8_t last_timer_enabled = 0xFF; // 初始无效值
        static uint16_t last_timer_value = 0xFFFF; // 初始无效值

        // 仅当状态变化或定时器变化时才更新
    if (last_timer_enabled != timer_enabled ||
            (timer_enabled && last_timer_value != timer_current)) {

            last_timer_enabled = timer_enabled;
            last_timer_value = timer_current;

            if (timer_enabled) {
                sprintf(start_str, "Running (%ds)", timer_current);
            } else {
                sprintf(start_str, "Start Timer");
            }
        }

        // 更新选项字符串指针 - 这些指针赋值操作性能开销很小，可以保留
        MENU_OptionList[1].String = time_str;
        MENU_OptionList[2].String = start_str;
        MENU_OptionList[3].String = stop_str;

    // 仅在字符串内容实际发生变化时才更新光标（减少动画触发次数）
        // 使用静态数组保存上次的字符串长度
        static uint8_t last_str_lengths[4] = {0};
        static uint8_t need_update = 0;

        // 只计算当前选中项的长度，避免不必要的计算
        uint8_t current_item = MENU.Catch_i;
        if (current_item <= 3) {
            uint8_t current_length = 0;

            // 根据选中项计算长度
            switch (current_item) {
                case 1:
                    current_length = strlen(time_str);
                    break;
                case 2:
                    current_length = strlen(start_str);
                    break;
                case 3:
                    current_length = strlen(stop_str);
                    break;
                default:
                    current_length = strlen(MENU_OptionList[0].String);
                    break;
            }

            // 只有长度变化时才触发更新
            if (last_str_lengths[current_item] != current_length) {
                last_str_lengths[current_item] = current_length;
                MENU.OptionList[current_item].StrLen = current_length;
                MENU.AnimationUpdateEvent = 1;
                need_update = 1;
            }
        }

        // 仅在第一次循环时初始化菜单
        static uint8_t first_run = 1;
    if (first_run) {
            // 手动初始化关键字段，避免使用MENU_HandleInit以保留状态
            MENU.AnimationUpdateEvent = 1; // 动画更新事件
            MENU.isRun = 1;                // 运行标志
            MENU.Option_Max_i = 3;         // 选项列表长度（不包括".."）
            MENU.Catch_i = 1;              // 选中下标默认为1,(因为MENU.OptionList[0]为"<<<")
            MENU.Cursor_i = 1;             // 光标下标对应选中项
            MENU.Show_i = 0;               // 显示(遍历)起始下标
            MENU.Show_i_Previous = 0;      // 上一次循环的显示下标
            MENU.Wheel_Event = 0;          // 初始化滚轮事件

            first_run = 0;
        }

        // 更新定时器状态
        if (MENU_UpdateTimer()) {
            // 定时器结束，显示"时间到"提示
            MENU_ShowTimeUpAlert();
        } else if (timer_enabled) {
            // 定时器运行中，每250ms更新一次显示（降低更新频率提高性能）
            static uint16_t last_timer_shown = 0;
            static uint32_t last_update_time = 0;
            uint32_t current_time = HAL_GetTick();

            // 降低更新频率至250ms一次，大幅提升性能
            if ((current_time - last_update_time >= 250) && (last_timer_shown != timer_current)) {
                last_timer_shown = timer_current;
                last_update_time = current_time;

                // 更新启动按钮显示内容
                sprintf(start_str, "Running (%ds)", timer_current);
                MENU_OptionList[2].String = start_str;

                // 仅当当前选中时更新光标宽度
                if (MENU.Catch_i == 2) {
                    MENU.OptionList[2].StrLen = strlen(start_str);
                    MENU.AnimationUpdateEvent = 1;
                }
            }
        }

        /* ========= 使用 FreeRTOS 输入事件队列替代 menu_command_callback(GET_EVENT_*) ========= */
        if (InputEventQueueHandle)
        {
            InputEvent evt;
            /* 一帧内把队列事件全部消费掉，按产生顺序处理 */
            while (osMessageQueueGet(InputEventQueueHandle, &evt, NULL, 0) == osOK)
            {
                switch (evt.type)
                {
                    case INPUT_ENTER: /* 确定 */
                    {
                        MENU_UpdateActivity();

                        switch (MENU.Catch_i)
                        {
                            case 0: // 返回
                                time_adjust_mode = 0; // 退出时重置模式
                                return;

                            case 1: // 时间调节 - 切换时间调节模式
                                time_adjust_mode = !time_adjust_mode;
                                break;

                            case 2: // 启动定时器
                                if (!timer_enabled) {
                                    MENU_StartTimer(timer_seconds);
                                    // 更新启动按钮显示内容
                                    sprintf(start_str, "Running (%ds)", timer_current);
                                    MENU_OptionList[2].String = start_str;
                                    // 更新字符串长度并触发光标更新
                                    MENU.OptionList[2].StrLen = strlen(start_str);
                                    MENU.AnimationUpdateEvent = 1;
                                }
                                time_adjust_mode = 0; // 启动定时器后退出调节模式
                                break;

                            case 3: // 停止定时器
                                if (timer_enabled) {
                                    MENU_StopTimer();
                                    // 更新启动按钮显示内容
                                    sprintf(start_str, "Start Timer");
                                    MENU_OptionList[2].String = start_str;
                                    // 更新字符串长度并触发光标更新
                                    MENU.OptionList[2].StrLen = strlen(start_str);
                                    MENU.AnimationUpdateEvent = 1;
                                }
                                break;
                        }
                    }
                    break;

                    case INPUT_BACK: /* 返回 */
                    {
                        MENU_UpdateActivity();
                        // 如果在时间调节模式，先退出调节模式
                        if (time_adjust_mode) {
                            time_adjust_mode = 0;
                        } else {
                            return;
                        }
                    }
                    break;

                    case INPUT_UP:   /* 编码器逆时针 */
                    case INPUT_DOWN: /* 编码器顺时针 */
                    {
                        /* 统一成 wheel_event：正数=向下/增加，负数=向上/减少 */
                        int16_t wheel_event = 0;
                        if (evt.type == INPUT_DOWN) {
                            wheel_event = evt.value;
                        } else {
                            wheel_event = -evt.value;
                        }

                        if (wheel_event)
                        {
                            MENU_UpdateActivity();

                            // 如果在时间调节模式且选中时间调节项，则调节时间
                            if (time_adjust_mode && MENU.Catch_i == 1)
                            {
                                // 基于当前时间值优化调整步长
                                int16_t step = 5; // 默认步长5秒

                                // 时间越大，步长越大，提升用户体验
                                if (timer_seconds >= 300) {       // >5分钟
                                    step = 30;                    // 30秒步长
                                } else if (timer_seconds >= 120) { // >2分钟
                                    step = 15;                    // 15秒步长
                                } else if (timer_seconds >= 60) {  // >1分钟
                                    step = 10;                    // 10秒步长
                                }

                                int32_t new_timer_time = timer_seconds + (wheel_event * step);

                                // 限制范围 (5-600秒 = 10分钟)
                                if (new_timer_time < 5) new_timer_time = 5;
                                if (new_timer_time > 600) new_timer_time = 600;

                                // 只有时间真的变化时才更新
                                if (timer_seconds != new_timer_time) {
                                    timer_seconds = new_timer_time;
                                    // 让静态变量追踪系统识别到时间变化
                                    last_seconds_shown = 0xFFFF; // 强制下次循环更新时间显示
                                }
                            }
                            else
                            {
                                // 正常菜单滚动模式：交给 MENU_UpdateIndex 统一处理循环、显示窗口等
                                MENU.Wheel_Event = wheel_event;
                                MENU_UpdateIndex(&MENU);
                                MENU.Wheel_Event = 0;
                                MENU.AnimationUpdateEvent = 1;
                            }
                        }
                    }
                    break;

                    default:
                        break;
                }
            }
        }

        // 检查自动息屏（仅在无输入时会逐步进入睡眠）
        MENU_CheckAutoSleep();

        // 优化：只在必要时更新长度，使用静态变量跟踪是否需要完全更新
        static uint8_t full_update_needed = 1; // 第一次需要完全更新
        static uint8_t update_counter = 0;     // 计数器，定期完全更新

        if (full_update_needed || need_update || ++update_counter >= 10) {
            // 仅更新当前显示的项目长度，减少计算
            int start_i = MENU.Show_i;
            int end_i = MENU.Show_i + CURSOR_CEILING + 1;

            if (end_i > MENU.Option_Max_i) end_i = MENU.Option_Max_i;
            if (start_i < 0) start_i = 0;

            for (int i = start_i; i <= end_i; i++) {
                MENU.OptionList[i].StrLen = MENU_ShowOption(0, 0, &MENU.OptionList[i]);
            }

            // 已经更新，重置标志
            full_update_needed = 0;
            need_update = 0;
            update_counter = 0;
        }

        // 如果屏幕未睡眠，则正常显示菜单
    if (!screen_sleeping) {
            menu_command_callback(BUFFER_CLEAR);

            MENU_ShowOptionList(&MENU);
            MENU_ShowCursor(&MENU);
            MENU_DrawScrollBar(&MENU);

            menu_command_callback(BUFFER_DISPLAY);
        }

        // 动态调整延时，减少CPU占用
        static uint32_t last_frame_time = 0;
        uint32_t current_time = HAL_GetTick();
        uint32_t frame_time = current_time - last_frame_time;

        // 目标帧率约40FPS (25ms/帧)
        if (frame_time < 20) {
            osDelay(23 - frame_time); // RTOS延时让出CPU
        } else {
            osDelay(1); // 最小延时保证其他任务能执行
        }

        last_frame_time = HAL_GetTick();
    }
}
