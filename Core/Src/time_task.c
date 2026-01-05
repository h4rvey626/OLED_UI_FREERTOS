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
