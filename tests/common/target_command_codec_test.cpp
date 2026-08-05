#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>

#include "protocol/target_command_codec.h"
// This is where we test our requirements PROTO - 001~ 004
namespace
{

    void test_known_byte_pattern()
    {
        const TargetCommand command{
            0x1234U,  // sequence
            0x0102,   // target_x
            0x0304,   // target_y
            0x0506U   // prediction_ms
        };

        std::array<uint8_t, TARGET_COMMAND_WIRE_SIZE> buffer{};

        const bool result = target_command_serialize(
            &command,
            buffer.data(),
            buffer.size()
        );

        assert(result);

        const std::array<uint8_t, TARGET_COMMAND_WIRE_SIZE> expected{
            0x34U, 0x12U,
            0x02U, 0x01U,
            0x04U, 0x03U,
            0x06U, 0x05U
        };

        assert(buffer == expected);
    }

    void test_negative_coordinates()
    {
        const TargetCommand original{
            10U,
            -100,
            -200,
            40U
        };

        std::array<uint8_t, TARGET_COMMAND_WIRE_SIZE> buffer{};

        const bool serialize_result = target_command_serialize(
            &original,
            buffer.data(),
            buffer.size()
        );

        assert(serialize_result);

        TargetCommand decoded{};

        const bool deserialize_result = target_command_deserialize(
            buffer.data(),
            buffer.size(),
            &decoded
        );

        assert(deserialize_result);

        assert(decoded.sequence == original.sequence);
        assert(decoded.target_x == original.target_x);
        assert(decoded.target_y == original.target_y);
        assert(decoded.prediction_ms == original.prediction_ms);
    }

    void test_boundary_values()
    {
        const TargetCommand original{
            std::numeric_limits<uint16_t>::max(),
            std::numeric_limits<int16_t>::min(),
            std::numeric_limits<int16_t>::max(),
            std::numeric_limits<uint16_t>::max()
        };

        std::array<uint8_t, TARGET_COMMAND_WIRE_SIZE> buffer{};

        assert(target_command_serialize(
            &original,
            buffer.data(),
            buffer.size()
        ));

        TargetCommand decoded{};

        assert(target_command_deserialize(
            buffer.data(),
            buffer.size(),
            &decoded
        ));

        assert(decoded.sequence == original.sequence);
        assert(decoded.target_x == original.target_x);
        assert(decoded.target_y == original.target_y);
        assert(decoded.prediction_ms == original.prediction_ms);
    }

    void test_invalid_arguments()
    {
        const TargetCommand command{
            1U,
            100,
            -100,
            40U
        };

        std::array<uint8_t, TARGET_COMMAND_WIRE_SIZE> buffer{};
        std::array<uint8_t, TARGET_COMMAND_WIRE_SIZE - 1U> small_buffer{};

        TargetCommand decoded{};

        assert(!target_command_serialize(
            nullptr,
            buffer.data(),
            buffer.size()
        ));

        assert(!target_command_serialize(
            &command,
            nullptr,
            buffer.size()
        ));

        assert(!target_command_serialize(
            &command,
            small_buffer.data(),
            small_buffer.size()
        ));

        assert(!target_command_deserialize(
            nullptr,
            buffer.size(),
            &decoded
        ));

        assert(!target_command_deserialize(
            buffer.data(),
            buffer.size(),
            nullptr
        ));

        assert(!target_command_deserialize(
            small_buffer.data(),
            small_buffer.size(),
            &decoded
        ));
    }

} // namespace

int main()
{
    test_known_byte_pattern();
    test_negative_coordinates();
    test_boundary_values();
    test_invalid_arguments();

    std::cout << "All TargetCommand codec tests passed.\n";

    return 0;
}