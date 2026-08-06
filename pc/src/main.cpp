#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>

#include "protocol/target_command.h"
#include "protocol/target_command_codec.h"
#include "protocol/uart_frame.h"
#include "protocol/uart_frame_codec.h"

namespace
{

constexpr std::size_t kUartFrameOverheadSize = 7U;

constexpr std::size_t kTargetCommandFrameSize =
    TARGET_COMMAND_WIRE_SIZE + kUartFrameOverheadSize;

void print_bytes(
    const char* label,
    const std::uint8_t* data,
    std::size_t size)
{
    std::cout << label << " (" << size << " bytes): ";

    for (std::size_t i = 0U; i < size; ++i)
    {
        std::cout << std::hex
                  << std::uppercase
                  << std::setw(2)
                  << std::setfill('0')
                  << static_cast<unsigned int>(data[i]);

        if (i + 1U < size)
        {
            std::cout << ' ';
        }
    }

    std::cout << std::dec << '\n';
}

bool commands_equal(
    const TargetCommand& lhs,
    const TargetCommand& rhs)
{
    return lhs.sequence == rhs.sequence &&
           lhs.target_x == rhs.target_x &&
           lhs.target_y == rhs.target_y &&
           lhs.prediction_ms == rhs.prediction_ms;
}

}  // namespace

int main()
{
    /*
     * PC 애플리케이션이 생성했다고 가정하는 원본 명령.
     */
    const TargetCommand original_command{
        1U,
        static_cast<std::int16_t>(1000),
        static_cast<std::int16_t>(-1000),
        40U
    };

    /*
     * Step 1:
     * TargetCommand → 고정 8바이트 Payload
     */
    std::array<std::uint8_t, TARGET_COMMAND_WIRE_SIZE> payload{};

    if (!target_command_serialize(
            &original_command,
            payload.data(),
            payload.size()))
    {
        std::cerr << "Failed to serialize TargetCommand.\n";
        return 1;
    }

    print_bytes(
        "Payload",
        payload.data(),
        payload.size());

    /*
     * Step 2:
     * Payload 길이를 기준으로 필요한 UART Frame 크기 계산.
     *
     * TargetCommand Payload 8바이트 + Frame Overhead 7바이트
     * = 총 15바이트여야 한다.
     */
    const std::size_t required_frame_size =
        uart_frame_encoded_size(payload.size());

    if (required_frame_size != kTargetCommandFrameSize)
    {
        std::cerr << "Unexpected UART frame size: "
                  << required_frame_size
                  << '\n';

        return 1;
    }

    /*
     * 동적 메모리를 사용하지 않고 고정 크기 배열 사용.
     */
    std::array<std::uint8_t, kTargetCommandFrameSize> frame_buffer{};

    std::size_t encoded_size = 0U;

    /*
     * Step 3:
     * Payload → UART Frame
     */
    if (!uart_frame_encode(
            UART_MESSAGE_TYPE_TARGET_COMMAND,
            payload.data(),
            payload.size(),
            frame_buffer.data(),
            frame_buffer.size(),
            &encoded_size))
    {
        std::cerr << "Failed to encode UART frame.\n";
        return 1;
    }

    print_bytes(
        "UART Frame",
        frame_buffer.data(),
        encoded_size);

    std::cout << "Total Size: "
              << encoded_size
              << " bytes\n";

    /*
     * Step 4:
     * UART Frame → UartFrameView
     */
    UartFrameView decoded_frame{};

    if (!uart_frame_decode(
            frame_buffer.data(),
            encoded_size,
            &decoded_frame))
    {
        std::cerr << "Failed to decode UART frame.\n";
        return 1;
    }

    /*
     * Step 5:
     * Message Type 검사.
     *
     * Frame Decode 성공이 곧 TargetCommand라는 뜻은 아니다.
     * 다른 종류의 메시지도 동일한 UART Frame 형식을 사용할 수 있다.
     */
    if (decoded_frame.message_type !=
        UART_MESSAGE_TYPE_TARGET_COMMAND)
    {
        std::cerr << "Unexpected message type: "
                  << static_cast<unsigned int>(
                         decoded_frame.message_type)
                  << '\n';

        return 1;
    }

    /*
     * Step 6:
     * TargetCommand가 요구하는 Payload 길이 검사.
     */
    if (decoded_frame.payload_length !=
        TARGET_COMMAND_WIRE_SIZE)
    {
        std::cerr << "Unexpected payload length: "
                  << static_cast<unsigned int>(
                         decoded_frame.payload_length)
                  << '\n';

        return 1;
    }

    /*
     * Step 7:
     * Decode된 Payload → TargetCommand
     */
    TargetCommand decoded_command{};

    if (!target_command_deserialize(
            decoded_frame.payload,
            decoded_frame.payload_length,
            &decoded_command))
    {
        std::cerr << "Failed to deserialize TargetCommand.\n";
        return 1;
    }

    /*
     * Step 8:
     * 원본과 복원 결과 비교.
     */
    if (!commands_equal(
            original_command,
            decoded_command))
    {
        std::cerr
            << "Decoded TargetCommand does not match original.\n";

        return 1;
    }

    /*
     * 정상 결과 출력.
     */
    std::cout << "Version         : 0x"
              << std::hex
              << std::uppercase
              << std::setw(2)
              << std::setfill('0')
              << static_cast<unsigned int>(
                     decoded_frame.version)
              << '\n';

    std::cout << "Message Type    : 0x"
              << std::hex
              << std::uppercase
              << std::setw(2)
              << std::setfill('0')
              << static_cast<unsigned int>(
                     decoded_frame.message_type)
              << '\n';

    std::cout << std::dec;

    std::cout << "Payload Length  : "
              << static_cast<unsigned int>(
                     decoded_frame.payload_length)
              << " bytes\n";

    std::cout << "Decoded Command : sequence="
              << decoded_command.sequence
              << ", target_x="
              << decoded_command.target_x
              << ", target_y="
              << decoded_command.target_y
              << ", prediction_ms="
              << decoded_command.prediction_ms
              << '\n';

    std::cout << "Round Trip      : PASS\n";

    return 0;
}
