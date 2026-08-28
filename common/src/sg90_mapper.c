#include "actuator/sg90_mapper.h"

#include <math.h>
#include <stddef.h>

static uint32_t interpolate_pulse(
    float ratio,
    uint32_t start_us,
    uint32_t end_us)
{
    const float start = (float)start_us;
    const float end = (float)end_us;

    const float result =
        start + ratio * (end - start);

    return (uint32_t)lroundf(result);
}

bool sg90_mapper_config_is_valid(
    const Sg90MapperConfig *config)
{
    if (config == NULL)
    {
        return false;
    }

    if (!isfinite(config->steering_limit_deg) ||
        (config->steering_limit_deg <= 0.0F))
    {
        return false;
    }

    if (config->period_us == 0U)
    {
        return false;
    }

    if ((config->pulse_at_negative_limit_us == 0U) ||
        (config->pulse_at_center_us == 0U) ||
        (config->pulse_at_positive_limit_us == 0U))
    {
        return false;
    }

    if ((config->pulse_at_negative_limit_us >= config->period_us) ||
        (config->pulse_at_center_us >= config->period_us) ||
        (config->pulse_at_positive_limit_us >= config->period_us))
    {
        return false;
    }

    const bool increasing =
        (config->pulse_at_negative_limit_us <
         config->pulse_at_center_us) &&
        (config->pulse_at_center_us <
         config->pulse_at_positive_limit_us);

    const bool decreasing =
        (config->pulse_at_negative_limit_us >
         config->pulse_at_center_us) &&
        (config->pulse_at_center_us >
         config->pulse_at_positive_limit_us);

    return increasing || decreasing;
}

bool sg90_mapper_map(
    const Sg90MapperConfig *config,
    float steering_deg,
    PwmCommand *out_command)
{
    if (!sg90_mapper_config_is_valid(config) ||
        (out_command == NULL) ||
        !isfinite(steering_deg))
    {
        return false;
    }

    float clamped_deg = steering_deg;

    if (clamped_deg > config->steering_limit_deg)
    {
        clamped_deg = config->steering_limit_deg;
    }
    else if (clamped_deg < -config->steering_limit_deg)
    {
        clamped_deg = -config->steering_limit_deg;
    }

    uint32_t pulse_width_us =
        config->pulse_at_center_us;

    if (clamped_deg < 0.0F)
    {
        const float ratio =
            (-clamped_deg) / config->steering_limit_deg;

        pulse_width_us = interpolate_pulse(
            ratio,
            config->pulse_at_center_us,
            config->pulse_at_negative_limit_us);
    }
    else if (clamped_deg > 0.0F)
    {
        const float ratio =
            clamped_deg / config->steering_limit_deg;

        pulse_width_us = interpolate_pulse(
            ratio,
            config->pulse_at_center_us,
            config->pulse_at_positive_limit_us);
    }

    out_command->period_us = config->period_us;
    out_command->pulse_width_us = pulse_width_us;

    return true;
}
