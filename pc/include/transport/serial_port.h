#ifndef LATC_PC_TRANSPORT_SERIAL_PORT_H
#define LATC_PC_TRANSPORT_SERIAL_PORT_H

#include <cstddef>
#include <cstdint>

namespace latc::transport
{

/**
 * @brief Linux Serial Port를 소유하고 바이트 데이터를 송신하는 Transport.
 *
 * 이 클래스는 UART Frame, TargetCommand, CRC 등 상위 프로토콜의
 * 내용을 해석하지 않는다.
 *
 * 하나의 SerialPort 객체는 최대 하나의 Linux file descriptor를
 * 독점적으로 소유한다.
 */
class SerialPort final
{
public:
    /**
     * 초기 UART 통신 속도.
     *
     * PC-UART-SWREQ-004
     */
    static constexpr std::uint32_t kBaudRate = 115200U;

    /**
     * 닫힌 상태의 SerialPort 객체를 생성한다.
     *
     * PC-UART-SWREQ-014
     */
    SerialPort() noexcept = default;

    /**
     * 객체가 소유한 Serial Port를 자동으로 닫는다.
     *
     * PC-UART-SWREQ-015
     */
    ~SerialPort() noexcept;

    /*
     * 동일한 file descriptor를 여러 객체가 소유하여 중복 close하는
     * 문제를 방지하기 위해 복사를 금지한다.
     *
     * PC-UART-SWREQ-016
     */
    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    /*
     * 초기 구현에서는 소유권 이전도 지원하지 않는다.
     * 필요해질 때 Move Constructor와 Move Assignment를 별도로
     * 설계할 수 있다.
     *
     * PC-UART-SWREQ-016
     */
    SerialPort(SerialPort&&) = delete;
    SerialPort& operator=(SerialPort&&) = delete;

    /**
     * @brief Linux Serial Device를 열고 115200 8N1 Raw Mode로 설정한다.
     *
     * @param device_path `/dev/ttyUSB0`, `/dev/ttyACM0` 형태의 장치 경로.
     *
     * @return Open 및 UART 설정이 모두 성공하면 true.
     * @return 실패하면 false. 실패 후 객체는 닫힌 상태를 유지한다.
     *
     * PC-UART-SWREQ-001
     * PC-UART-SWREQ-002
     * PC-UART-SWREQ-003
     * PC-UART-SWREQ-004
     * PC-UART-SWREQ-005
     * PC-UART-SWREQ-014
     * PC-UART-SWREQ-019
     */
    [[nodiscard]] bool open(const char* device_path) noexcept;

    /**
     * @brief 주어진 바이트 배열 전체를 Serial Port로 송신한다.
     *
     * 한 번의 write()가 전체 데이터를 처리한다고 가정하지 않는다.
     * Partial Write가 발생하면 남은 데이터를 계속 송신한다.
     *
     * @param data 송신할 바이트 배열.
     * @param data_size 송신할 바이트 수.
     * @param bytes_written 실제로 송신된 바이트 수를 기록할 출력 포인터.
     *
     * @return 전체 데이터 송신 성공 시 true.
     * @return 파라미터 오류 또는 송신 실패 시 false.
     *
     * PC-UART-SWREQ-006
     * PC-UART-SWREQ-007
     * PC-UART-SWREQ-008
     * PC-UART-SWREQ-009
     * PC-UART-SWREQ-010
     * PC-UART-SWREQ-011
     * PC-UART-SWREQ-012
     * PC-UART-SWREQ-013
     * PC-UART-SWREQ-019
     * PC-UART-SWREQ-020
     */
    [[nodiscard]] bool write_all(
        const std::uint8_t* data,
        std::size_t data_size,
        std::size_t* bytes_written) noexcept;

    /**
     * @brief 현재 열려 있는 Serial Port를 닫는다.
     *
     * 이미 닫힌 상태에서 호출해도 성공으로 처리한다.
     *
     * @return 닫기 작업이 성공하거나 이미 닫혀 있으면 true.
     * @return 운영체제의 close 작업이 실패하면 false.
     *
     * PC-UART-SWREQ-014
     * PC-UART-SWREQ-015
     * PC-UART-SWREQ-019
     */
    [[nodiscard]] bool close() noexcept;

    /**
     * @brief Serial Port가 현재 열려 있는지 반환한다.
     *
     * @return 유효한 file descriptor를 소유하고 있으면 true.
     * @return 닫혀 있으면 false.
     *
     * PC-UART-SWREQ-014
     */
    [[nodiscard]] bool is_open() const noexcept;

private:
    /**
     * 유효하지 않은 Linux file descriptor 값.
     */
    static constexpr int kInvalidFileDescriptor = -1;

    /**
     * SerialPort 객체가 독점적으로 소유하는 Linux file descriptor.
     */
    int file_descriptor_{kInvalidFileDescriptor};
};

}  // namespace latc::transport

#endif  // LATC_PC_TRANSPORT_SERIAL_PORT_H
