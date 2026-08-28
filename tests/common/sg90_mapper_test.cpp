#include <cmath>
#include <cstdint>
#include <iostream>

#include "actuator/sg90_mapper.h"

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

Sg90MapperConfig make_test_config()
{
    /*
     * These pulse widths are UNIT-TEST FIXTURES only.
     *
     * They are not yet the calibrated SG90 runtime values.
     */
    return Sg90MapperConfig{
        30.0F,     // steering_limit_deg
        20000U,    // period_us
        1200U,     // pulse_at_negative_limit_us
        1500U,     // pulse_at_center_us
        1800U      // pulse_at_positive_limit_us
    };
}

bool test_center_maps_to_center_pulse()
{
    const Sg90MapperConfig config =
        make_test_config();

    PwmCommand command{};

    CHECK(
        sg90_mapper_map(
            &config,
            0.0F,
            &command)
    );

    CHECK(command.period_us == 20000U);
    CHECK(command.pulse_width_us == 1500U);

    return true;
}

bool test_negative_limit_maps_correctly()
{
    const Sg90MapperConfig config =
        make_test_config();

    PwmCommand command{};

    CHECK(
        sg90_mapper_map(
            &config,
            -30.0F,
            &command)
    );

    CHECK(command.pulse_width_us == 1200U);

    return true;
}

bool test_positive_limit_maps_correctly()
{
    const Sg90MapperConfig config =
        make_test_config();

    PwmCommand command{};

    CHECK(
        sg90_mapper_map(
            &config,
            30.0F,
            &command)
    );

    CHECK(command.pulse_width_us == 1800U);

    return true;
}

bool test_negative_half_scale_interpolates()
{
    const Sg90MapperConfig config =
        make_test_config();

    PwmCommand command{};

    CHECK(
        sg90_mapper_map(
            &config,
            -15.0F,
            &command)
    );

    CHECK(command.pulse_width_us == 1350U);

    return true;
}

bool test_positive_half_scale_interpolates()
{
    const Sg90MapperConfig config =
        make_test_config();

    PwmCommand command{};

    CHECK(
        sg90_mapper_map(
            &config,
            15.0F,
            &command)
    );

    CHECK(command.pulse_width_us == 1650U);

    return true;
}

bool test_out_of_range_is_clamped()
{
    const Sg90MapperConfig config =
        make_test_config();

    PwmCommand command{};

    CHECK(
        sg90_mapper_map(
            &config,
            100.0F,
            &command)
    );

    CHECK(command.pulse_width_us == 1800U);

    CHECK(
        sg90_mapper_map(
            &config,
            -100.0F,
            &command)
    );

    CHECK(command.pulse_width_us == 1200U);

    return true;
}

bool test_reversed_servo_direction_is_supported()
{
    Sg90MapperConfig config =
        make_test_config();

    config.pulse_at_negative_limit_us = 1800U;
    config.pulse_at_center_us = 1500U;
    config.pulse_at_positive_limit_us = 1200U;

    PwmCommand command{};

    CHECK(
        sg90_mapper_map(
            &config,
            15.0F,
            &command)
    );

    CHECK(command.pulse_width_us == 1350U);

    return true;
}

bool test_invalid_configuration_is_rejected()
{
    Sg90MapperConfig config =
        make_test_config();

    config.steering_limit_deg = 0.0F;
    CHECK(!sg90_mapper_config_is_valid(&config));

    config = make_test_config();
    config.period_us = 0U;
    CHECK(!sg90_mapper_config_is_valid(&config));

    config = make_test_config();
    config.pulse_at_center_us = config.period_us;
    CHECK(!sg90_mapper_config_is_valid(&config));

    config = make_test_config();
    config.pulse_at_negative_limit_us = 1600U;
    config.pulse_at_center_us = 1500U;
    config.pulse_at_positive_limit_us = 1800U;
    CHECK(!sg90_mapper_config_is_valid(&config));

    return true;
}

bool test_invalid_arguments_are_rejected()
{
    const Sg90MapperConfig config =
        make_test_config();

    PwmCommand command{};

    CHECK(
        !sg90_mapper_map(
            nullptr,
            0.0F,
            &command)
    );

    CHECK(
        !sg90_mapper_map(
            &config,
            0.0F,
            nullptr)
    );

    CHECK(
        !sg90_mapper_map(
            &config,
            NAN,
            &command)
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
        {
            "center pulse",
            test_center_maps_to_center_pulse
        },
        {
            "negative limit",
            test_negative_limit_maps_correctly
        },
        {
            "positive limit",
            test_positive_limit_maps_correctly
        },
        {
            "negative interpolation",
            test_negative_half_scale_interpolates
        },
        {
            "positive interpolation",
            test_positive_half_scale_interpolates
        },
        {
            "clamp",
            test_out_of_range_is_clamped
        },
        {
            "reversed direction",
            test_reversed_servo_direction_is_supported
        },
        {
            "invalid configuration",
            test_invalid_configuration_is_rejected
        },
        {
            "invalid arguments",
            test_invalid_arguments_are_rejected
        }
    };

    for (const TestCase& test : tests)
    {
        if (!test.function())
        {
            std::cerr
                << "SG90 mapper test failed: "
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
        << "All SG90 mapper tests passed.\n";

    return 0;
}
