/*
 * wifi_task.h
 *
 *  Created on: Jan 4, 2026
 *      Author: 10637
 */

#ifndef INC_WIFI_TASK_H_
#define INC_WIFI_TASK_H_

#include <stdbool.h>

/* WiFi 初始化与连接 */
bool WIFI_App_Init(void);
bool WIFI_App_ConnectWiFi(void);
bool WIFI_App_ConnectMQTT(void);


/* MQTT 发布 */
bool WIFI_MQTT_Publish(const char* topic, const char* msg);

/* FreeRTOS 任务入口 */
void WIFITask_Entry(void *argument);

#endif /* INC_WIFI_TASK_H_ */
