#ifndef LATC_PROTOCOL_UART_FRAME_H
#define LATC_PROTOCOL_UART_FRAME_H

#include <stdint.h>

/*
 * UART frame wire format
 *
 * Offset  Size  Field
 * 0       1     SOF 1
 * 1       1     SOF 2
 * 2       1     Version
 * 3       1     Message Type
 * 4       1     Payload Length
 * 5       N     Payload
 * 5 + N   2     CRC-16
 */

#define UART_FRAME_SOF_1                  0xAAU
#define UART_FRAME_SOF_2                  0x55U
#define UART_FRAME_VERSION                0x01U

#define UART_MESSAGE_TYPE_TARGET_COMMAND  0x01U

#define UART_FRAME_HEADER_SIZE            5U
#define UART_FRAME_CRC_SIZE               2U
#define UART_FRAME_OVERHEAD_SIZE          7U
#define UART_FRAME_MIN_SIZE               UART_FRAME_OVERHEAD_SIZE

#define UART_FRAME_MAX_PAYLOAD_SIZE       255U
#define UART_FRAME_MAX_SIZE               \
    (UART_FRAME_MAX_PAYLOAD_SIZE + UART_FRAME_OVERHEAD_SIZE)

/*
 * Logical view of a successfully decoded UART frame.
 *
 * payload points into the original encoded frame buffer.
 * The original frame buffer must remain valid while payload is used.
 */
typedef struct
{
    uint8_t version;
    uint8_t message_type;
    uint8_t payload_length;
    const uint8_t* payload;
} UartFrameView;

#endif /* LATC_PROTOCOL_UART_FRAME_H */
