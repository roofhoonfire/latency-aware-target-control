#include "protocol/uart_frame_validator.h"

#include "protocol/crc16.h"
#include "protocol/target_command_codec.h"
#include "protocol/uart_frame.h"

static uint16_t read_u16_le(const uint8_t* buffer)
{
    const uint16_t low_byte =
        (uint16_t)buffer[0];

    const uint16_t high_byte =
        (uint16_t)((uint16_t)buffer[1] << 8U);

    return (uint16_t)(low_byte | high_byte);
}

UartFrameValidationResult uart_frame_validate(
    const uint8_t* frame,
    size_t frame_size)
{
    if (frame == NULL)
    {
        return UART_FRAME_ERROR_NULL;
    }

    /*
     * At least the fixed UART frame overhead must exist before
     * accessing the frame header and CRC.
     */
    if (frame_size < UART_FRAME_MIN_SIZE)
    {
        return UART_FRAME_ERROR_LENGTH;
    }

    if ((frame[UART_FRAME_SOF_1_OFFSET] != UART_FRAME_SOF_1) ||
        (frame[UART_FRAME_SOF_2_OFFSET] != UART_FRAME_SOF_2))
    {
        return UART_FRAME_ERROR_SOF;
    }

    if (frame[UART_FRAME_VERSION_OFFSET] != UART_FRAME_VERSION)
    {
        return UART_FRAME_ERROR_VERSION;
    }

    const uint8_t payload_length =
        frame[UART_FRAME_PAYLOAD_LENGTH_OFFSET];

    const size_t expected_frame_size =
        (size_t)payload_length + UART_FRAME_OVERHEAD_SIZE;

    if (frame_size != expected_frame_size)
    {
        return UART_FRAME_ERROR_LENGTH;
    }

    const size_t crc_offset =
        UART_FRAME_PAYLOAD_OFFSET + (size_t)payload_length;

    const uint16_t received_crc =
        read_u16_le(&frame[crc_offset]);

    /*
     * CRC coverage:
     *
     * Version
     * Message Type
     * Payload Length
     * Payload
     */
    const size_t crc_input_size =
        3U + (size_t)payload_length;

    uint16_t calculated_crc = 0U;

    if (!crc16_ccitt_false_calculate(
            &frame[UART_FRAME_VERSION_OFFSET],
            crc_input_size,
            &calculated_crc))
    {
        return UART_FRAME_ERROR_CRC;
    }

    if (received_crc != calculated_crc)
    {
        return UART_FRAME_ERROR_CRC;
    }

    return UART_FRAME_VALID;
}

UartFrameValidationResult uart_frame_validate_target_command(
    const uint8_t* frame,
    size_t frame_size)
{
    const UartFrameValidationResult frame_result =
        uart_frame_validate(frame, frame_size);

    if (frame_result != UART_FRAME_VALID)
    {
        return frame_result;
    }

    if (frame[UART_FRAME_MESSAGE_TYPE_OFFSET] !=
        UART_MESSAGE_TYPE_TARGET_COMMAND)
    {
        return UART_FRAME_ERROR_MESSAGE_TYPE;
    }

    if ((size_t)frame[UART_FRAME_PAYLOAD_LENGTH_OFFSET] !=
        TARGET_COMMAND_WIRE_SIZE)
    {
        return UART_FRAME_ERROR_PAYLOAD_LENGTH;
    }

    return UART_FRAME_VALID;
}
