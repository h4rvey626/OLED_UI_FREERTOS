/*
 * Input.h
 *
 *  Created on: Dec 29, 2025
 *      Author: 10637
 */

#ifndef INC_INPUT_H_
#define INC_INPUT_H_

#include "TIM_EC11.h"   
#include <stdint.h>

// 定义事件类型
typedef enum {
    INPUT_NONE = 0,
    INPUT_UP,      // 对应编码器逆时针
    INPUT_DOWN,    // 对应编码器顺时针
    INPUT_BACK,    // 对应按键长按
    INPUT_ENTER    // 对应按键短按
} InputType;

// 定义包含数据的事件结构体
typedef struct {
    InputType type;  // 事件类型
    int16_t value;   // 事件数据 (编码器旋转的格数，按键通常为1)
} InputEvent;

InputEvent Input_GetEvent(void);

#endif /* INC_INPUT_H_ */
