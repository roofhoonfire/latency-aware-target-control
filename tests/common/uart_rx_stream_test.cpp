#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include "protocol/uart_rx_stream.h"
#include "protocol/uart_frame_codec.h"

int main()
{
    TargetCommand input = {7, 515, -97, 0}, output{};
    std::array<uint8_t, TARGET_COMMAND_WIRE_SIZE> payload{};
    std::array<uint8_t, TARGET_FRAME_SIZE> frame{};
    size_t size = 0;
    assert(target_command_serialize(&input, payload.data(), payload.size()));
    assert(uart_frame_encode(UART_MESSAGE_TYPE_TARGET_COMMAND, payload.data(),
        payload.size(), frame.data(), frame.size(), &size));
    assert(size == frame.size());

    // Every possible gap position must prevent joining an old prefix to a
    // post-loss suffix, even if those bytes would otherwise have a valid CRC.
    for (size_t gap = 1; gap < frame.size(); ++gap) {
        UartRxStream parser{};
        unsigned count = 0;
        for (size_t i = 0; i < frame.size(); ++i) {
            UartRxByte item{static_cast<uint32_t>(i < gap ? 0 : 1), frame[i]};
            count += uart_rx_stream_push(&parser, &item, &output);
        }
        assert(count == 0 && parser.gap_resets == 1);
        for (auto byte : frame) {
            UartRxByte item{1, byte};
            count += uart_rx_stream_push(&parser, &item, &output);
        }
        assert(count == 1 && output.target_x == 515 && output.target_y == -97);
    }
    // Queued complete pre-loss frames remain usable; the next generation
    // resets only an incomplete parser. Epoch wrap is an ordinary change.
    UartRxStream parser{};
    unsigned count = 0;
    for (uint32_t epoch : {UINT32_MAX, 0U, 9U}) {
        for (auto byte : frame) {
            UartRxByte item{epoch, byte};
            count += uart_rx_stream_push(&parser, &item, &output);
        }
    }
    assert(count == 3 && parser.gap_resets == 3);
    std::cout << "UART stream gaps at every frame boundary and epoch wrap: PASS\n";
}
