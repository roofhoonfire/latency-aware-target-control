#include <array>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>

#include "input/detection_csv_reader.h"

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

/*
 * Simulink에서는 Target 위치를 meter 단위로 사용한다.
 *
 * TargetCommand의 int16_t 좌표에는 centimeter 단위로 변환하여 저장한다.
 *
 * 예:
 * 5.1475 m
 *   ↓ × 100
 * 514.75 cm
 *   ↓ round
 * 515
 */
constexpr double kMetersToCommandUnits = 100.0;

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

/**
 * @brief Simulink DetectionRecord를 baseline TargetCommand로 변환한다.
 *
 * 현재 Take에서는 latency-aware prediction을 적용하지 않는다.
 *
 * Detection 위치:
 *   meter
 *
 * TargetCommand 위치:
 *   centimeter
 *
 * prediction_ms:
 *   baseline이므로 0
 */
bool detection_to_target_command(
    const latc::input::DetectionRecord& detection,
    std::uint16_t sequence,
    TargetCommand* command)
{
    if (command == nullptr)
    {
        return false;
    }

    const long target_x =
        std::lround(
            detection.target_x_m *
            kMetersToCommandUnits);

    const long target_y =
        std::lround(
            detection.target_y_m *
            kMetersToCommandUnits);

    /*
     * TargetCommand의 target_x / target_y는 int16_t이므로
     * 변환 결과가 표현 가능한 범위인지 확인한다.
     */
    if (target_x <
            std::numeric_limits<std::int16_t>::min() ||
        target_x >
            std::numeric_limits<std::int16_t>::max() ||
        target_y <
            std::numeric_limits<std::int16_t>::min() ||
        target_y >
            std::numeric_limits<std::int16_t>::max())
    {
        return false;
    }

    command->sequence = sequence;

    command->target_x =
        static_cast<std::int16_t>(target_x);

    command->target_y =
        static_cast<std::int16_t>(target_y);

    /*
     * Baseline mode.
     *
     * Latency-aware future prediction은
     * 이후 Take에서 추가한다.
     */
    command->prediction_ms = 0U;

    return true;
}

}  // namespace

int main(int argc, char* argv[])
{
    /*
     * Serial Device 경로와 Simulink Detection CSV 경로를
     * 명령행 인자로 받는다.
     *
     * 예:
     *
     * ./target_control_pc \
     *     /dev/ttyACM0 \
     *     ../simulation/simulink/target_detections.csv
     */
    if (argc != 3)
    {
        std::cerr
            << "Usage: "
            << argv[0]
            << " <serial-device> <detection-csv>\n";

        return 1;
    }

    const char* const serial_device_path = argv[1];
    const char* const detection_csv_path = argv[2];

    /*
     * Step 1:
     * Simulink Detection CSV에서 DetectionRecord를 읽는다.
     *
     * 현재 Take에서는 첫 번째 유효 Detection Record를 사용한다.
     */
    latc::input::DetectionRecord detection{};

    if (!latc::input::read_first_detection_csv(
            detection_csv_path,
            &detection))
    {
        std::cerr
            << "Failed to read detection CSV.\n";

        return 1;
    }

    std::cout
        << std::fixed
        << std::setprecision(3)
        << "Detection Time   : "
        << detection.time_sec
        << " s\n"
        << "Target Position  : ("
        << detection.target_x_m
        << ", "
        << detection.target_y_m
        << ") m\n"
        << "Target Velocity  : ("
        << detection.target_vx_mps
        << ", "
        << detection.target_vy_mps
        << ") m/s\n";

    /*
     * Step 2:
     * DetectionRecord를 baseline TargetCommand로 변환한다.
     *
     * 현재:
     *
     * target_x / target_y
     *     meter → centimeter
     *
     * prediction_ms
     *     0 (baseline)
     */
    TargetCommand original_command{};

    if (!detection_to_target_command(
            detection,
            1U,
            &original_command))
    {
        std::cerr
            << "Detection cannot be represented as TargetCommand.\n";

        return 1;
    }

    std::cout
        << "TargetCommand    : seq="
        << original_command.sequence
        << ", x="
        << original_command.target_x
        << ", y="
        << original_command.target_y
        << ", prediction_ms="
        << original_command.prediction_ms
        << '\n';

    /*
     * Step 3:
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
     * Step 4:
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
     * Step 5:
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
     * 생성한 Frame을 다시 Decode하여
     *
     * TargetCommand
     * → Serialize
     * → UART Frame Encode
     * → UART Frame Decode
     * → Deserialize
     *
     * round-trip 결과가 원본과 동일한지 확인한다.
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
     * Step 6:
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
     * Step 7:
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
     * Step 8:
     * Serial Device 명시적으로 닫기.
     *
     * 생략해도 SerialPort 소멸자가 닫지만,
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
