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
#include "input/udp_detection_receiver.h"

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
 * Simulink Detection 위치:
 * meter
 *
 * TargetCommand 위치:
 * centimeter
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

void print_detection(
    const latc::input::DetectionRecord& detection)
{
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
}

/*
 * DetectionRecord
 *     ↓
 * TargetCommand
 *
 * 현재는 latency compensation을 적용하지 않는 baseline이다.
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
     * Baseline.
     *
     * Latency-aware prediction은 이후 Take에서 추가한다.
     */
    command->prediction_ms = 0U;

    return true;
}

bool encode_target_command_frame(
    const TargetCommand& command,
    std::array<
        std::uint8_t,
        kTargetCommandFrameSize>* frame_buffer,
    std::size_t* encoded_size)
{
    if (frame_buffer == nullptr ||
        encoded_size == nullptr)
    {
        return false;
    }

    std::array<
        std::uint8_t,
        TARGET_COMMAND_WIRE_SIZE> payload{};

    if (!target_command_serialize(
            &command,
            payload.data(),
            payload.size()))
    {
        return false;
    }

    const std::size_t required_frame_size =
        uart_frame_encoded_size(payload.size());

    if (required_frame_size !=
        kTargetCommandFrameSize)
    {
        return false;
    }

    return uart_frame_encode(
        UART_MESSAGE_TYPE_TARGET_COMMAND,
        payload.data(),
        payload.size(),
        frame_buffer->data(),
        frame_buffer->size(),
        encoded_size);
}

bool transmit_detection(
    latc::transport::SerialPort* serial_port,
    const latc::input::DetectionRecord& detection,
    std::uint16_t sequence,
    bool verbose)
{
    if (serial_port == nullptr)
    {
        return false;
    }

    TargetCommand command{};

    if (!detection_to_target_command(
            detection,
            sequence,
            &command))
    {
        std::cerr
            << "Detection cannot be represented as "
            << "TargetCommand.\n";

        return false;
    }

    std::array<
        std::uint8_t,
        kTargetCommandFrameSize> frame_buffer{};

    std::size_t encoded_size = 0U;

    if (!encode_target_command_frame(
            command,
            &frame_buffer,
            &encoded_size))
    {
        std::cerr
            << "Failed to encode TargetCommand frame.\n";

        return false;
    }

    std::size_t bytes_written = 0U;

    if (!serial_port->write_all(
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

        return false;
    }

    if (verbose)
    {
        print_detection(detection);

        std::cout
            << "TargetCommand    : seq="
            << command.sequence
            << ", x="
            << command.target_x
            << ", y="
            << command.target_y
            << ", prediction_ms="
            << command.prediction_ms
            << '\n';

        print_bytes(
            "UART Frame",
            frame_buffer.data(),
            encoded_size);

        std::cout
            << "Bytes Transmitted: "
            << bytes_written
            << "\n\n";
    }
    else
    {
        /*
         * Live mode에서는 출력량을 줄인다.
         *
         * 나중에 latency 측정 단계에서
         * 불필요한 console I/O가 timing에 영향을
         * 주는 것을 최소화하기 위함이다.
         */
        std::cout
            << std::fixed
            << std::setprecision(3)
            << "[LIVE] "
            << "seq="
            << command.sequence
            << "  t="
            << detection.time_sec
            << "  pos=("
            << detection.target_x_m
            << ", "
            << detection.target_y_m
            << ")"
            << "  cmd=("
            << command.target_x
            << ", "
            << command.target_y
            << ")"
            << "  pred="
            << command.prediction_ms
            << "ms"
            << '\n';
    }

    return true;
}

int run_csv_mode(
    const char* csv_path,
    latc::transport::SerialPort* serial_port)
{
    latc::input::DetectionRecord detection{};

    if (!latc::input::read_first_detection_csv(
            csv_path,
            &detection))
    {
        std::cerr
            << "Failed to read detection CSV.\n";

        return 1;
    }

    std::cout
        << "Input Mode       : CSV Replay\n";

    if (!transmit_detection(
            serial_port,
            detection,
            1U,
            true))
    {
        return 1;
    }

    std::cout
        << "Transmission     : PASS\n";

    return 0;
}

int run_live_mode(
    latc::transport::SerialPort* serial_port)
{
    latc::input::UdpDetectionReceiver receiver;

    if (!receiver.open())
    {
        std::cerr
            << "Failed to open UDP detection receiver: "
            << std::strerror(errno)
            << '\n';

        return 1;
    }

    std::cout
        << "Input Mode       : LIVE UDP\n"
        << "UDP Listen       : 127.0.0.1:"
        << latc::input::UdpDetectionReceiver::kDefaultPort
        << '\n'
        << "Waiting for Simulink detections...\n\n";

    std::uint16_t sequence = 1U;

    while (true)
    {
        latc::input::DetectionRecord detection{};

        if (!receiver.receive(&detection))
        {
            std::cerr
                << "UDP receive failed: "
                << std::strerror(errno)
                << '\n';

            return 1;
        }

        if (!transmit_detection(
                serial_port,
                detection,
                sequence,
                false))
        {
            return 1;
        }

        ++sequence;
    }
}

}  // namespace

int main(int argc, char* argv[])
{
    /*
     * Usage:
     *
     * CSV Replay:
     *
     * ./target_control_pc \
     *     /dev/ttyACM0 \
     *     simulation/simulink/target_detections.csv
     *
     * Live:
     *
     * ./target_control_pc \
     *     /dev/ttyACM0 \
     *     --live
     */
    if (argc != 3)
    {
        std::cerr
            << "Usage:\n"
            << "  "
            << argv[0]
            << " <serial-device> <detection-csv>\n"
            << "  "
            << argv[0]
            << " <serial-device> --live\n";

        return 1;
    }

    const char* const serial_device_path = argv[1];

    const bool live_mode =
        std::strcmp(
            argv[2],
            "--live") == 0;

    latc::transport::SerialPort serial_port;

    std::cout
        << "Serial Device    : "
        << serial_device_path
        << '\n';

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

    int result = 0;

    if (live_mode)
    {
        result =
            run_live_mode(
                &serial_port);
    }
    else
    {
        result =
            run_csv_mode(
                argv[2],
                &serial_port);
    }

    if (!serial_port.close())
    {
        std::cerr
            << "Failed to close serial device: "
            << std::strerror(errno)
            << '\n';

        return 1;
    }

    return result;
}
