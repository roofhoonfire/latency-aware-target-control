#include "control/steering_controller.h"

#include <math.h>
#include <stddef.h>

#define LATC_CM_TO_M          0.01f
#define LATC_RAD_TO_DEG       57.29577951308232f

static bool config_is_valid(
    const SteeringControllerConfig* config)
{
    if (config == NULL)
    {
        return false;
    }

    if (!isfinite(config->wheelbase_m) ||
        !isfinite(config->deadband_deg) ||
        !isfinite(config->steering_limit_deg))
    {
        return false;
    }

    if (config->wheelbase_m <= 0.0f)
    {
        return false;
    }

    if (config->deadband_deg < 0.0f)
    {
        return false;
    }

    if (config->steering_limit_deg <= 0.0f)
    {
        return false;
    }

    if (config->deadband_deg >
        config->steering_limit_deg)
    {
        return false;
    }

    if (config->target_timeout_ms == 0U)
    {
        return false;
    }

    return true;
}

static bool target_is_semantically_valid(
    const TargetState* target)
{
    if (target == NULL)
    {
        return false;
    }

    /*
     * Current V1 operating requirement:
     *
     * The target must be in front of the ego vehicle.
     *
     * Additional operating-region bounds may be introduced
     * later when their exact requirements are defined.
     */
    if (target->x_cm <= 0)
    {
        return false;
    }

    return true;
}

static float calculate_steering_deg(
    const SteeringController* controller)
{
    const float x_m =
        (float)controller->last_target.x_cm *
        LATC_CM_TO_M;

    const float y_m =
        (float)controller->last_target.y_cm *
        LATC_CM_TO_M;

    const float distance_squared =
        (x_m * x_m) +
        (y_m * y_m);

    /*
     * Pure-Pursuit-inspired target-relative steering:
     *
     * delta =
     * atan(2 * L * y / (x^2 + y^2))
     */
    const float steering_rad =
        atanf(
            (2.0f *
             controller->config.wheelbase_m *
             y_m) /
            distance_squared);

    float steering_deg =
        steering_rad * LATC_RAD_TO_DEG;

    /*
     * Steering deadband.
     */
    if (fabsf(steering_deg) <=
        controller->config.deadband_deg)
    {
        steering_deg = 0.0f;
    }

    /*
     * Steering saturation.
     */
    if (steering_deg >
        controller->config.steering_limit_deg)
    {
        steering_deg =
            controller->config.steering_limit_deg;
    }
    else if (steering_deg <
             -controller->config.steering_limit_deg)
    {
        steering_deg =
            -controller->config.steering_limit_deg;
    }

    return steering_deg;
}

bool steering_controller_init(
    SteeringController* controller,
    const SteeringControllerConfig* config)
{
    if ((controller == NULL) ||
        !config_is_valid(config))
    {
        return false;
    }

    controller->config = *config;

    controller->last_target.x_cm = 0;
    controller->last_target.y_cm = 0;

    controller->last_target_update_ms = 0U;
    controller->has_valid_target = false;

    return true;
}

bool steering_controller_update_target(
    SteeringController* controller,
    const TargetState* target,
    uint32_t now_ms)
{
    if ((controller == NULL) ||
        !target_is_semantically_valid(target))
    {
        return false;
    }

    controller->last_target = *target;
    controller->last_target_update_ms = now_ms;
    controller->has_valid_target = true;

    return true;
}

float steering_controller_step(
    const SteeringController* controller,
    uint32_t now_ms)
{
    if ((controller == NULL) ||
        !controller->has_valid_target)
    {
        return 0.0f;
    }

    const uint32_t elapsed_ms =
        now_ms - controller->last_target_update_ms;

    /*
     * No new valid target has arrived within the timeout.
     * Return neutral steering.
     */
    if (elapsed_ms >=
        controller->config.target_timeout_ms)
    {
        return 0.0f;
    }

    return calculate_steering_deg(controller);
}
