#include "input/udp_detection_receiver.h"

#include <array>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{

constexpr std::size_t kDetectionValueCount = 5U;
constexpr std::size_t kDoubleWireSize = 8U;

constexpr std::size_t kDetectionPacketSize =
    kDetectionValueCount * kDoubleWireSize;

/*
 * Simulink Python sender:
 *
 * struct.pack(
 *     "<5d",
 *     time_sec,
 *     target_x,
 *     target_y,
 *     target_vx,
 *     target_vy)
 *
 * 따라서 wire format은
 *
 * Little Endian IEEE-754 double × 5
 * = 40 bytes
 */
double read_double_le(const std::uint8_t* data)
{
    std::uint64_t bits = 0U;

    for (std::size_t i = 0U; i < kDoubleWireSize; ++i)
    {
        bits |=
            static_cast<std::uint64_t>(data[i])
            << (8U * i);
    }

    double value = 0.0;

    static_assert(
        sizeof(value) == sizeof(bits),
        "This implementation requires 64-bit double.");

    std::memcpy(
        &value,
        &bits,
        sizeof(value));

    return value;
}

bool detection_is_valid(
    const latc::input::DetectionRecord& detection)
{
    return
        std::isfinite(detection.time_sec) &&
        std::isfinite(detection.target_x_m) &&
        std::isfinite(detection.target_y_m) &&
        std::isfinite(detection.target_vx_mps) &&
        std::isfinite(detection.target_vy_mps);
}

}  // namespace

namespace latc::input
{

UdpDetectionReceiver::~UdpDetectionReceiver()
{
    if (socket_fd_ >= 0)
    {
        ::close(socket_fd_);
    }
}

bool UdpDetectionReceiver::open(
    std::uint16_t port)
{
    if (socket_fd_ >= 0)
    {
        return false;
    }

    socket_fd_ =
        ::socket(
            AF_INET,
            SOCK_DGRAM,
            0);

    if (socket_fd_ < 0)
    {
        return false;
    }

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    /*
     * 현재 Simulink와 C++ 프로그램은
     * 같은 Ubuntu PC에서 실행한다.
     */
    address.sin_addr.s_addr =
        htonl(INADDR_LOOPBACK);

    if (::bind(
            socket_fd_,
            reinterpret_cast<const sockaddr*>(&address),
            sizeof(address)) < 0)
    {
        const int saved_errno = errno;

        ::close(socket_fd_);
        socket_fd_ = -1;

        errno = saved_errno;

        return false;
    }

    return true;
}

bool UdpDetectionReceiver::receive(
    DetectionRecord* detection)
{
    if (socket_fd_ < 0 ||
        detection == nullptr)
    {
        return false;
    }

    /*
     * 정확히 40-byte Detection packet만 사용한다.
     *
     * 여유 있는 buffer를 사용하여
     * oversized UDP datagram도 구분 가능하게 한다.
     */
    std::array<std::uint8_t, 256U> buffer{};

    while (true)
    {
        const ssize_t bytes_received =
            ::recv(
                socket_fd_,
                buffer.data(),
                buffer.size(),
                0);

        if (bytes_received < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return false;
        }

        if (static_cast<std::size_t>(
                bytes_received) !=
            kDetectionPacketSize)
        {
            /*
             * 예상하지 않은 packet은 버리고
             * 다음 packet을 기다린다.
             */
            continue;
        }

        DetectionRecord parsed_detection{};

        parsed_detection.time_sec =
            read_double_le(
                buffer.data() + 0U);

        parsed_detection.target_x_m =
            read_double_le(
                buffer.data() + 8U);

        parsed_detection.target_y_m =
            read_double_le(
                buffer.data() + 16U);

        parsed_detection.target_vx_mps =
            read_double_le(
                buffer.data() + 24U);

        parsed_detection.target_vy_mps =
            read_double_le(
                buffer.data() + 32U);

        if (!detection_is_valid(
                parsed_detection))
        {
            continue;
        }

        *detection = parsed_detection;

        return true;
    }
}

bool UdpDetectionReceiver::close()
{
    if (socket_fd_ < 0)
    {
        return true;
    }

    if (::close(socket_fd_) < 0)
    {
        return false;
    }

    socket_fd_ = -1;

    return true;
}

bool UdpDetectionReceiver::is_open() const
{
    return socket_fd_ >= 0;
}

}  // namespace latc::input
