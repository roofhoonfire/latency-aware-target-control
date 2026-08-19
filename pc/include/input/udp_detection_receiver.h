#ifndef LATC_PC_INPUT_UDP_DETECTION_RECEIVER_H
#define LATC_PC_INPUT_UDP_DETECTION_RECEIVER_H

#include <cstdint>

#include "input/detection_record.h"

namespace latc::input
{

class UdpDetectionReceiver
{
public:
    static constexpr std::uint16_t kDefaultPort = 51001U;

    UdpDetectionReceiver() = default;
    ~UdpDetectionReceiver();

    UdpDetectionReceiver(
        const UdpDetectionReceiver&) = delete;

    UdpDetectionReceiver& operator=(
        const UdpDetectionReceiver&) = delete;

    [[nodiscard]] bool open(
        std::uint16_t port = kDefaultPort);

    [[nodiscard]] bool receive(
        DetectionRecord* detection);

    [[nodiscard]] bool close();

    [[nodiscard]] bool is_open() const;

private:
    int socket_fd_{-1};
};

}  // namespace latc::input

#endif  // LATC_PC_INPUT_UDP_DETECTION_RECEIVER_H
