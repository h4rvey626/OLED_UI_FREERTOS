/**
  ******************************************************************************
  * @file           : esp_driver.h
  * @brief          : ESP8266 low-level UART driver header
  ******************************************************************************
  */

#ifndef __ESP_DRIVER_H
#define __ESP_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "ringbuffer.h"
#include <stdbool.h>

/* Exported constants --------------------------------------------------------*/
#define ESP_RX_BUFFER_SIZE  512  // 接收缓冲区大小

/* Exported functions prototypes ---------------------------------------------*/

/* 驱动初始化与中断处理 */
void ESP_Driver_Init(void);
void ESP_Driver_ISR(uint8_t data);       // 在串口接收中断中调用
void ESP_Driver_FlushBuffer(void);

/* 底层 UART 读写接口 */
bool uart_read_byte(uint8_t* data);                   // 读取单字节
size_t uart_read(uint8_t* buf, size_t max_len);       // 批量读取
void uart_send(const uint8_t* data, size_t len);      // 发送数据

#ifdef __cplusplus
}
#endif

#endif /* __ESP_DRIVER_H */

