#include "transport/serial_port.h"

#include <cerrno>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace
{

/**
 * @brief 열린 Serial Device를 115200 8N1 Raw Mode로 설정한다.
 *
 * 성공하면 true를 반환한다.
 * 실패하면 errno는 실패한 POSIX 함수가 설정한 값을 유지한다.
 */
bool configure_serial_port(int file_descriptor) noexcept
{
    termios configuration{};

    /*
     * 현재 Serial Device 설정을 읽는다.
     */
    if (::tcgetattr(file_descriptor, &configuration) != 0)
    {
        return false;
    }

    /*
     * 입력 및 출력 Baud Rate를 115200으로 설정한다.
     *
     * 정수 115200을 직접 넣지 않고 termios가 정의한
     * B115200 상수를 사용한다.
     */
    if (::cfsetispeed(&configuration, B115200) != 0)
    {
        return false;
    }

    if (::cfsetospeed(&configuration, B115200) != 0)
    {
        return false;
    }

    /*
     * Character Size를 8비트로 설정한다.
     */
    configuration.c_cflag &= ~CSIZE;
    configuration.c_cflag |= CS8;

    /*
     * Parity 비트를 사용하지 않는다.
     */
    configuration.c_cflag &= ~PARENB;

    /*
     * Stop Bit를 1개 사용한다.
     *
     * CSTOPB가 설정되면 Stop Bit 2개이고,
     * 해제되면 Stop Bit 1개다.
     */
    configuration.c_cflag &= ~CSTOPB;

    /*
     * Hardware Flow Control을 비활성화한다.
     *
     * CRTSCTS는 Linux에서 제공되지만 일부 환경에서는
     * 정의되지 않을 수 있으므로 조건부로 처리한다.
     */
#ifdef CRTSCTS
    configuration.c_cflag &= ~CRTSCTS;
#endif

    /*
     * CLOCAL:
     * 모뎀 제어선을 사용하지 않는다.
     *
     * CREAD:
     * 수신 기능을 활성화한다.
     *
     * 현재 Take에서는 송신만 사용하지만, Serial Device 자체는
     * 송수신 가능한 일반적인 형태로 설정한다.
     */
    configuration.c_cflag |= CLOCAL;
    configuration.c_cflag |= CREAD;

    /*
     * 입력 데이터 변환과 Software Flow Control을 비활성화한다.
     *
     * UART Frame의 바이트가 Linux에 의해 변경되지 않게 한다.
     */
    configuration.c_iflag &= ~(
        IGNBRK |
        BRKINT |
        PARMRK |
        ISTRIP |
        INLCR |
        IGNCR |
        ICRNL |
        IXON |
        IXOFF |
        IXANY);

    /*
     * 출력 후처리를 비활성화한다.
     *
     * 예를 들어 개행 문자를 변환하는 등의 처리를 막는다.
     */
    configuration.c_oflag &= ~OPOST;

    /*
     * Canonical Mode, Echo, Signal 처리 등을 비활성화한다.
     *
     * 터미널 문자 입력이 아니라 순수한 바이트 스트림으로 다룬다.
     */
    configuration.c_lflag &= ~(
        ECHO |
        ECHONL |
        ICANON |
        ISIG |
        IEXTEN);

    /*
     * read() 동작에 영향을 주는 설정이다.
     *
     * 현재 수신은 범위 밖이지만, Non-canonical Raw Mode에서
     * 즉시 반환하도록 초기화해 둔다.
     */
    configuration.c_cc[VMIN] = 0;
    configuration.c_cc[VTIME] = 0;

    /*
     * 변경한 설정을 즉시 적용한다.
     */
    return ::tcsetattr(
               file_descriptor,
               TCSANOW,
               &configuration) == 0;
}

/**
 * @brief 기존 errno 값을 보존하면서 file descriptor를 닫는다.
 *
 * 설정 실패 원인을 호출자가 errno로 확인할 수 있도록,
 * close()가 errno를 덮어쓰지 않게 한다.
 */
void close_preserving_errno(int file_descriptor) noexcept
{
    const int saved_errno = errno;

    (void)::close(file_descriptor);

    errno = saved_errno;
}

}  // namespace

namespace latc::transport
{

SerialPort::~SerialPort() noexcept
{
    /*
     * 소멸자는 실패를 반환할 수 없으므로 반환값을 명시적으로 무시한다.
     */
    (void)close();
}

bool SerialPort::open(const char* device_path) noexcept
{
    /*
     * PC-UART-SWREQ-002:
     * null 또는 빈 문자열 경로를 거부한다.
     */
    if (device_path == nullptr || device_path[0] == '\0')
    {
        errno = EINVAL;
        return false;
    }

    /*
     * 이미 다른 장치를 소유하고 있다면 새 Open을 거부한다.
     * 기존 연결은 그대로 유지한다.
     */
    if (is_open())
    {
        errno = EALREADY;
        return false;
    }

    /*
     * O_RDWR:
     * 장치를 읽기 및 쓰기 모드로 연다.
     *
     * O_NOCTTY:
     * 이 Serial Device가 현재 프로세스의 제어 터미널이 되는 것을 막는다.
     *
     * O_CLOEXEC:
     * 향후 다른 프로그램을 exec()할 때 이 descriptor가
     * 의도치 않게 상속되는 것을 막는다.
     */
    const int opened_file_descriptor = ::open(
        device_path,
        O_RDWR | O_NOCTTY | O_CLOEXEC);

    if (opened_file_descriptor < 0)
    {
        /*
         * ::open()이 실패 원인을 errno에 설정한다.
         */
        return false;
    }

    /*
     * 장치를 열었더라도 UART 설정이 실패하면 사용할 수 없다.
     *
     * 설정이 완전히 성공하기 전까지는 file_descriptor_에 저장하지
     * 않으므로 객체는 계속 Closed 상태다.
     */
    if (!configure_serial_port(opened_file_descriptor))
    {
        close_preserving_errno(opened_file_descriptor);
        return false;
    }

    /*
     * Open과 설정이 모두 성공한 뒤에만 객체가 descriptor를 소유한다.
     */
    file_descriptor_ = opened_file_descriptor;

    return true;
}

bool SerialPort::write_all(
    const std::uint8_t* data,
    std::size_t data_size,
    std::size_t* bytes_written) noexcept
{
    /*
     * 출력 포인터가 없으면 결과를 전달할 수 없으므로 거부한다.
     */
    if (bytes_written == nullptr)
    {
        errno = EINVAL;
        return false;
    }

    /*
     * 크기가 0보다 큰 경우에는 유효한 데이터 주소가 필요하다.
     */
    if (data_size > 0U && data == nullptr)
    {
        errno = EINVAL;
        return false;
    }

    /*
     * 0바이트 송신은 성공이다.
     *
     * 이 경우 장치가 열려 있지 않아도 실제 write()를 수행하지
     * 않으므로 성공으로 처리한다.
     */
    if (data_size == 0U)
    {
        *bytes_written = 0U;
        return true;
    }

    /*
     * 실제 송신할 데이터가 있는데 포트가 닫혀 있으면 거부한다.
     *
     * 전송이 시작되지 않았으므로 bytes_written 값은 변경하지 않는다.
     */
    if (!is_open())
    {
        errno = EBADF;
        return false;
    }

    std::size_t total_bytes_written = 0U;

    while (total_bytes_written < data_size)
    {
        const std::size_t remaining_size =
            data_size - total_bytes_written;

        /*
         * data + total_bytes_written:
         * 아직 보내지 않은 첫 번째 바이트의 주소.
         *
         * remaining_size:
         * 아직 남아 있는 바이트 수.
         */
        const ssize_t write_result = ::write(
            file_descriptor_,
            data + total_bytes_written,
            remaining_size);

        if (write_result > 0)
        {
            total_bytes_written +=
                static_cast<std::size_t>(write_result);

            continue;
        }

        /*
         * Signal 때문에 write()가 중단됐다면 다시 시도한다.
         */
        if (write_result < 0 && errno == EINTR)
        {
            continue;
        }

        /*
         * 양수 크기를 요청했는데 write()가 0을 반환하면,
         * 무한 반복을 방지하기 위해 전송 실패로 처리한다.
         */
        if (write_result == 0)
        {
            errno = EIO;
        }

        /*
         * 실제 전송이 시작된 이후의 실패이므로,
         * 여기까지 성공한 바이트 수를 호출자에게 전달한다.
         */
        *bytes_written = total_bytes_written;
        return false;
    }

    *bytes_written = total_bytes_written;

    return true;
}

bool SerialPort::close() noexcept
{
    /*
     * 이미 닫혀 있다면 아무 작업 없이 성공한다.
     */
    if (!is_open())
    {
        return true;
    }

    /*
     * 실제 close()를 호출하기 전에 객체 상태를 Closed로 바꾼다.
     *
     * close() 실패 후 동일 descriptor를 무작정 다시 닫는 것을
     * 방지하고 객체가 중복 소유하지 않도록 한다.
     */
    const int file_descriptor_to_close = file_descriptor_;
    file_descriptor_ = kInvalidFileDescriptor;

    return ::close(file_descriptor_to_close) == 0;
}

bool SerialPort::is_open() const noexcept
{
    return file_descriptor_ != kInvalidFileDescriptor;
}

}  // namespace latc::transport
