#include "transport/serial_port.h"

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <stdlib.h>
#include <termios.h>
#include <type_traits>
#include <unistd.h>

namespace
{

constexpr int kReadTimeoutMilliseconds = 1000;

/*
 * PC-UART-SWREQ-016:
 * SerialPort 객체는 file descriptor를 독점적으로 소유하므로
 * 복사와 이동이 모두 금지되어야 한다.
 */
static_assert(
    !std::is_copy_constructible_v<latc::transport::SerialPort>);

static_assert(
    !std::is_copy_assignable_v<latc::transport::SerialPort>);

static_assert(
    !std::is_move_constructible_v<latc::transport::SerialPort>);

static_assert(
    !std::is_move_assignable_v<latc::transport::SerialPort>);

/**
 * @brief Linux pseudo-terminal Master와 Slave 경로를 관리한다.
 *
 * 테스트에서는 SerialPort가 Slave를 열고,
 * 테스트 코드가 Master에서 전송된 데이터를 읽는다.
 */
class PseudoTerminal final
{
public:
    PseudoTerminal() noexcept = default;

    ~PseudoTerminal() noexcept
    {
        if (master_file_descriptor_ >= 0)
        {
            (void)::close(master_file_descriptor_);
        }
    }

    PseudoTerminal(const PseudoTerminal&) = delete;
    PseudoTerminal& operator=(const PseudoTerminal&) = delete;
    PseudoTerminal(PseudoTerminal&&) = delete;
    PseudoTerminal& operator=(PseudoTerminal&&) = delete;

    [[nodiscard]] bool initialize() noexcept
    {
        /*
         * pseudo-terminal Master를 생성한다.
         */
        master_file_descriptor_ = ::posix_openpt(
            O_RDWR | O_NOCTTY | O_CLOEXEC);

        if (master_file_descriptor_ < 0)
        {
            return false;
        }

        /*
         * 대응되는 Slave 장치를 현재 사용자에게 접근 가능하게 한다.
         */
        if (::grantpt(master_file_descriptor_) != 0)
        {
            return false;
        }

        /*
         * Slave 장치를 사용할 수 있도록 잠금을 해제한다.
         */
        if (::unlockpt(master_file_descriptor_) != 0)
        {
            return false;
        }

        /*
         * Master에 대응되는 Slave 장치 경로를 얻는다.
         *
         * 예:
         * /dev/pts/3
         */
        const char* generated_slave_path =
            ::ptsname(master_file_descriptor_);

        if (generated_slave_path == nullptr)
        {
            return false;
        }

        const std::size_t path_length =
            std::strlen(generated_slave_path);

        if (path_length >= slave_path_.size())
        {
            errno = ENAMETOOLONG;
            return false;
        }

        std::memcpy(
            slave_path_.data(),
            generated_slave_path,
            path_length + 1U);

        return true;
    }

    [[nodiscard]] int master_file_descriptor() const noexcept
    {
        return master_file_descriptor_;
    }

    [[nodiscard]] const char* slave_path() const noexcept
    {
        return slave_path_.data();
    }

private:
    int master_file_descriptor_{-1};
    std::array<char, 128U> slave_path_{};
};

/**
 * @brief 지정된 크기의 데이터를 timeout 내에 전부 읽는다.
 */
[[nodiscard]] bool read_exact(
    int file_descriptor,
    std::uint8_t* output,
    std::size_t output_size) noexcept
{
    if (output == nullptr && output_size > 0U)
    {
        return false;
    }

    std::size_t total_bytes_read = 0U;

    while (total_bytes_read < output_size)
    {
        pollfd descriptor_status{};
        descriptor_status.fd = file_descriptor;
        descriptor_status.events = POLLIN;

        int poll_result = 0;

        do
        {
            poll_result = ::poll(
                &descriptor_status,
                1,
                kReadTimeoutMilliseconds);
        }
        while (poll_result < 0 && errno == EINTR);

        /*
         * 0이면 timeout, 음수이면 오류다.
         */
        if (poll_result <= 0)
        {
            return false;
        }

        ssize_t read_result = 0;

        do
        {
            read_result = ::read(
                file_descriptor,
                output + total_bytes_read,
                output_size - total_bytes_read);
        }
        while (read_result < 0 && errno == EINTR);

        if (read_result <= 0)
        {
            return false;
        }

        total_bytes_read +=
            static_cast<std::size_t>(read_result);
    }

    return true;
}

/**
 * @brief 새 pseudo-terminal과 열린 SerialPort를 준비한다.
 */
[[nodiscard]] bool open_test_serial_port(
    PseudoTerminal& pseudo_terminal,
    latc::transport::SerialPort& serial_port) noexcept
{
    if (!pseudo_terminal.initialize())
    {
        return false;
    }

    return serial_port.open(
        pseudo_terminal.slave_path());
}

/**
 * PC-UART-SWREQ-014
 * PC-UART-SWREQ-015
 */
[[nodiscard]] bool test_initial_and_closed_state()
{
    latc::transport::SerialPort serial_port;

    if (serial_port.is_open())
    {
        return false;
    }

    /*
     * 이미 닫힌 객체를 닫는 것은 안전하며 성공해야 한다.
     */
    if (!serial_port.close())
    {
        return false;
    }

    return !serial_port.is_open();
}

/**
 * PC-UART-SWREQ-002
 */
[[nodiscard]] bool test_invalid_device_paths()
{
    latc::transport::SerialPort serial_port;

    if (serial_port.open(nullptr))
    {
        return false;
    }

    if (serial_port.is_open())
    {
        return false;
    }

    if (serial_port.open(""))
    {
        return false;
    }

    if (serial_port.is_open())
    {
        return false;
    }

    if (serial_port.open(
            "/dev/latc-device-that-does-not-exist"))
    {
        return false;
    }

    return !serial_port.is_open();
}

/**
 * PC-UART-SWREQ-001
 * PC-UART-SWREQ-003
 * PC-UART-SWREQ-014
 *
 * 이미 열린 상태에서 두 번째 open() 요청은 거부되고
 * 기존 연결은 유지돼야 한다.
 */
[[nodiscard]] bool test_open_and_duplicate_open()
{
    PseudoTerminal pseudo_terminal;
    latc::transport::SerialPort serial_port;

    if (!open_test_serial_port(
            pseudo_terminal,
            serial_port))
    {
        return false;
    }

    if (!serial_port.is_open())
    {
        return false;
    }

    if (serial_port.open(
            pseudo_terminal.slave_path()))
    {
        return false;
    }

    if (!serial_port.is_open())
    {
        return false;
    }

    if (!serial_port.close())
    {
        return false;
    }

    if (serial_port.is_open())
    {
        return false;
    }

    /*
     * 반복 close도 안전해야 한다.
     */
    return serial_port.close();
}

/**
 * PC-UART-SWREQ-004
 *
 * SerialPort가 실제로 설정한 termios 값을 검사한다.
 */
[[nodiscard]] bool test_uart_configuration()
{
    PseudoTerminal pseudo_terminal;
    latc::transport::SerialPort serial_port;

    if (!open_test_serial_port(
            pseudo_terminal,
            serial_port))
    {
        return false;
    }

    /*
     * 같은 Slave 장치를 검사 목적으로 별도 Open한다.
     */
    const int inspection_file_descriptor = ::open(
        pseudo_terminal.slave_path(),
        O_RDWR | O_NOCTTY | O_CLOEXEC);

    if (inspection_file_descriptor < 0)
    {
        return false;
    }

    termios configuration{};

    const bool get_attributes_succeeded =
        ::tcgetattr(
            inspection_file_descriptor,
            &configuration) == 0;

    (void)::close(inspection_file_descriptor);

    if (!get_attributes_succeeded)
    {
        return false;
    }

    if (::cfgetispeed(&configuration) != B115200)
    {
        return false;
    }

    if (::cfgetospeed(&configuration) != B115200)
    {
        return false;
    }

    if ((configuration.c_cflag & CSIZE) != CS8)
    {
        return false;
    }

    if ((configuration.c_cflag & PARENB) != 0U)
    {
        return false;
    }

    if ((configuration.c_cflag & CSTOPB) != 0U)
    {
        return false;
    }

#ifdef CRTSCTS
    if ((configuration.c_cflag & CRTSCTS) != 0U)
    {
        return false;
    }
#endif

    if ((configuration.c_cflag & CLOCAL) == 0U)
    {
        return false;
    }

    if ((configuration.c_cflag & CREAD) == 0U)
    {
        return false;
    }

    if ((configuration.c_iflag & IXON) != 0U)
    {
        return false;
    }

    if ((configuration.c_iflag & IXOFF) != 0U)
    {
        return false;
    }

    if ((configuration.c_iflag & IXANY) != 0U)
    {
        return false;
    }

    if ((configuration.c_oflag & OPOST) != 0U)
    {
        return false;
    }

    if ((configuration.c_lflag & ICANON) != 0U)
    {
        return false;
    }

    if ((configuration.c_lflag & ECHO) != 0U)
    {
        return false;
    }

    if ((configuration.c_lflag & ISIG) != 0U)
    {
        return false;
    }

    if ((configuration.c_lflag & IEXTEN) != 0U)
    {
        return false;
    }

    if (configuration.c_cc[VMIN] != 0)
    {
        return false;
    }

    if (configuration.c_cc[VTIME] != 0)
    {
        return false;
    }

    return true;
}

/**
 * PC-UART-SWREQ-010
 * PC-UART-SWREQ-011
 * PC-UART-SWREQ-012
 * PC-UART-SWREQ-013
 */
[[nodiscard]] bool test_write_parameter_validation()
{
    latc::transport::SerialPort serial_port;

    std::size_t bytes_written = 1234U;

    /*
     * null data + nonzero size는 실패해야 한다.
     * 검증 단계 실패이므로 출력값은 보존돼야 한다.
     */
    if (serial_port.write_all(
            nullptr,
            1U,
            &bytes_written))
    {
        return false;
    }

    if (bytes_written != 1234U)
    {
        return false;
    }

    const std::array<std::uint8_t, 1U> one_byte{
        0xAAU
    };

    /*
     * bytes_written 출력 포인터가 null이면 실패해야 한다.
     */
    if (serial_port.write_all(
            one_byte.data(),
            one_byte.size(),
            nullptr))
    {
        return false;
    }

    /*
     * 닫힌 Port의 nonzero 송신은 실패하고
     * 출력값을 보존해야 한다.
     */
    if (serial_port.write_all(
            one_byte.data(),
            one_byte.size(),
            &bytes_written))
    {
        return false;
    }

    if (bytes_written != 1234U)
    {
        return false;
    }

    /*
     * Zero-length 송신은 Port가 닫혀 있어도 성공한다.
     */
    if (!serial_port.write_all(
            nullptr,
            0U,
            &bytes_written))
    {
        return false;
    }

    return bytes_written == 0U;
}

/**
 * PC-UART-SWREQ-006
 * PC-UART-SWREQ-007
 * PC-UART-SWREQ-008
 * PC-UART-SWREQ-013
 *
 * Transport는 Frame 내용을 해석하지 않지만,
 * 실제 프로젝트의 15바이트 Frame 크기를 대표하는 값을
 * 그대로 전달할 수 있는지 검사한다.
 */
[[nodiscard]] bool test_exact_fifteen_byte_transmission()
{
    PseudoTerminal pseudo_terminal;
    latc::transport::SerialPort serial_port;

    if (!open_test_serial_port(
            pseudo_terminal,
            serial_port))
    {
        return false;
    }

    const std::array<std::uint8_t, 15U> transmitted_bytes{
        0xAAU,
        0x55U,
        0x01U,
        0x01U,
        0x08U,
        0x01U,
        0x00U,
        0xE8U,
        0x03U,
        0x18U,
        0xFCU,
        0x28U,
        0x00U,
        0x9FU,
        0xE2U
    };

    std::size_t bytes_written = 0U;

    if (!serial_port.write_all(
            transmitted_bytes.data(),
            transmitted_bytes.size(),
            &bytes_written))
    {
        return false;
    }

    if (bytes_written != transmitted_bytes.size())
    {
        return false;
    }

    std::array<std::uint8_t, 15U> received_bytes{};

    if (!read_exact(
            pseudo_terminal.master_file_descriptor(),
            received_bytes.data(),
            received_bytes.size()))
    {
        return false;
    }

    return std::memcmp(
               transmitted_bytes.data(),
               received_bytes.data(),
               transmitted_bytes.size()) == 0;
}

using TestFunction = bool (*)();

struct TestCase
{
    const char* name;
    TestFunction function;
};

}  // namespace

int main()
{
    const std::array<TestCase, 6U> test_cases{{
        {
            "initial and closed state",
            test_initial_and_closed_state
        },
        {
            "invalid device paths",
            test_invalid_device_paths
        },
        {
            "open and duplicate open",
            test_open_and_duplicate_open
        },
        {
            "UART configuration",
            test_uart_configuration
        },
        {
            "write parameter validation",
            test_write_parameter_validation
        },
        {
            "exact 15-byte transmission",
            test_exact_fifteen_byte_transmission
        }
    }};

    std::size_t passed_test_count = 0U;

    for (const TestCase& test_case : test_cases)
    {
        const bool passed = test_case.function();

        std::cout
            << (passed ? "[PASS] " : "[FAIL] ")
            << test_case.name
            << '\n';

        if (passed)
        {
            ++passed_test_count;
        }
    }

    std::cout
        << passed_test_count
        << " / "
        << test_cases.size()
        << " tests passed\n";

    return passed_test_count == test_cases.size()
        ? 0
        : 1;
}
