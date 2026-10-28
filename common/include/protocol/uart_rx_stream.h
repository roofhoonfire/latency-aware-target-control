#ifndef LATC_UART_RX_STREAM_H
#define LATC_UART_RX_STREAM_H

#include "protocol/target_frame_stream.h"

#ifndef UART_RX_QUEUE_CAPACITY
#define UART_RX_QUEUE_CAPACITY 128U
#endif

/* A byte and its continuity generation travel together through the RTOS queue.
 * Only the UART ISR changes epoch, on a dropped byte or a hardware RX error.
 * This prevents a parser from joining bytes across a known discontinuity,
 * including when pre-error bytes are still waiting in the queue. */
typedef struct {
    uint32_t epoch;
    uint8_t value;
} UartRxByte;

typedef struct {
    uint32_t ready;
    uint32_t irq_count;
    uint32_t bytes_read;
    uint32_t queue_drops;
    uint32_t error_events;
    uint32_t overrun_events;
    uint32_t fifo_resets;
    uint32_t last_error;
    uint32_t epoch;
} UartRxStats;

typedef struct {
    TargetFrameStream frame;
    uint32_t epoch;
    uint32_t gap_resets;
} UartRxStream;

/* Receive-task only; no peripheral/OS calls and no shared mutable parser state. */
static inline bool uart_rx_stream_push(
    UartRxStream *stream, const UartRxByte *item, TargetCommand *command)
{
    if (item->epoch != stream->epoch) {
        stream->frame.used = 0U;
        stream->epoch = item->epoch;
        ++stream->gap_resets;
    }
    return target_frame_stream_push(&stream->frame, item->value, command);
}

#ifdef __cplusplus
extern "C" {
#endif
extern volatile UartRxStats uart_rx_stats;
/* Platform implementations. Start once, in the receive task after scheduler
 * start. Wait blocks that task on an ISR-fed byte queue (no UART polling). */
bool uart_rx_start(void);
bool uart_rx_wait(UartRxByte *item);
#ifdef __cplusplus
}
#endif
#endif
