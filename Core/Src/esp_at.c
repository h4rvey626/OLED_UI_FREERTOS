/**
  ******************************************************************************
  * @file           : esp_at.c
  * @brief          : ESP8266 AT command layer implementation
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "esp_at.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>

/* Private variables ---------------------------------------------------------*/
static char last_rx_snippet[128] = {0};

/* Private function prototypes -----------------------------------------------*/
static void ESP_AT_SetLastRxSnippet(const char* s);

/* Function implementations --------------------------------------------------*/

/**
  * @brief  Get last received response snippet
  * @retval Pointer to internal buffer
  */
const char* ESP_AT_GetLastRxSnippet(void) {
    return last_rx_snippet;
}

/**
  * @brief  Set last received snippet (internal use)
  * @param  s: String to store
  */
static void ESP_AT_SetLastRxSnippet(const char* s) {
    if (!s) {
        last_rx_snippet[0] = '\0';
        return;
    }
    strncpy(last_rx_snippet, s, sizeof(last_rx_snippet) - 1);
    last_rx_snippet[sizeof(last_rx_snippet) - 1] = '\0';
}

/**
  * @brief  Send AT command WITHOUT waiting
  * @param  cmd: AT command string
  * @retval None
  */
void ESP_AT_Send(const char* cmd) {
    if (!cmd) return;
    // 不清缓冲：由上层决定是否 Flush（有时需要保留前序输出用于匹配）
    uart_send((const uint8_t*)cmd, strlen(cmd));
    uart_send((const uint8_t*)"\r\n", 2);
}

/**
  * @brief  Send AT command and wait for specific string
  */
bool ESP_AT_SendWaitFor(const char* cmd, const char* expect, uint32_t timeout_ms) {
    if (!cmd || !expect) return false;
    ESP_Driver_FlushBuffer();
    uart_send((const uint8_t*)cmd, strlen(cmd));
    uart_send((const uint8_t*)"\r\n", 2);
    return ESP_AT_WaitFor(expect, timeout_ms);
}

/**
  * @brief  Wait for specific string in receive buffer
  * @param  expect: String to expect (e.g., "OK", "ready")
  * @param  timeout_ms: Max wait time
  * @retval true if found, false if timeout
  */
bool ESP_AT_WaitFor(const char* expect, uint32_t timeout_ms) {
    uint32_t start = HAL_GetTick();
    char line_buf[256];
    uint16_t idx = 0;
    uint8_t data;
    line_buf[0] = '\0';

    while ((HAL_GetTick() - start) < timeout_ms) {
        if (uart_read_byte(&data)) {
            // 存入临时流缓冲区（滚动窗口，避免溢出时丢掉关键字符导致匹配不到 "OK"）
            if (idx >= (sizeof(line_buf) - 1)) {
                // 保留后半段内容，继续拼接
                const size_t keep = sizeof(line_buf) / 2;
                memmove(line_buf, line_buf + (idx - keep), keep);
                idx = (uint16_t)keep;
                line_buf[idx] = '\0';
            }
            line_buf[idx++] = (char)data;
            line_buf[idx] = '\0';

            // 检查是否包含目标字符串
            if (strstr(line_buf, expect) != NULL) {
                ESP_AT_SetLastRxSnippet(line_buf);
                return true;
            }

            // 检查错误条件
            if (strstr(line_buf, "ERROR") != NULL || strstr(line_buf, "FAIL") != NULL) {
                ESP_AT_SetLastRxSnippet(line_buf);
                return false;
            }
        } else {
            // 给 OS 一点时间
            osDelay(5);
        }
    }
    ESP_AT_SetLastRxSnippet((idx > 0) ? line_buf : "");
    return false;
}
