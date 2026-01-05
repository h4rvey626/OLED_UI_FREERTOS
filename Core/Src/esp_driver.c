/**
  ******************************************************************************
  * @file           : esp_driver.c
  * @brief          : ESP8266 low-level UART driver
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "esp_driver.h"
#include "usart.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static uint8_t rb_storage[ESP_RX_BUFFER_SIZE];
static RingBuffer esp_rb;
static bool driver_initialized = false;

/* Function implementations --------------------------------------------------*/

/**
  * @brief  Initialize the driver and RingBuffer
  */
void ESP_Driver_Init(void) {
    if (!driver_initialized) {
        ring_buffer_init(&esp_rb, rb_storage, sizeof(rb_storage));
        driver_initialized = true;
        
        // 启动串口中断接收 (需要在 usart.c 或 main.c 中开启 HAL_UART_Receive_IT)
        // 这里假设外部已经配置好了 UART 硬件
    }
}

/**
  * @brief  Handle byte received from ISR
  * @param  data: Received byte
  */
void ESP_Driver_ISR(uint8_t data) {
    if (driver_initialized) {
        ring_buffer_write(&esp_rb, data);
    }
}

/**
  * @brief  Flush receive buffer
  */
void ESP_Driver_FlushBuffer(void) {
    ring_buffer_flush(&esp_rb);
}

/**
  * @brief  Read single byte from buffer
  * @param  data: Pointer to store received byte
  * @retval true if data available, false if buffer empty
  */
bool uart_read_byte(uint8_t* data) {
    return ring_buffer_read(&esp_rb, data);
}

/**
  * @brief  Read available data from buffer
  * @param  buf: Buffer to store data
  * @param  max_len: Maximum bytes to read
  * @retval Number of bytes actually read
  */
size_t uart_read(uint8_t* buf, size_t max_len) {
    size_t count = 0;
    while (count < max_len) {
        if (!ring_buffer_read(&esp_rb, &buf[count])) {
            break;
        }
        count++;
    }
    return count;
}

/**
  * @brief  Send data through UART
  * @param  data: Data buffer to send
  * @param  len: Length of data
  */
void uart_send(const uint8_t* data, size_t len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)data, len, 100);
}

