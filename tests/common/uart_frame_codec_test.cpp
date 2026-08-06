#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "protocol/uart_frame_codec.h"

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

constexpr std::array<uint8_t, 8U> TARGET_PAYLOAD{
    0x01U, 0x00U,
    0xE8U, 0x03U,
    0x18U, 0xFCU,
    0x28U, 0x00U
};

/*
 * CRC input:
 * 01 01 08 01 00 E8 03 18 FC 28 00
 *
 * CRC-16/CCITT-FALSE:
 * 0xE29F
 *
 * Wire order:
 * 9F E2
 */
constexpr std::array<uint8_t, 15U> EXPECTED_FRAME{
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

bool test_encoded_size()
{
    CHECK(uart_frame_encoded_size(0U) == 7U);
    CHECK(uart_frame_encoded_size(8U) == 15U);
    CHECK(uart_frame_encoded_size(255U) == 262U);
    CHECK(uart_frame_encoded_size(256U) == 0U);

    return true;
}

bool test_known_frame_encoding()
{
    std::array<uint8_t, UART_FRAME_MAX_SIZE> frame{};
    size_t encoded_size = 0U;

    CHECK(
        uart_frame_encode(
            UART_MESSAGE_TYPE_TARGET_COMMAND,
            TARGET_PAYLOAD.data(),
            TARGET_PAYLOAD.size(),
            frame.data(),
            frame.size(),
            &encoded_size
        )
    );

    CHECK(encoded_size == EXPECTED_FRAME.size());

    for (size_t index = 0U;
         index < EXPECTED_FRAME.size();
         ++index)
    {
        CHECK(frame[index] == EXPECTED_FRAME[index]);
    }

    return true;
}

bool test_valid_frame_decoding()
{
    UartFrameView decoded_frame{};

    CHECK(
        uart_frame_decode(
            EXPECTED_FRAME.data(),
            EXPECTED_FRAME.size(),
            &decoded_frame
        )
    );

    CHECK(decoded_frame.version == UART_FRAME_VERSION);

    CHECK(
        decoded_frame.message_type ==
        UART_MESSAGE_TYPE_TARGET_COMMAND
    );

    CHECK(
        decoded_frame.payload_length ==
        TARGET_PAYLOAD.size()
    );

    for (size_t index = 0U;
         index < TARGET_PAYLOAD.size();
         ++index)
    {
        CHECK(
            decoded_frame.payload[index] ==
            TARGET_PAYLOAD[index]
        );
    }

    return true;
}

bool test_corrupted_payload_is_rejected()
{
    std::array<uint8_t, EXPECTED_FRAME.size()> frame =
        EXPECTED_FRAME;

    /*
     * Corrupt the first payload byte without updating the CRC.
     */
    frame[5U] ^= 0x01U;

    UartFrameView decoded_frame{};

    CHECK(
        !uart_frame_decode(
            frame.data(),
            frame.size(),
            &decoded_frame
        )
    );

    return true;
}

bool test_invalid_sof_is_rejected()
{
    std::array<uint8_t, EXPECTED_FRAME.size()> frame =
        EXPECTED_FRAME;

    frame[0U] = 0x00U;

    UartFrameView decoded_frame{};

    CHECK(
        !uart_frame_decode(
            frame.data(),
            frame.size(),
            &decoded_frame
        )
    );

    return true;
}

bool test_invalid_version_is_rejected()
{
    std::array<uint8_t, EXPECTED_FRAME.size()> frame =
        EXPECTED_FRAME;

    frame[2U] = 0x02U;

    UartFrameView decoded_frame{};

    CHECK(
        !uart_frame_decode(
            frame.data(),
            frame.size(),
            &decoded_frame
        )
    );

    return true;
}

bool test_invalid_frame_size_is_rejected()
{
    UartFrameView decoded_frame{};

    CHECK(
        !uart_frame_decode(
            EXPECTED_FRAME.data(),
            EXPECTED_FRAME.size() - 1U,
            &decoded_frame
        )
    );

    return true;
}

bool test_decode_output_unchanged_on_failure()
{
    const uint8_t sentinel_payload = 0x42U;

    UartFrameView decoded_frame{
        0x11U,
        0x22U,
        0x33U,
        &sentinel_payload
    };

    std::array<uint8_t, EXPECTED_FRAME.size()> frame =
        EXPECTED_FRAME;

    frame[5U] ^= 0x01U;

    CHECK(
        !uart_frame_decode(
            frame.data(),
            frame.size(),
            &decoded_frame
        )
    );

    CHECK(decoded_frame.version == 0x11U);
    CHECK(decoded_frame.message_type == 0x22U);
    CHECK(decoded_frame.payload_length == 0x33U);
    CHECK(decoded_frame.payload == &sentinel_payload);

    return true;
}

bool test_encode_input_validation()
{
    std::array<uint8_t, 15U> frame{};
    size_t encoded_size = 123U;

    CHECK(
        !uart_frame_encode(
            UART_MESSAGE_TYPE_TARGET_COMMAND,
            nullptr,
            TARGET_PAYLOAD.size(),
            frame.data(),
            frame.size(),
            &encoded_size
        )
    );

    CHECK(encoded_size == 123U);

    CHECK(
        !uart_frame_encode(
            UART_MESSAGE_TYPE_TARGET_COMMAND,
            TARGET_PAYLOAD.data(),
            TARGET_PAYLOAD.size(),
            frame.data(),
            14U,
            &encoded_size
        )
    );

    CHECK(encoded_size == 123U);

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
        {"encoded size", test_encoded_size},
        {"known frame encoding", test_known_frame_encoding},
        {"valid frame decoding", test_valid_frame_decoding},
        {
            "corrupted payload rejected",
            test_corrupted_payload_is_rejected
        },
        {"invalid SOF rejected", test_invalid_sof_is_rejected},
        {
            "invalid version rejected",
            test_invalid_version_is_rejected
        },
        {
            "invalid frame size rejected",
            test_invalid_frame_size_is_rejected
        },
        {
            "decode output unchanged",
            test_decode_output_unchanged_on_failure
        },
        {
            "encode input validation",
            test_encode_input_validation
        }
    };

    for (const TestCase& test : tests)
    {
        if (!test.function())
        {
            std::cerr
                << "UART frame test failed: "
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
        << "All UART frame codec tests passed.\n";

    return 0;
}
