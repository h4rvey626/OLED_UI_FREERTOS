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

/* WiFi 默认配置 (可通过 config_store 持久化覆盖) */
#define WIFI_SSID_DEFAULT       "LEDE"
#define WIFI_PASSWORD_DEFAULT   "26524563"

/* MQTT 配置 */
#define MQTT_BROKER             "emqx-prod.neuroncloud.ai"
#define MQTT_PORT               30333
#define MQTT_CLIENT_ID          "harvey69"
#define MQTT_USERNAME           "device"
#define MQTT_PASSWORD           "Neuron@1234"

/* 超时配置 (ms) */
#define TIMEOUT_RST             3000
#define TIMEOUT_GOT_IP          1000
#define TIMEOUT_AT_TEST         1000
#define TIMEOUT_CWMODE          1000
#define TIMEOUT_CWJAP           25000
#define TIMEOUT_MQTT_CFG        2000
#define TIMEOUT_MQTT_CONN       10000
#define TIMEOUT_MQTT_PUB        3000

/* 重试次数配置 */
#define INIT_RETRY_MAX          3
#define WIFI_RETRY_MAX          5
#define MQTT_RETRY_MAX          5


/* MQTT 发布 */
bool WIFI_MQTT_Publish(const char* topic, const char* msg);

/* FreeRTOS 任务入口 */
void WIFITask_Entry(void *argument);

#endif /* INC_WIFI_TASK_H_ */
