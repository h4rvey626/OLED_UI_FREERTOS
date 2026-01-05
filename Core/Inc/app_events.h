/*
 * app_events.h
 *
 *  Created on: Jan 4, 2026
 *      Author: 10637
 *
 */

#ifndef __APP_EVENTS_H
#define __APP_EVENTS_H

#include "cmsis_os.h"

/* 全局事件标志句柄（在 freertos.c 中创建） */
extern osEventFlagsId_t g_appEventFlags;

/* WiFi 事件位定义 */
#define APP_EVT_WIFI_DONE   (1U << 0)   /* WiFi 任务已结束（成功/失败都会置位） */
#define APP_EVT_WIFI_OK     (1U << 1)   /* WiFi 已成功连接（仅成功置位） */

/* SNTP 事件位定义 */
#define APP_EVT_SNTP_DONE   (1U << 2)   /* SNTP 配置已结束（成功/失败都会置位） */
#define APP_EVT_SNTP_OK     (1U << 3)   /* SNTP 配置成功（仅成功置位） */

#endif /* __APP_EVENTS_H */
