#ifndef LATC_PROTOCOL_UART_FRAME_VALIDATOR_H
#define LATC_PROTOCOL_UART_FRAME_VALIDATOR_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    UART_FRAME_VALID = 0,

    UART_FRAME_ERROR_NULL,
    UART_FRAME_ERROR_LENGTH,
    UART_FRAME_ERROR_SOF,
    UART_FRAME_ERROR_VERSION,
    UART_FRAME_ERROR_MESSAGE_TYPE,
    UART_FRAME_ERROR_PAYLOAD_LENGTH,
    UART_FRAME_ERROR_CRC

} UartFrameValidationResult;

/*
 * Validates the common UART frame structure and integrity.
 *
 * Checks:
 * - null input
 * - SOF
 * - protocol version
 * - encoded frame length
 * - CRC-16/CCITT-FALSE
 */
UartFrameValidationResult uart_frame_validate(
    const uint8_t* frame,
    size_t frame_size
);

/*
 * Validates that an already encoded UART frame is a valid
 * TargetCommand frame.
 *
 * In addition to common frame validation, checks:
 * - message type
 * - TargetCommand payload length
 */
UartFrameValidationResult uart_frame_validate_target_command(
    const uint8_t* frame,
    size_t frame_size
);

#ifdef __cplusplus
}
#endif

#endif /* LATC_PROTOCOL_UART_FRAME_VALIDATOR_H */
