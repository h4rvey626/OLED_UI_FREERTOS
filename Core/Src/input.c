/*
 * input.c
 *
 *  Created on: Dec 29, 2025
 *      Author: 10637
 */
#include "input.h"
#include "encoder_driver.h"

InputEvent Input_GetEvent(void) // 获取输入事件
{
    InputEvent event;
    // 初始化默认值
    event.type = INPUT_NONE;
    event.value = 0;

    // 1. 处理按键
    KeyPressState key_press = KeyPress();
    if(key_press == KEY_SHORT_PRESSED)
    {
        event.type = INPUT_ENTER;
        event.value = 1;
        return event; // 优先返回按键
    }
    else if(key_press == KEY_LONG_PRESSED)
    {
        event.type = INPUT_BACK;
        event.value = 2;
        return event;
    }

    // 2. 处理编码器
    // 直接调用 Encoder_Roll 获取原始增量 (正数=CW, 负数=CCW)
    int16_t roll = Encoder_Roll(); 

    if(roll != 0)
    {
        if(roll > 0)
        {
            event.type = INPUT_DOWN; // 顺时针 -> 向下/增加
            event.value = roll;      // 记录转了多少格 (正数)
        }
        else
        {
            event.type = INPUT_UP;   // 逆时针 -> 向上/减少
            event.value = -roll;     // 记录转了多少格 (取反变正数，方便上层使用)
        }
    }

    return event;
}
