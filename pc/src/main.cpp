#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>

#include "protocol/target_command.h"
#include "protocol/target_command_codec.h"
#include "protocol/uart_frame.h"
#include "protocol/uart_frame_codec.h"

#include "transport/serial_port.h"

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
    std::cout
        << label
        << " ("
        << size
        << " bytes): ";

    for (std::size_t i = 0U; i < size; ++i)
    {
        std::cout
            << std::hex
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

int main(int argc, char* argv[])
{
    /*
     * Serial Device 경로를 명령행 인자로 받는다.
     *
     * 예:
     * ./target_control_pc /dev/ttyACM0
     */
    if (argc != 2)
    {
        std::cerr
            << "Usage: "
            << argv[0]
            << " <serial-device>\n";

        return 1;
    }

    const char* const serial_device_path = argv[1];

    /*
     * Step 1:
     * Application 계층의 원본 TargetCommand 생성.
     */
    const TargetCommand original_command{
        1U,
        static_cast<std::int16_t>(1000),
        static_cast<std::int16_t>(-1000),
        40U
    };

    /*
     * Step 2:
     * TargetCommand → 8-byte Payload
     */
    std::array<std::uint8_t, TARGET_COMMAND_WIRE_SIZE> payload{};

    if (!target_command_serialize(
            &original_command,
            payload.data(),
            payload.size()))
    {
        std::cerr
            << "Failed to serialize TargetCommand.\n";

        return 1;
    }

    print_bytes(
        "Payload",
        payload.data(),
        payload.size());

    /*
     * Step 3:
     * 필요한 UART Frame 크기 계산.
     */
    const std::size_t required_frame_size =
        uart_frame_encoded_size(payload.size());

    if (required_frame_size != kTargetCommandFrameSize)
    {
        std::cerr
            << "Unexpected UART frame size: "
            << required_frame_size
            << '\n';

        return 1;
    }

    std::array<
        std::uint8_t,
        kTargetCommandFrameSize> frame_buffer{};

    std::size_t encoded_size = 0U;

    /*
     * Step 4:
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
        std::cerr
            << "Failed to encode UART frame.\n";

        return 1;
    }

    print_bytes(
        "UART Frame",
        frame_buffer.data(),
        encoded_size);

    std::cout
        << "Total Size       : "
        << encoded_size
        << " bytes\n";

    /*
     * 기존 PC Mock 검증도 유지한다.
     *
     * Frame을 한 번 Decode하여 Protocol 계층이
     * 정상 동작하는지 확인한 뒤 실제 송신한다.
     */
    UartFrameView decoded_frame{};

    if (!uart_frame_decode(
            frame_buffer.data(),
            encoded_size,
            &decoded_frame))
    {
        std::cerr
            << "Failed to decode UART frame.\n";

        return 1;
    }

    if (decoded_frame.message_type !=
        UART_MESSAGE_TYPE_TARGET_COMMAND)
    {
        std::cerr
            << "Unexpected message type.\n";

        return 1;
    }

    if (decoded_frame.payload_length !=
        TARGET_COMMAND_WIRE_SIZE)
    {
        std::cerr
            << "Unexpected payload length.\n";

        return 1;
    }

    TargetCommand decoded_command{};

    if (!target_command_deserialize(
            decoded_frame.payload,
            decoded_frame.payload_length,
            &decoded_command))
    {
        std::cerr
            << "Failed to deserialize TargetCommand.\n";

        return 1;
    }

    if (!commands_equal(
            original_command,
            decoded_command))
    {
        std::cerr
            << "Decoded TargetCommand does not match original.\n";

        return 1;
    }

    std::cout
        << "Protocol Round Trip: PASS\n";

    /*
     * Step 5:
     * Transport 계층 시작.
     */
    latc::transport::SerialPort serial_port;

    std::cout
        << "Serial Device    : "
        << serial_device_path
        << '\n';

    /*
     * Linux Serial Device Open + 115200 8N1 Raw 설정.
     */
    if (!serial_port.open(serial_device_path))
    {
        std::cerr
            << "Failed to open serial device: "
            << std::strerror(errno)
            << '\n';

        return 1;
    }

    std::cout
        << "Baud Rate        : "
        << latc::transport::SerialPort::kBaudRate
        << '\n';

    /*
     * Step 6:
     * UART Frame 전체 송신.
     */
    std::size_t bytes_written = 0U;

    if (!serial_port.write_all(
            frame_buffer.data(),
            encoded_size,
            &bytes_written))
    {
        std::cerr
            << "Serial transmission failed after "
            << bytes_written
            << " bytes: "
            << std::strerror(errno)
            << '\n';

        return 1;
    }

    std::cout
        << "Bytes Transmitted: "
        << bytes_written
        << '\n';

    /*
     * Step 7:
     * Serial Device 명시적으로 닫기.
     *
     * 생략해도 소멸자가 닫지만,
     * 여기서는 성공/실패를 확인하기 위해 직접 호출한다.
     */
    if (!serial_port.close())
    {
        std::cerr
            << "Failed to close serial device: "
            << std::strerror(errno)
            << '\n';

        return 1;
    }

    std::cout
        << "Transmission     : PASS\n";

    return 0;
}
