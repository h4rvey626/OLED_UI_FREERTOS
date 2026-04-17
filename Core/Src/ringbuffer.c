#include "ringbuffer.h"

/* =========================
 * 内部辅助宏
 * ========================= */
#define RB_NEXT_INDEX(rb, idx)   (((idx) + 1) % (rb)->size)

/* =========================
 * 函数实现
 * ========================= */

void ring_buffer_init(RingBuffer *rb, uint8_t *buf, size_t size)
{
    rb->buffer = buf;
    rb->size   = size;
    rb->head   = 0;
    rb->tail   = 0;
}

bool ring_buffer_write(RingBuffer *rb, uint8_t data)
{
    size_t next = RB_NEXT_INDEX(rb, rb->head);

    /* buffer 满：下一个 head 会追上 tail */
    if (next == rb->tail)
    {
        return false;
    }

    rb->buffer[rb->head] = data;
    rb->head = next;

    return true;
}

bool ring_buffer_read(RingBuffer *rb, uint8_t *data)
{
    /* buffer 空 */
    if (rb->head == rb->tail)
    {
        return false;
    }

    *data = rb->buffer[rb->tail];
    rb->tail = RB_NEXT_INDEX(rb, rb->tail);

    return true;
}

size_t ring_buffer_available(const RingBuffer *rb)
{
    if (rb->head >= rb->tail)
    {
        return rb->head - rb->tail;
    }
    else
    {
        return rb->size - rb->tail + rb->head;
    }
}

size_t ring_buffer_free(const RingBuffer *rb)
{
    /* 留一个字节区分满/空 */
    return (rb->size - 1) - ring_buffer_available(rb);
}

void ring_buffer_flush(RingBuffer *rb)
{
    rb->head = rb->tail = 0;
}
