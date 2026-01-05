#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* =========================
 * Ring Buffer 控制结构体
 * ========================= */
typedef struct
{
    uint8_t  *buffer;      // 数据缓冲区
    size_t    size;        // buffer 总大小
    volatile size_t head;  // 写指针（producer）
    volatile size_t tail;  // 读指针（consumer）
} RingBuffer;

/* =========================
 * API 接口
 * ========================= */

/**
 * @brief 初始化 RingBuffer
 * @param rb     RingBuffer 句柄
 * @param buf    实际存储 buffer
 * @param size   buffer 大小
 */
void ring_buffer_init(RingBuffer *rb, uint8_t *buf, size_t size);

/**
 * @brief 写入 1 字节
 * @return true  成功
 * @return false 缓冲区满
 */
bool ring_buffer_write(RingBuffer *rb, uint8_t data);

/**
 * @brief 读取 1 字节
 * @param data 读取出的数据
 * @return true  成功
 * @return false 缓冲区空
 */
bool ring_buffer_read(RingBuffer *rb, uint8_t *data);

/**
 * @brief 当前可读数据长度
 */
size_t ring_buffer_available(const RingBuffer *rb);

/**
 * @brief 剩余可写空间
 */
size_t ring_buffer_free(const RingBuffer *rb);

/**
 * @brief 清空 buffer
 */
void ring_buffer_flush(RingBuffer *rb);

#endif /* __RING_BUFFER_H */
