#ifndef LATC_CONTROL_STEERING_CONTROLLER_H
#define LATC_CONTROL_STEERING_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

#include "control/target_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float wheelbase_m;
    float deadband_deg;
    float steering_limit_deg;
    uint32_t target_timeout_ms;
} SteeringControllerConfig;

typedef struct
{
    SteeringControllerConfig config;

    TargetState last_target;
    uint32_t last_target_update_ms;

    bool has_valid_target;
} SteeringController;

/*
 * Initializes the controller.
 *
 * The timeout value is provided by the runtime/application configuration.
 * No platform-specific time API is used here.
 */
bool steering_controller_init(
    SteeringController* controller,
    const SteeringControllerConfig* config
);

/*
 * Supplies a newly received semantic target to the controller.
 *
 * Invalid targets are rejected and do not replace the last valid target.
 */
bool steering_controller_update_target(
    SteeringController* controller,
    const TargetState* target,
    uint32_t now_ms
);

/*
 * Executes one control step.
 *
 * Returns desired steering angle [deg].
 *
 * positive : left
 * negative : right
 * zero     : straight
 */
float steering_controller_step(
    const SteeringController* controller,
    uint32_t now_ms
);

#ifdef __cplusplus
}
#endif

#endif /* LATC_CONTROL_STEERING_CONTROLLER_H */
