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

/* RST 后短窗口监听到 “WIFI GOT IP” 的标志：用于跳过 CWJAP */
static bool GOT_IP = false;

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
    GOT_IP = false;
    // 初始化底层
    ESP_Driver_Init();
    if (!ESP_AT_SendWaitFor("AT+RST", "ready", 3000)) {
        /* 个别固件/串口噪声下可能匹配不到 ready，兜底延时避免后续 AT 立刻失败 */
        osDelay(3000);
    }
    if (ESP_AT_WaitFor("WIFI GOT IP", 1000)) {
        GOT_IP = true;
    }

    WIFI_ShowStatus("ESP8266", "AT Test...");
    if (!ESP_AT_SendWaitFor("AT", "OK", 1000)) {
        WIFI_ShowStatus("ESP8266", "No Response");
        return false;
    }

    (void)ESP_AT_SendWaitFor("ATE0", "OK", 1000); // 关闭回显（失败不致命）
    osDelay(100);

    if (!ESP_AT_SendWaitFor("AT+CWMODE=1", "OK", 1000)) {
        WIFI_ShowStatus("ESP8266", "Mode Fail");
        return false;
    }

    WIFI_ShowStatus("ESP8266", "Ready");
    return true;
}

/**
  * @brief  Connect to WiFi only
  */
bool WIFI_App_ConnectWiFi(void) {
    WIFI_ShowStatus("WiFi", "Connecting...");
    // 注意：增加超时时间
    if (!ESP_AT_SendWaitFor("AT+CWJAP=\"RT-AC1200_2.4G\",\"Neuron@1234\"", "OK", 25000)) {
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
    osDelay(500);
    return true;
}

/**
  * @brief  Connect to MQTT only (assumes WiFi is connected)
  */
bool WIFI_App_ConnectMQTT(void) {
    // 1. 配置 MQTT 用户
    WIFI_ShowStatus("MQTT", "Config...");
    if (!ESP_AT_SendWaitFor("AT+MQTTUSERCFG=0,1,\"harvey69\",\"device\",\"Neuron@1234\",0,0,\"\"", "OK", 2000)) {
         WIFI_ShowStatus("MQTT", "Cfg Fail");
         return false;
    }

    // 2. 连接 MQTT Broker
    WIFI_ShowStatus("MQTT", "Connecting...");
    if (!ESP_AT_SendWaitFor("AT+MQTTCONN=0,\"emqx-prod.neuroncloud.ai\",30333,1", "OK", 10000)) {
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
    
    if (ESP_AT_SendWaitFor(cmd, "OK", 3000)) {
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
    const uint32_t WIFI_RETRY_MAX = 5;
    const uint32_t MQTT_RETRY_MAX = 5;

    bool ok = false;

    // 初始化重试
    for (uint32_t i = 0; i < INIT_RETRY_MAX; i++) {
        if (WIFI_App_Init()) {
            ok = true;
            break;
        }
        osDelay(2000);
    }

    // WiFi 连接重试
    if (ok) {
        if (GOT_IP) {
            /* RST 后已自动拿到 IP：跳过 CWJAP，加快启动 */
            ok = true;
        } else {
            ok = false;
            for (uint32_t i = 0; i < WIFI_RETRY_MAX; i++) {
                if (WIFI_App_ConnectWiFi()) {
                    ok = true;
                    break;
                }
                osDelay(5000);
            }
        }
    }

    // MQTT 连接重试
    if (ok) {
        ok = false;
        for (uint32_t i = 0; i < MQTT_RETRY_MAX; i++) {
            if (WIFI_App_ConnectMQTT()) {
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
