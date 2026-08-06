#ifndef LATC_PROTOCOL_UART_FRAME_CODEC_H
#define LATC_PROTOCOL_UART_FRAME_CODEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "protocol/uart_frame.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t uart_frame_encoded_size(size_t payload_size);

bool uart_frame_encode(
    uint8_t message_type,
    const uint8_t* payload,
    size_t payload_size,
    uint8_t* frame_buffer,
    size_t frame_buffer_size,
    size_t* encoded_size
);

bool uart_frame_decode(
    const uint8_t* frame_buffer,
    size_t frame_size,
    UartFrameView* decoded_frame
);

#ifdef __cplusplus
}
#endif

#endif /* LATC_PROTOCOL_UART_FRAME_CODEC_H */
