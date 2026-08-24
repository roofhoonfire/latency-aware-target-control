#include <cmath>
#include <cstdint>
#include <iostream>

#include "control/steering_controller.h"

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

#define CHECK_NEAR(actual, expected, tolerance)                \
    do                                                        \
    {                                                         \
        const float actual_value = (actual);                   \
        const float expected_value = (expected);               \
                                                                \
        if (std::fabs(actual_value - expected_value) >         \
            (tolerance))                                       \
        {                                                       \
            std::cerr                                           \
                << "[FAIL] "                                    \
                << __func__                                     \
                << ":" << __LINE__                              \
                << " - actual=" << actual_value                 \
                << ", expected=" << expected_value              \
                << ", tolerance=" << (tolerance)                \
                << '\n';                                        \
            return false;                                       \
        }                                                       \
    } while (false)

namespace
{

/*
 * This timeout is only a unit-test fixture.
 *
 * It is NOT the final runtime timeout requirement.
 */
constexpr uint32_t kTestTimeoutMs = 300U;

constexpr float kToleranceDeg = 0.001F;

SteeringControllerConfig make_test_config()
{
    return SteeringControllerConfig{
        2.8F,              // wheelbase_m
        1.0F,              // deadband_deg
        30.0F,             // steering_limit_deg
        kTestTimeoutMs      // target_timeout_ms
    };
}

bool test_center_target_returns_zero()
{
    SteeringController controller{};
    const SteeringControllerConfig config =
        make_test_config();

    CHECK(
        steering_controller_init(
            &controller,
            &config)
    );

    const TargetState target{
        1000,   // 10.0 m forward
        0
    };

    CHECK(
        steering_controller_update_target(
            &controller,
            &target,
            1000U)
    );

    CHECK_NEAR(
        steering_controller_step(
            &controller,
            1000U),
        0.0F,
        kToleranceDeg
    );

    return true;
}

bool test_left_target_returns_positive_steering()
{
    SteeringController controller{};
    const SteeringControllerConfig config =
        make_test_config();

    CHECK(
        steering_controller_init(
            &controller,
            &config)
    );

    /*
     * x = 10.0 m
     * y = +1.0 m
     *
     * Expected Pure-Pursuit-inspired steering:
     * approximately +3.17355 deg.
     */
    const TargetState target{
        1000,
        100
    };

    CHECK(
        steering_controller_update_target(
            &controller,
            &target,
            1000U)
    );

    const float steering =
        steering_controller_step(
            &controller,
            1000U);

    CHECK(steering > 0.0F);

    CHECK_NEAR(
        steering,
        3.17355F,
        kToleranceDeg
    );

    return true;
}

bool test_right_target_returns_negative_steering()
{
    SteeringController controller{};
    const SteeringControllerConfig config =
        make_test_config();

    const TargetState target{
        1000,
        -100
    };

    CHECK(
        steering_controller_init(
            &controller,
            &config)
    );

    CHECK(
        steering_controller_update_target(
            &controller,
            &target,
            1000U)
    );

    const float steering =
        steering_controller_step(
            &controller,
            1000U);

    CHECK(steering < 0.0F);

    CHECK_NEAR(
        steering,
        -3.17355F,
        kToleranceDeg
    );

    return true;
}

bool test_deadband_returns_zero()
{
    SteeringController controller{};
    const SteeringControllerConfig config =
        make_test_config();

    /*
     * x = 10.0 m
     * y = 0.2 m
     *
     * Raw steering is approximately 0.641 deg,
     * which is inside the ±1 deg deadband.
     */
    const TargetState target{
        1000,
        20
    };

    CHECK(
        steering_controller_init(
            &controller,
            &config)
    );

    CHECK(
        steering_controller_update_target(
            &controller,
            &target,
            1000U)
    );

    CHECK_NEAR(
        steering_controller_step(
            &controller,
            1000U),
        0.0F,
        kToleranceDeg
    );

    return true;
}

bool test_positive_saturation()
{
    SteeringController controller{};
    const SteeringControllerConfig config =
        make_test_config();

    const TargetState target{
        100,    // 1.0 m
        100     // +1.0 m
    };

    CHECK(
        steering_controller_init(
            &controller,
            &config)
    );

    CHECK(
        steering_controller_update_target(
            &controller,
            &target,
            1000U)
    );

    CHECK_NEAR(
        steering_controller_step(
            &controller,
            1000U),
        30.0F,
        kToleranceDeg
    );

    return true;
}

bool test_negative_saturation()
{
    SteeringController controller{};
    const SteeringControllerConfig config =
        make_test_config();

    const TargetState target{
        100,
        -100
    };

    CHECK(
        steering_controller_init(
            &controller,
            &config)
    );

    CHECK(
        steering_controller_update_target(
            &controller,
            &target,
            1000U)
    );

    CHECK_NEAR(
        steering_controller_step(
            &controller,
            1000U),
        -30.0F,
        kToleranceDeg
    );

    return true;
}

bool test_invalid_target_is_rejected()
{
    SteeringController controller{};
    const SteeringControllerConfig config =
        make_test_config();

    CHECK(
        steering_controller_init(
            &controller,
            &config)
    );

    const TargetState origin{
        0,
        0
    };

    const TargetState behind{
        -100,
        50
    };

    CHECK(
        !steering_controller_update_target(
            &controller,
            &origin,
            1000U)
    );

    CHECK(
        !steering_controller_update_target(
            &controller,
            &behind,
            1000U)
    );

    CHECK_NEAR(
        steering_controller_step(
            &controller,
            1000U),
        0.0F,
        kToleranceDeg
    );

    return true;
}

bool test_invalid_target_does_not_replace_last_valid_target()
{
    SteeringController controller{};
    const SteeringControllerConfig config =
        make_test_config();

    CHECK(
        steering_controller_init(
            &controller,
            &config)
    );

    const TargetState valid_target{
        1000,
        100
    };

    CHECK(
        steering_controller_update_target(
            &controller,
            &valid_target,
            1000U)
    );

    const float original_steering =
        steering_controller_step(
            &controller,
            1000U);

    CHECK(original_steering > 0.0F);

    const TargetState invalid_target{
        -100,
        -100
    };

    CHECK(
        !steering_controller_update_target(
            &controller,
            &invalid_target,
            1100U)
    );

    /*
     * 1150 ms is still only 150 ms after the last VALID update.
     * The invalid target must not reset the target timestamp.
     */
    const float held_steering =
        steering_controller_step(
            &controller,
            1150U);

    CHECK_NEAR(
        held_steering,
        original_steering,
        kToleranceDeg
    );

    return true;
}

bool test_hold_last_before_timeout()
{
    SteeringController controller{};
    const SteeringControllerConfig config =
        make_test_config();

    CHECK(
        steering_controller_init(
            &controller,
            &config)
    );

    const TargetState target{
        1000,
        100
    };

    CHECK(
        steering_controller_update_target(
            &controller,
            &target,
            1000U)
    );

    const float initial_steering =
        steering_controller_step(
            &controller,
            1000U);

    /*
     * No new target arrives.
     *
     * 1299 - 1000 = 299 ms,
     * still before the 300 ms test timeout.
     */
    const float held_steering =
        steering_controller_step(
            &controller,
            1299U);

    CHECK_NEAR(
        held_steering,
        initial_steering,
        kToleranceDeg
    );

    return true;
}

bool test_timeout_returns_neutral()
{
    SteeringController controller{};
    const SteeringControllerConfig config =
        make_test_config();

    CHECK(
        steering_controller_init(
            &controller,
            &config)
    );

    const TargetState target{
        1000,
        100
    };

    CHECK(
        steering_controller_update_target(
            &controller,
            &target,
            1000U)
    );

    /*
     * Exact timeout boundary:
     *
     * 1300 - 1000 = 300 ms
     *
     * Current requirement:
     * elapsed >= timeout -> neutral.
     */
    CHECK_NEAR(
        steering_controller_step(
            &controller,
            1300U),
        0.0F,
        kToleranceDeg
    );

    return true;
}

bool test_invalid_configuration_is_rejected()
{
    SteeringController controller{};

    SteeringControllerConfig config =
        make_test_config();

    config.wheelbase_m = 0.0F;

    CHECK(
        !steering_controller_init(
            &controller,
            &config)
    );

    config = make_test_config();
    config.deadband_deg = -1.0F;

    CHECK(
        !steering_controller_init(
            &controller,
            &config)
    );

    config = make_test_config();
    config.steering_limit_deg = 0.0F;

    CHECK(
        !steering_controller_init(
            &controller,
            &config)
    );

    config = make_test_config();
    config.deadband_deg = 31.0F;

    CHECK(
        !steering_controller_init(
            &controller,
            &config)
    );

    config = make_test_config();
    config.target_timeout_ms = 0U;

    CHECK(
        !steering_controller_init(
            &controller,
            &config)
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
            "center target",
            test_center_target_returns_zero
        },
        {
            "left target",
            test_left_target_returns_positive_steering
        },
        {
            "right target",
            test_right_target_returns_negative_steering
        },
        {
            "deadband",
            test_deadband_returns_zero
        },
        {
            "positive saturation",
            test_positive_saturation
        },
        {
            "negative saturation",
            test_negative_saturation
        },
        {
            "invalid target",
            test_invalid_target_is_rejected
        },
        {
            "invalid does not replace valid",
            test_invalid_target_does_not_replace_last_valid_target
        },
        {
            "hold last",
            test_hold_last_before_timeout
        },
        {
            "timeout neutral",
            test_timeout_returns_neutral
        },
        {
            "invalid configuration",
            test_invalid_configuration_is_rejected
        }
    };

    for (const TestCase& test : tests)
    {
        if (!test.function())
        {
            std::cerr
                << "Steering controller test failed: "
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
        << "All steering controller tests passed.\n";

    return 0;
}
