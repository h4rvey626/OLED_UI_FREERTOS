#ifndef __ENCODER_DRIVER_H
#define __ENCODER_DRIVER_H


#include "stm32f4xx_hal.h"
#include "tim.h"


typedef enum
{
    KEY_UNPRESSED ,
    KEY_SHORT_PRESSED ,
    KEY_LONG_PRESSED ,
} KeyPressState;

// EncoderDirection 枚举已移除，请使用 input.h 的 InputType 枚举

void Encoder_Init(void);

KeyPressState KeyPress(void);

// 编码器接口 - 推荐使用 input.h 的 Input_GetEvent() 获取统一事件
int16_t Encoder_Roll(void);      // 获取消抖后的旋转格数
int16_t Encoder_RawDelta(void);  // 获取原始增量（内部使用）

#endif

