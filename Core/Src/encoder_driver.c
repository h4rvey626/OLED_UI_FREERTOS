/*
 * encoder_driver.c
 *
 *  Created on: Dec 22, 2025
 *      Author: 10637
 */
#include "encoder_driver.h"
#include "tim.h"

#define KEY_DEBOUNCE_MS  (20U)
#define KEY_LONG_MS      (800U)

// ===== 编码器消抖参数 =====

// 设为1：每1个有效计数输出一格（如果仍有抖动可尝试改为2）
#define ENCODER_PULSE_PER_DETENT  (1)
// 起步确认窗口（ms）：在编码器从“静止→开始转动”的最初窗口内，先积分确认方向再输出，避免第一拍抖动导致反向输出
#define ENCODER_START_DEBOUNCE_MS (8)
// 方向锁定超时（ms）：在此时间内反向脉冲会被忽略（用于抑制转动过程中的抖动反跳）
#define ENCODER_DIR_LOCK_MS       (40)
// 最小有效间隔（ms）：两次有效输出之间的最小间隔
#define ENCODER_MIN_INTERVAL_MS   (2)

void Encoder_Init(void)
{
    HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL);

    __HAL_TIM_SET_COUNTER(&htim1, 0);
}

KeyPressState KeyPress(void)
{
    // 约定：GPIO 低电平=按下（上拉输入）
    const uint8_t raw_level = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_SET) ? 0 : 1;
    const uint32_t now = HAL_GetTick();

    // 去抖：检测输入稳定 KEY_DEBOUNCE_MS 之后才算有效变化
    static uint8_t last_level = 1;
    static uint8_t stable_level = 1;
    static uint32_t last_change_tick = 0;

    // 按键状态跟踪
    static uint8_t pressed = 0;
    static uint32_t press_tick = 0;
    static uint8_t long_reported = 0;

    if (raw_level != last_level)
    {
        last_level = raw_level;
        last_change_tick = now;
    }

    // 输入稳定足够长时间后，确认一次“有效跳变”
    if (((uint32_t)(now - last_change_tick)) >= KEY_DEBOUNCE_MS && stable_level != last_level)
    {
        stable_level = last_level;

        if (stable_level == 0)
        {
            // 进入按下
            pressed = 1;
            press_tick = now;
            long_reported = 0;
        }
        else
        {
            // 进入松开：若之前没有触发长按，则判定短按事件
            if (pressed)
            {
                pressed = 0;
                if (!long_reported)
                {
                    return KEY_SHORT_PRESSED;
                }
            }
        }
    }

    // 长按：按住达到阈值就上报一次
    if (pressed && !long_reported && ((uint32_t)(now - press_tick)) >= KEY_LONG_MS)
    {
        long_reported = 1;
        return KEY_LONG_PRESSED;
    }

    return KEY_UNPRESSED;
}

/**
 * @brief 获取编码器原始增量（无消抖）
 * @return 原始脉冲增量
 */
int16_t Encoder_RawDelta(void)
{
    static int8_t inited = 0;
    static int16_t last = 0;

    const int16_t now = (int16_t)__HAL_TIM_GET_COUNTER(&htim1);
    if (!inited)
    {
        inited = 1;
        last = now;
        return 0;
    }

    const int16_t delta = (int16_t)(now - last);
    last = now;
    return delta;
}

/**
 * @brief 编码器消抖读取
 * @return 消抖后的"格数"变化（正=顺时针，负=逆时针，0=无变化）
 * 
 * 消抖策略：
 * 1. 累积阈值：累积脉冲达到 ENCODER_PULSE_PER_DETENT 才输出一格
 * 2. 方向锁定：一旦确定方向，在 ENCODER_DIR_LOCK_MS 内忽略反向脉冲
 * 3. 最小间隔：两次有效输出间隔不小于 ENCODER_MIN_INTERVAL_MS
 */
int16_t Encoder_Roll(void)
{
    static int16_t accumulator = 0;         // 脉冲累积器
    static int8_t  locked_dir = 0;          // 锁定方向：1=正向, -1=反向, 0=未锁定
    static int8_t start_window = 0;       // 起步确认窗口标志
    static int32_t start_tick = 0;        // 起步窗口起始时刻
    static int32_t dir_tick = 0;          // 最近一次“同方向”脉冲时刻（用于锁定超时）
    static int32_t last_output_tick = 0;  // 上次有效输出时刻

    const int32_t now = HAL_GetTick();
    const int16_t raw_delta = Encoder_RawDelta();

    // 1) 锁定超时：超过锁定时间未出现“同方向”脉冲，则认为进入静止/或用户已反向，解除锁定并清空累积
    if (locked_dir != 0 && ((int32_t)(now - dir_tick)) > ENCODER_DIR_LOCK_MS)
    {
        locked_dir = 0;
        accumulator = 0;
        start_window = 0;
    }

    // 2) 没有新脉冲：仅处理“起步窗口到期”的结算
    if (raw_delta == 0)
    {
        if (start_window && ((int32_t)(now - start_tick)) >= ENCODER_START_DEBOUNCE_MS)
        {
            // 起步窗口结束：用积分后的净增量判定方向（避免第一拍抖动导致反向输出）
            start_window = 0;
            if (accumulator > 0)
            {
                locked_dir = 1;
            }
            else if (accumulator < 0)
            {
                locked_dir = -1;
            }
            else
            {
                // 窗口内正反相互抵消：视为抖动，无事件
                locked_dir = 0;
                accumulator = 0;
                return 0;
            }
            dir_tick = now;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        // 3) 有新脉冲：未锁定时先进入“起步窗口”积分确认方向；已锁定则只接受同方向脉冲
        if (locked_dir == 0)
        {
            if (!start_window)
            {
                start_window = 1;
                start_tick = now;
                accumulator = 0;
            }

            // 起步窗口内：允许正反都累积，让抖动自然抵消
            accumulator += raw_delta;

            // 窗口未结束先不输出（避免第一拍立刻输出反向）
            if (((int32_t)(now - start_tick)) < ENCODER_START_DEBOUNCE_MS)
            {
                return 0;
            }

            // 窗口到期：锁定方向并继续进入输出阶段
            start_window = 0;
            if (accumulator > 0)
            {
                locked_dir = 1;
            }
            else if (accumulator < 0)
            {
                locked_dir = -1;
            }
            else
            {
                locked_dir = 0;
                accumulator = 0;
                return 0;
            }
            dir_tick = now;
        }
        else
        {
            const int8_t cur_dir = (raw_delta > 0) ? 1 : -1;
            if (cur_dir == locked_dir)
            {
                accumulator += raw_delta;
                dir_tick = now; // 同方向脉冲刷新锁定时间
            }
            else
            {
                // 反方向：锁定期内忽略（抖动/反跳）。若确实反向，锁定会因超时自动释放。
                return 0;
            }
        }
    }

    // 4) 输出限速：不丢脉冲，只是在过密时先不输出（让累积器保留到下一次输出）
    if (((int32_t)(now - last_output_tick)) < ENCODER_MIN_INTERVAL_MS)
    {
        return 0;
    }

    // 5) 根据阈值输出“格数”
    int16_t output = 0;
    while (accumulator >= ENCODER_PULSE_PER_DETENT)
    {
        output += 1;
        accumulator -= ENCODER_PULSE_PER_DETENT;
    }
    while (accumulator <= -ENCODER_PULSE_PER_DETENT)
    {
        output -= 1;
        accumulator += ENCODER_PULSE_PER_DETENT;
    }

    if (output != 0)
    {
        last_output_tick = now;
    }

    return output;
} 

