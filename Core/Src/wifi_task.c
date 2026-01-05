/*
 * wifi_task.c
 *
 *  Created on: Dec 30, 2025
 *      Author: 10637
 */

#include "main.h"
#include "cmsis_os.h"
#include "esp_driver.h"
#include "esp_at.h"
#include "OLED.h"
#include "app_events.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Private function prototypes */
static void WIFI_ShowStatus(const char* line1, const char* line2);
static int WIFI_ParseCWJAP_Code(const char* s);
static const char* WIFI_CWJAP_CodeToMsg(int code);

/**
  * @brief  Helper to show status on OLED
  */
static void WIFI_ShowStatus(const char* line1, const char* line2) {
    OLED_Clear();
    if (line1) OLED_ShowString(0, 0, (char*)line1, OLED_8X16);
    if (line2) OLED_ShowString(0, 16, (char*)line2, OLED_8X16);
    OLED_Update();
}

static int WIFI_ParseCWJAP_Code(const char* s) {
    if (!s) return -1;
    const char* p = strstr(s, "+CWJAP:");
    if (!p) return -1;
    p += strlen("+CWJAP:");
    while (*p == ' ' || *p == '\t') p++;
    return (int)strtol(p, NULL, 10);
}

static const char* WIFI_CWJAP_CodeToMsg(int code) {
    // 常见 AT 固件含义：1超时 2密码错 3找不到AP 4连接失败
    switch (code) {
        case 1: return "Timeout(1)";
        case 2: return "BadPwd(2)";
        case 3: return "NoAP(3)";
        case 4: return "ConnFail(4)";
        default: return NULL;
    }
}

/**
  * @brief  WiFi Business Logic Initialization
  */
bool WIFI_App_Init(void) {
    WIFI_ShowStatus("ESP8266", "Resetting...");
    
    // 初始化底层
    ESP_Driver_Init();
    
    // 复位
    ESP_AT_Send("AT+RST");
    osDelay(3000); // 等待复位完成

    WIFI_ShowStatus("ESP8266", "AT Test...");
    if (!ESP_AT_Send("AT")) {
        WIFI_ShowStatus("ESP8266", "No Response");
        return false;
    }

    ESP_AT_Send("ATE0"); // 关闭回显
    osDelay(100);

    if (!ESP_AT_Send("AT+CWMODE=1")) {
        WIFI_ShowStatus("ESP8266", "Mode Fail");
        return false;
    }

    WIFI_ShowStatus("ESP8266", "Ready");
    return true;
}

/**
  * @brief  Connect to WiFi and MQTT
  */
bool WIFI_App_Connect(void) {
    // 1. 连接 WiFi
    WIFI_ShowStatus("WiFi", "Connecting...");
    // 注意：增加超时时间
    if (!ESP_AT_SendTimeout("AT+CWJAP=\"RT-AC1200_2.4G\",\"Neuron@1234\"", 25000)) {
        const char* rx = ESP_AT_GetLastRxSnippet();
        char line2[17];
        memset(line2, 0, sizeof(line2));
        if (rx && rx[0]) {
            int code = WIFI_ParseCWJAP_Code(rx);
            const char* msg = WIFI_CWJAP_CodeToMsg(code);
            if (msg) {
                strncpy(line2, msg, 16);
            } else {
                // OLED 8x16 一行大约 16 个字符，截断显示关键片段
                strncpy(line2, rx, 16);
            }
            WIFI_ShowStatus("WiFi Fail", line2);
        } else {
            WIFI_ShowStatus("WiFi", "No Reply");
        }
        return false;
    }
    WIFI_ShowStatus("WiFi", "Connected");
    osDelay(1000);

    // 2. 配置 MQTT 用户
    WIFI_ShowStatus("MQTT", "Config...");
    if (!ESP_AT_SendTimeout("AT+MQTTUSERCFG=0,1,\"harvey69\",\"device\",\"Neuron@1234\",0,0,\"\"", 2000)) {
         WIFI_ShowStatus("MQTT", "Cfg Fail");
         return false;
    }

    // 3. 连接 MQTT Broker
    WIFI_ShowStatus("MQTT", "Connecting...");
    if (!ESP_AT_SendTimeout("AT+MQTTCONN=0,\"emqx-prod.neuroncloud.ai\",30333,1", 10000)) {
        WIFI_ShowStatus("MQTT", "Conn Fail");
        return false;
    }

    WIFI_ShowStatus("MQTT", "Connected!");
    return true;
}

/**
  * @brief  Publish MQTT Message
  */
bool WIFI_MQTT_Publish(const char* topic, const char* msg) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "AT+MQTTPUB=0,\"%s\",\"%s\",0,0", topic, msg);
    
    if (ESP_AT_SendTimeout(cmd, 3000)) {
        // 可以在这里加一个闪烁或者日志
        return true;
    }
    return false;
}

/**
  * @brief  Main Task Function (Called by FreeRTOS)
  */
void WIFITask_Entry(void *argument) {
    (void)argument;

    /* 启动阶段只跑一次：有限重试，避免卡住菜单 */
    const uint32_t INIT_RETRY_MAX = 3;
    const uint32_t CONN_RETRY_MAX = 5;

    bool ok = false;

    // 初始化重试
    for (uint32_t i = 0; i < INIT_RETRY_MAX; i++) {
        if (WIFI_App_Init()) {
            ok = true;
            break;
        }
        osDelay(2000);
    }

    // 连接重试
    if (ok) {
        ok = false;
        for (uint32_t i = 0; i < CONN_RETRY_MAX; i++) {
            if (WIFI_App_Connect()) {
                ok = true;
                break;
            }
            osDelay(5000);
        }
    }

    if (ok) {
        WIFI_MQTT_Publish("RADAR", "Start success");
        osDelay(50);
        WIFI_MQTT_Publish("RADAR", "System Online");
    } else {
        WIFI_ShowStatus("WiFi", "Skip/Fail");
    }

    /* 通知菜单：WiFi 启动阶段已结束 */
    if (g_appEventFlags) {
        uint32_t set = APP_EVT_WIFI_DONE | (ok ? APP_EVT_WIFI_OK : 0U);
        osEventFlagsSet(g_appEventFlags, set);
    }

    /* 任务只运行一次，结束 */
    osThreadExit();
}
