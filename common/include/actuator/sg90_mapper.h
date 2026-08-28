#ifndef LATC_ACTUATOR_SG90_MAPPER_H
#define LATC_ACTUATOR_SG90_MAPPER_H

#include <stdbool.h>
#include <stdint.h>

#include "actuator/pwm_command.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float steering_limit_deg;

    uint32_t period_us;

    uint32_t pulse_at_negative_limit_us;
    uint32_t pulse_at_center_us;
    uint32_t pulse_at_positive_limit_us;
} Sg90MapperConfig;

bool sg90_mapper_config_is_valid(
    const Sg90MapperConfig *config);

bool sg90_mapper_map(
    const Sg90MapperConfig *config,
    float steering_deg,
    PwmCommand *out_command);

#ifdef __cplusplus
}
#endif

#endif
