#ifndef LATC_ACTUATOR_PWM_COMMAND_H
#define LATC_ACTUATOR_PWM_COMMAND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t period_us;
    uint32_t pulse_width_us;
} PwmCommand;

#ifdef __cplusplus
}
#endif

#endif
