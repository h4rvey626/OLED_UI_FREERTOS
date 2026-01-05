/**
  ******************************************************************************
  * @file           : esp_at.h
  * @brief          : ESP8266 AT command layer header
  ******************************************************************************
  */

#ifndef __ESP_AT_H
#define __ESP_AT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "esp_driver.h"
#include <stdbool.h>
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
#define ESP_AT_TIMEOUT_SHORT   1000    // 短超时 (ms)
#define ESP_AT_TIMEOUT_LONG    10000   // 长超时 (ms)

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Send AT command and wait for OK
  * @param  cmd: AT command string (without \r\n)
  * @retval true if OK received, false otherwise
  */
bool ESP_AT_Send(const char* cmd);

/**
  * @brief  Send AT command with custom timeout
  * @param  cmd: AT command string (without \r\n)
  * @param  timeout_ms: Timeout in milliseconds
  * @retval true if OK received, false otherwise
  */
bool ESP_AT_SendTimeout(const char* cmd, uint32_t timeout_ms);

/**
  * @brief  Wait for specific response string
  * @param  expect: String to wait for (e.g., "OK", "ready", ">")
  * @param  timeout_ms: Timeout in milliseconds
  * @retval true if found, false if timeout or error
  */
bool ESP_AT_WaitFor(const char* expect, uint32_t timeout_ms);

/**
  * @brief  Get last received response snippet (for debugging)
  * @retval Pointer to internal buffer containing last response
  */
const char* ESP_AT_GetLastRxSnippet(void);

#ifdef __cplusplus
}
#endif

#endif /* __ESP_AT_H */
