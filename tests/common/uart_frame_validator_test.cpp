#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "protocol/crc16.h"
#include "protocol/target_command_codec.h"
#include "protocol/uart_frame.h"
#include "protocol/uart_frame_validator.h"

#define CHECK(condition)                                      \
    do                                                        \
    {                                                         \
        if (!(condition))                                     \
        {                                                     \
            std::cerr                                         \
                << "[FAIL] "                                  \
                << __func__                                   \
                << ":" << __LINE__                            \
                << " - " << #condition                        \
                << '\n';                                      \
            return false;                                     \
        }                                                     \
    } while (false)

namespace
{

constexpr std::array<uint8_t, 15U> VALID_TARGET_COMMAND_FRAME{
    0xAAU,
    0x55U,
    0x01U,
    0x01U,
    0x08U,

    0x01U, 0x00U,
    0xE8U, 0x03U,
    0x18U, 0xFCU,
    0x28U, 0x00U,

    0x9FU,
    0xE2U
};

/*
 * Recalculate and rewrite the CRC of a structurally complete frame.
 *
 * This helper is used by tests that intentionally modify fields
 * covered by CRC, while still requiring a CRC-valid frame.
 */
bool update_frame_crc(
    uint8_t* frame,
    size_t frame_size)
{
    if (frame == nullptr)
    {
        return false;
    }

    if (frame_size < UART_FRAME_MIN_SIZE)
    {
        return false;
    }

    const size_t payload_length =
        static_cast<size_t>(
            frame[UART_FRAME_PAYLOAD_LENGTH_OFFSET]
        );

    const size_t expected_frame_size =
        payload_length + UART_FRAME_OVERHEAD_SIZE;

    if (frame_size != expected_frame_size)
    {
        return false;
    }

    const size_t crc_input_size =
        3U + payload_length;

    uint16_t crc = 0U;

    if (!crc16_ccitt_false_calculate(
            &frame[UART_FRAME_VERSION_OFFSET],
            crc_input_size,
            &crc))
    {
        return false;
    }

    const size_t crc_offset =
        UART_FRAME_PAYLOAD_OFFSET + payload_length;

    frame[crc_offset] =
        static_cast<uint8_t>(crc & 0x00FFU);

    frame[crc_offset + 1U] =
        static_cast<uint8_t>((crc >> 8U) & 0x00FFU);

    return true;
}

bool test_valid_target_command_frame()
{
    CHECK(
        uart_frame_validate(
            VALID_TARGET_COMMAND_FRAME.data(),
            VALID_TARGET_COMMAND_FRAME.size()
        ) == UART_FRAME_VALID
    );

    CHECK(
        uart_frame_validate_target_command(
            VALID_TARGET_COMMAND_FRAME.data(),
            VALID_TARGET_COMMAND_FRAME.size()
        ) == UART_FRAME_VALID
    );

    return true;
}

bool test_null_frame_is_rejected()
{
    CHECK(
        uart_frame_validate(
            nullptr,
            0U
        ) == UART_FRAME_ERROR_NULL
    );

    CHECK(
        uart_frame_validate_target_command(
            nullptr,
            0U
        ) == UART_FRAME_ERROR_NULL
    );

    return true;
}

bool test_too_short_frame_is_rejected()
{
    CHECK(
        uart_frame_validate(
            VALID_TARGET_COMMAND_FRAME.data(),
            UART_FRAME_MIN_SIZE - 1U
        ) == UART_FRAME_ERROR_LENGTH
    );

    return true;
}

bool test_invalid_sof1_is_rejected()
{
    std::array<uint8_t, VALID_TARGET_COMMAND_FRAME.size()> frame =
        VALID_TARGET_COMMAND_FRAME;

    frame[UART_FRAME_SOF_1_OFFSET] = 0x00U;

    CHECK(
        uart_frame_validate(
            frame.data(),
            frame.size()
        ) == UART_FRAME_ERROR_SOF
    );

    return true;
}

bool test_invalid_sof2_is_rejected()
{
    std::array<uint8_t, VALID_TARGET_COMMAND_FRAME.size()> frame =
        VALID_TARGET_COMMAND_FRAME;

    frame[UART_FRAME_SOF_2_OFFSET] = 0x00U;

    CHECK(
        uart_frame_validate(
            frame.data(),
            frame.size()
        ) == UART_FRAME_ERROR_SOF
    );

    return true;
}

bool test_invalid_version_is_rejected()
{
    std::array<uint8_t, VALID_TARGET_COMMAND_FRAME.size()> frame =
        VALID_TARGET_COMMAND_FRAME;

    frame[UART_FRAME_VERSION_OFFSET] = 0x02U;

    CHECK(
        uart_frame_validate(
            frame.data(),
            frame.size()
        ) == UART_FRAME_ERROR_VERSION
    );

    return true;
}

bool test_invalid_frame_size_is_rejected()
{
    CHECK(
        uart_frame_validate(
            VALID_TARGET_COMMAND_FRAME.data(),
            VALID_TARGET_COMMAND_FRAME.size() - 1U
        ) == UART_FRAME_ERROR_LENGTH
    );

    return true;
}

bool test_corrupted_payload_is_rejected()
{
    std::array<uint8_t, VALID_TARGET_COMMAND_FRAME.size()> frame =
        VALID_TARGET_COMMAND_FRAME;

    /*
     * Corrupt one payload byte without updating CRC.
     */
    frame[UART_FRAME_PAYLOAD_OFFSET] ^= 0x01U;

    CHECK(
        uart_frame_validate(
            frame.data(),
            frame.size()
        ) == UART_FRAME_ERROR_CRC
    );

    return true;
}

bool test_corrupted_crc_is_rejected()
{
    std::array<uint8_t, VALID_TARGET_COMMAND_FRAME.size()> frame =
        VALID_TARGET_COMMAND_FRAME;

    frame[frame.size() - 1U] ^= 0x01U;

    CHECK(
        uart_frame_validate(
            frame.data(),
            frame.size()
        ) == UART_FRAME_ERROR_CRC
    );

    return true;
}

bool test_unsupported_message_type_is_rejected()
{
    std::array<uint8_t, VALID_TARGET_COMMAND_FRAME.size()> frame =
        VALID_TARGET_COMMAND_FRAME;

    /*
     * Change the message type and then generate a new valid CRC.
     *
     * The common frame validator should therefore accept this frame.
     * The TargetCommand validator must reject it because of its type.
     */
    frame[UART_FRAME_MESSAGE_TYPE_OFFSET] = 0x7FU;

    CHECK(
        update_frame_crc(
            frame.data(),
            frame.size()
        )
    );

    CHECK(
        uart_frame_validate(
            frame.data(),
            frame.size()
        ) == UART_FRAME_VALID
    );

    CHECK(
        uart_frame_validate_target_command(
            frame.data(),
            frame.size()
        ) == UART_FRAME_ERROR_MESSAGE_TYPE
    );

    return true;
}

bool test_invalid_target_command_payload_length_is_rejected()
{
    std::array<uint8_t, VALID_TARGET_COMMAND_FRAME.size()> frame =
        VALID_TARGET_COMMAND_FRAME;

    constexpr size_t invalid_payload_length =
        TARGET_COMMAND_WIRE_SIZE - 1U;

    const size_t invalid_frame_size =
        invalid_payload_length + UART_FRAME_OVERHEAD_SIZE;

    /*
     * Make a structurally valid UART frame whose TargetCommand
     * payload length is intentionally one byte too short.
     */
    frame[UART_FRAME_PAYLOAD_LENGTH_OFFSET] =
        static_cast<uint8_t>(invalid_payload_length);

    CHECK(
        update_frame_crc(
            frame.data(),
            invalid_frame_size
        )
    );

    CHECK(
        uart_frame_validate(
            frame.data(),
            invalid_frame_size
        ) == UART_FRAME_VALID
    );

    CHECK(
        uart_frame_validate_target_command(
            frame.data(),
            invalid_frame_size
        ) == UART_FRAME_ERROR_PAYLOAD_LENGTH
    );

    return true;
}

bool test_input_buffer_is_not_modified()
{
    std::array<uint8_t, VALID_TARGET_COMMAND_FRAME.size()> frame =
        VALID_TARGET_COMMAND_FRAME;

    const std::array<uint8_t, VALID_TARGET_COMMAND_FRAME.size()> before =
        frame;

    CHECK(
        uart_frame_validate(
            frame.data(),
            frame.size()
        ) == UART_FRAME_VALID
    );

    CHECK(frame == before);

    CHECK(
        uart_frame_validate_target_command(
            frame.data(),
            frame.size()
        ) == UART_FRAME_VALID
    );

    CHECK(frame == before);

    return true;
}

struct TestCase
{
    const char* name;
    bool (*function)();
};

} // namespace

int main()
{
    const TestCase tests[]{
        {
            "valid target command frame",
            test_valid_target_command_frame
        },
        {
            "null frame rejected",
            test_null_frame_is_rejected
        },
        {
            "too short frame rejected",
            test_too_short_frame_is_rejected
        },
        {
            "invalid SOF1 rejected",
            test_invalid_sof1_is_rejected
        },
        {
            "invalid SOF2 rejected",
            test_invalid_sof2_is_rejected
        },
        {
            "invalid version rejected",
            test_invalid_version_is_rejected
        },
        {
            "invalid frame size rejected",
            test_invalid_frame_size_is_rejected
        },
        {
            "corrupted payload rejected",
            test_corrupted_payload_is_rejected
        },
        {
            "corrupted CRC rejected",
            test_corrupted_crc_is_rejected
        },
        {
            "unsupported message type rejected",
            test_unsupported_message_type_is_rejected
        },
        {
            "invalid target command payload length rejected",
            test_invalid_target_command_payload_length_is_rejected
        },
        {
            "input buffer unchanged",
            test_input_buffer_is_not_modified
        }
    };

    for (const TestCase& test : tests)
    {
        if (!test.function())
        {
            std::cerr
                << "UART frame validator test failed: "
                << test.name
                << '\n';

            return 1;
        }

        std::cout
            << "[PASS] "
            << test.name
            << '\n';
    }

    std::cout
        << "All UART frame validator tests passed.\n";

    return 0;
}
