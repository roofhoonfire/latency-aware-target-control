#include "protocol/uart_frame_codec.h"
#include "protocol/crc16.h"
#include <string.h>

/*
 * UART frame wire-format offsets
 */
enum
{
    UART_FRAME_SOF_1_OFFSET          = 0U,
    UART_FRAME_SOF_2_OFFSET          = 1U,
    UART_FRAME_VERSION_OFFSET        = 2U,
    UART_FRAME_MESSAGE_TYPE_OFFSET   = 3U,
    UART_FRAME_PAYLOAD_LENGTH_OFFSET = 4U,
    UART_FRAME_PAYLOAD_OFFSET        = 5U
};

static void write_u16_le(uint8_t* buffer, uint16_t value)
{
    buffer[0] = (uint8_t)(value & 0x00FFU);
    buffer[1] = (uint8_t)((value >> 8U) & 0x00FFU);
}

static uint16_t read_u16_le(const uint8_t* buffer)
{
    const uint16_t low_byte = (uint16_t)buffer[0];
    const uint16_t high_byte =
        (uint16_t)((uint16_t)buffer[1] << 8U);

    return (uint16_t)(low_byte | high_byte);
}

size_t uart_frame_encoded_size(size_t payload_size)
{
    if (payload_size > UART_FRAME_MAX_PAYLOAD_SIZE)
    {
        return 0U;
    }

    return payload_size + UART_FRAME_OVERHEAD_SIZE;
}


bool uart_frame_encode(
    uint8_t message_type,
    const uint8_t* payload,
    size_t payload_size,
    uint8_t* frame_buffer,
    size_t frame_buffer_size,
    size_t* encoded_size)
{
    if ((frame_buffer == NULL) ||
        (encoded_size == NULL))
    {
        return false;
    }

    if ((payload == NULL) && (payload_size > 0U))
    {
        return false;
    }

    const size_t required_frame_size =
        uart_frame_encoded_size(payload_size);

    if ((required_frame_size == 0U) ||
        (frame_buffer_size < required_frame_size))
    {
        return false;
    }

    frame_buffer[UART_FRAME_SOF_1_OFFSET] =
        UART_FRAME_SOF_1;

    frame_buffer[UART_FRAME_SOF_2_OFFSET] =
        UART_FRAME_SOF_2;

    frame_buffer[UART_FRAME_VERSION_OFFSET] =
        UART_FRAME_VERSION;

    frame_buffer[UART_FRAME_MESSAGE_TYPE_OFFSET] =
        message_type;

    frame_buffer[UART_FRAME_PAYLOAD_LENGTH_OFFSET] =
        (uint8_t)payload_size;

    if (payload_size > 0U)
    {
        memcpy(
            &frame_buffer[UART_FRAME_PAYLOAD_OFFSET],
            payload,
            payload_size
        );
    }

    /*
     * CRC coverage:
     *
     * Version
     * Message Type
     * Payload Length
     * Payload
     *
     * SOF bytes are not included.
     */
    const size_t crc_input_size =
        3U + payload_size;

 uint16_t crc = 0U;

if (!crc16_ccitt_false_calculate(
        &frame_buffer[UART_FRAME_VERSION_OFFSET],
        crc_input_size,
        &crc))
{
    return false;
}

    const size_t crc_offset =
        UART_FRAME_PAYLOAD_OFFSET + payload_size;

    write_u16_le(
        &frame_buffer[crc_offset],
        crc
    );

    *encoded_size = required_frame_size;

    return true;
}

bool uart_frame_decode(
    const uint8_t* frame_buffer,
    size_t frame_size,
    UartFrameView* decoded_frame)
{
    if ((frame_buffer == NULL) ||
        (decoded_frame == NULL))
    {
        return false;
    }

    if (frame_size < UART_FRAME_MIN_SIZE)
    {
        return false;
    }

    if (frame_buffer[UART_FRAME_SOF_1_OFFSET] !=
        UART_FRAME_SOF_1)
    {
        return false;
    }

    if (frame_buffer[UART_FRAME_SOF_2_OFFSET] !=
        UART_FRAME_SOF_2)
    {
        return false;
    }

    if (frame_buffer[UART_FRAME_VERSION_OFFSET] !=
        UART_FRAME_VERSION)
    {
        return false;
    }

    const uint8_t payload_length =
        frame_buffer[UART_FRAME_PAYLOAD_LENGTH_OFFSET];

    const size_t expected_frame_size =
        uart_frame_encoded_size((size_t)payload_length);

    if (frame_size != expected_frame_size)
    {
        return false;
    }

    const size_t crc_offset =
        UART_FRAME_PAYLOAD_OFFSET +
        (size_t)payload_length;

    const uint16_t received_crc =
        read_u16_le(&frame_buffer[crc_offset]);

    const size_t crc_input_size =
        3U + (size_t)payload_length;

  uint16_t calculated_crc = 0U;

if (!crc16_ccitt_false_calculate(
        &frame_buffer[UART_FRAME_VERSION_OFFSET],
        crc_input_size,
        &calculated_crc))
{
    return false;
}

    if (received_crc != calculated_crc)
    {
        return false;
    }

    /*
     * Use a local object so that the caller-visible output is not
     * modified unless every validation check succeeds.
     */
    const UartFrameView validated_frame = {
        frame_buffer[UART_FRAME_VERSION_OFFSET],
        frame_buffer[UART_FRAME_MESSAGE_TYPE_OFFSET],
        payload_length,
        &frame_buffer[UART_FRAME_PAYLOAD_OFFSET]
    };

    *decoded_frame = validated_frame;

    return true;
}
