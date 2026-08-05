#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>

#include "protocol/target_command_codec.h"

namespace
{

void print_command(
    const char* title,
    const TargetCommand& command)
{
    std::cout
        << title << '\n'
        << "  sequence      : " << command.sequence << '\n'
        << "  target_x      : " << command.target_x << '\n'
        << "  target_y      : " << command.target_y << '\n'
        << "  prediction_ms : " << command.prediction_ms << '\n';
}

void print_buffer(
    const std::array<uint8_t, TARGET_COMMAND_WIRE_SIZE>& buffer)
{
    std::cout << "Serialized bytes : ";

    for (const uint8_t byte : buffer)
    {
        std::cout
            << std::hex
            << std::uppercase
            << std::setw(2)
            << std::setfill('0')
            << static_cast<unsigned int>(byte)
            << ' ';
    }

    std::cout
        << std::dec
        << std::setfill(' ')
        << '\n';
}

} // namespace

int main()
{
    std::cout
        << "Latency-Aware Target Control PC mock started.\n\n";

    const TargetCommand original_command{
        1U,      // sequence
        1000,    // target_x
        -1000,   // target_y
        40U      // prediction_ms
    };

    print_command(
        "Original TargetCommand",
        original_command
    );

    std::array<uint8_t, TARGET_COMMAND_WIRE_SIZE> buffer{};

    const bool serialize_success = target_command_serialize(
        &original_command,
        buffer.data(),
        buffer.size()
    );

    if (!serialize_success)
    {
        std::cerr << "TargetCommand serialization failed.\n";
        return 1;
    }

    std::cout << '\n';
    print_buffer(buffer);

    TargetCommand decoded_command{};

    const bool deserialize_success = target_command_deserialize(
        buffer.data(),
        buffer.size(),
        &decoded_command
    );

    if (!deserialize_success)
    {
        std::cerr << "TargetCommand deserialization failed.\n";
        return 1;
    }

    std::cout << '\n';

    print_command(
        "Decoded TargetCommand",
        decoded_command
    );

    return 0;
}
