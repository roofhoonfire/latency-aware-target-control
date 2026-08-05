#ifndef LATC_PROTOCOL_UART_FRAME_CODEC_H
#define LATC_PROTOCOL_UART_FRAME_CODEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "protocol/uart_frame.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Returns the required encoded frame size for the given payload size.
 *
 * Returns 0 when payload_size exceeds UART_FRAME_MAX_PAYLOAD_SIZE.
 */
size_t uart_frame_encoded_size(size_t payload_size);

/*
 * Calculates CRC-16/CCITT-FALSE.
 *
 * Polynomial : 0x1021
 * Initial    : 0xFFFF
 * RefIn      : false
 * RefOut     : false
 * XorOut     : 0x0000
 */
uint16_t uart_frame_crc16_ccitt_false(
    const uint8_t* data,
    size_t data_size
);

/*
 * Encodes one complete UART frame.
 *
 * encoded_size is written only when encoding succeeds.
 */
bool uart_frame_encode(
    uint8_t message_type,
    const uint8_t* payload,
    size_t payload_size,
    uint8_t* frame_buffer,
    size_t frame_buffer_size,
    size_t* encoded_size
);

/*
 * Validates and decodes one complete UART frame.
 *
 * decoded_frame is modified only when all validation checks succeed.
 */
bool uart_frame_decode(
    const uint8_t* frame_buffer,
    size_t frame_size,
    UartFrameView* decoded_frame
);

#ifdef __cplusplus
}
#endif

#endif /* LATC_PROTOCOL_UART_FRAME_CODEC_H */
