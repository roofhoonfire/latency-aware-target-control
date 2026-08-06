#include <array>
#include <cstdint>
#include <iostream>

#include "protocol/crc16.h"

#define CHECK(condition)                                      \
    do                                                        \
    {                                                         \
        if (!(condition))                                     \
        {                                                     \
            std::cerr                                         \
                << "[FAIL] "                                  \
                << __func__                                   \
                << ":" << __LINE__                            \
                << " - " << #condition                        \
                << '\n';                                      \
            return false;                                     \
        }                                                     \
    } while (false)

namespace
{

bool test_standard_check_value()
{
    const std::array<uint8_t, 9U> data{
        '1', '2', '3', '4', '5',
        '6', '7', '8', '9'
    };

    uint16_t crc = 0U;

    CHECK(
        crc16_ccitt_false_calculate(
            data.data(),
            data.size(),
            &crc
        )
    );

    CHECK(crc == 0x29B1U);

    return true;
}

bool test_empty_data()
{
    uint16_t crc = 0U;

    CHECK(
        crc16_ccitt_false_calculate(
            nullptr,
            0U,
            &crc
        )
    );

    /*
     * CRC of an empty input remains the initial value.
     */
    CHECK(crc == 0xFFFFU);

    return true;
}

bool test_null_data_with_nonzero_size()
{
    uint16_t crc = 0x1234U;

    CHECK(
        !crc16_ccitt_false_calculate(
            nullptr,
            1U,
            &crc
        )
    );

    /*
     * Output shall remain unchanged on failure.
     */
    CHECK(crc == 0x1234U);

    return true;
}

bool test_null_output()
{
    const std::array<uint8_t, 1U> data{0x01U};

    CHECK(
        !crc16_ccitt_false_calculate(
            data.data(),
            data.size(),
            nullptr
        )
    );

    return true;
}

struct TestCase
{
    const char* name;
    bool (*function)();
};

} // namespace

int main()
{
    const TestCase tests[]{
        {"standard check value", test_standard_check_value},
        {"empty data", test_empty_data},
        {
            "null data with nonzero size",
            test_null_data_with_nonzero_size
        },
        {"null output", test_null_output}
    };

    for (const TestCase& test : tests)
    {
        if (!test.function())
        {
            std::cerr
                << "CRC16 test failed: "
                << test.name
                << '\n';

            return 1;
        }

        std::cout
            << "[PASS] "
            << test.name
            << '\n';
    }

    std::cout
        << "All CRC16 tests passed.\n";

    return 0;
}
