/*
 * time_task.h
 *
 *  Created on: Jan 4, 2026
 *      Author: 10637
 */

#ifndef INC_TIME_TASK_H_
#define INC_TIME_TASK_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* SNTP 配置 */
#define SNTP_TIMEZONE       8                    // 时区：东八区（中国）
#define SNTP_SERVER         "ntp.aliyun.com"     // NTP 服务器

/* 时间结构体 */
typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    char     weekday[4];  // "Sun", "Mon", etc.
} SNTP_Time_t;

/* 函数声明 */
/**
 * @brief  配置 SNTP（需在 WiFi 连接后调用）
 * @retval true 配置成功, false 失败
 */
bool SNTP_Config(void);

/**
 * @brief  从网络获取当前时间
 * @param  time: 输出时间结构体指针
 * @retval true 获取成功, false 失败
 */
bool SNTP_GetTime(SNTP_Time_t* time);

/**
 * @brief  获取时间并格式化为字符串
 * @param  buf: 输出缓冲区
 * @param  buf_size: 缓冲区大小
 * @retval true 成功, false 失败
 */
bool SNTP_GetTimeString(char* buf, size_t buf_size);



#endif /* INC_TIME_TASK_H_ */
