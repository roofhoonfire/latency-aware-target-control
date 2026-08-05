#include "protocol/target_command_codec.h"

#include <limits.h>
#include <stdint.h>

/*
 * TargetCommand wire format offsets
 *
 * Offset  Size  Field
 * 0       2     sequence
 * 2       2     target_x
 * 4       2     target_y
 * 6       2     prediction_ms
 */
enum
{
    TARGET_COMMAND_SEQUENCE_OFFSET      = 0U,
    TARGET_COMMAND_TARGET_X_OFFSET      = 2U,
    TARGET_COMMAND_TARGET_Y_OFFSET      = 4U,
    TARGET_COMMAND_PREDICTION_MS_OFFSET = 6U
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

static void write_i16_le(uint8_t* buffer, int16_t value)
{
    /*
     * signed -> unsigned 변환은 2^16 기준으로 변환된다.
     *
     * 예:
     * -1   -> 0xFFFF
     * -100 -> 0xFF9C
     */
    write_u16_le(buffer, (uint16_t)value);
}

static int16_t read_i16_le(const uint8_t* buffer)
{
    const uint16_t raw_value = read_u16_le(buffer);

    if (raw_value <= (uint16_t)INT16_MAX)
    {
        return (int16_t)raw_value;
    }

    /*
     * 0x8000 ~ 0xFFFF를 -32768 ~ -1로 복원한다.
     *
     * uint16_t 값을 int16_t로 바로 캐스팅할 경우,
     * 표현할 수 없는 값에 대한 결과가 구현 의존적일 수 있으므로
     * 명시적으로 음수 범위로 변환한다.
     */
    return (int16_t)((int32_t)raw_value - 65536);
}

bool target_command_serialize(
    const TargetCommand* command,
    uint8_t* buffer,
    size_t buffer_size)
{
    if ((command == NULL) ||
        (buffer == NULL) ||
        (buffer_size < TARGET_COMMAND_WIRE_SIZE))
    {
        return false;
    }

    write_u16_le(
        &buffer[TARGET_COMMAND_SEQUENCE_OFFSET],
        command->sequence);

    write_i16_le(
        &buffer[TARGET_COMMAND_TARGET_X_OFFSET],
        command->target_x);

    write_i16_le(
        &buffer[TARGET_COMMAND_TARGET_Y_OFFSET],
        command->target_y);

    write_u16_le(
        &buffer[TARGET_COMMAND_PREDICTION_MS_OFFSET],
        command->prediction_ms);

    return true;
}

bool target_command_deserialize(
    const uint8_t* buffer,
    size_t buffer_size,
    TargetCommand* command)
{
    if ((buffer == NULL) ||
        (command == NULL) ||
        (buffer_size < TARGET_COMMAND_WIRE_SIZE))
    {
        return false;
    }

    command->sequence =
        read_u16_le(&buffer[TARGET_COMMAND_SEQUENCE_OFFSET]);

    command->target_x =
        read_i16_le(&buffer[TARGET_COMMAND_TARGET_X_OFFSET]);

    command->target_y =
        read_i16_le(&buffer[TARGET_COMMAND_TARGET_Y_OFFSET]);

    command->prediction_ms =
        read_u16_le(&buffer[TARGET_COMMAND_PREDICTION_MS_OFFSET]);

    return true;
}
