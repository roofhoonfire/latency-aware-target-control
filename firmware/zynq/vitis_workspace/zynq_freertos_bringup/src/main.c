#include "FreeRTOS.h"
#include "task.h"

#include "xil_printf.h"

#include "control/steering_controller.h"
#include "control/target_state.h"
#include "actuator/sg90_mapper.h"

#include <stdint.h>

#define PORTABILITY_TASK_STACK_SIZE  1024U
#define PORTABILITY_TASK_PRIORITY    (tskIDLE_PRIORITY + 1U)

static uint32_t platform_now_ms(void)
{
    return (uint32_t)(
        xTaskGetTickCount() * portTICK_PERIOD_MS
    );
}
static void portability_proof_task(void *pvParameters)
{
    (void)pvParameters;

    const SteeringControllerConfig controller_config =
    {
        .wheelbase_m = 2.8f,
        .deadband_deg = 1.0f,
        .steering_limit_deg = 30.0f,
        .target_timeout_ms = 400U
    };

    /*
     * Bring-up / unit-test fixture only.
     * These pulse values are NOT final physical SG90 calibration values.
     */
    const Sg90MapperConfig mapper_config =
    {
        .steering_limit_deg = 30.0f,

        .period_us = 20000U,

        .pulse_at_negative_limit_us = 1200U,
        .pulse_at_center_us = 1500U,
        .pulse_at_positive_limit_us = 1800U
    };

    const TargetState target =
    {
        .x_cm = 515,
        .y_cm = -97
    };

    SteeringController controller;
    PwmCommand pwm_command;

    xil_printf(
        "=== Zynq FreeRTOS Common Pipeline Proof ===\r\n"
    );

    if (!steering_controller_init(
            &controller,
            &controller_config))
    {
        xil_printf("controller init: FAILED\r\n");
        vTaskDelete(NULL);
        return;
    }

    const uint32_t now_ms = platform_now_ms();

    if (!steering_controller_update_target(
            &controller,
            &target,
            now_ms))
    {
        xil_printf("target update: FAILED\r\n");
        vTaskDelete(NULL);
        return;
    }

    const float steering_deg =
        steering_controller_step(
            &controller,
            now_ms);

    /*
     * xil_printf does not reliably support %f,
     * so print milli-degrees as an integer.
     */
    const int steering_mdeg =
        (int)(steering_deg * 1000.0f);

    xil_printf(
        "TargetState: x_cm=%d, y_cm=%d\r\n",
        (int)target.x_cm,
        (int)target.y_cm
    );

    xil_printf(
        "desired_steering = %d mdeg\r\n",
        steering_mdeg
    );

    if ((steering_mdeg > -11250) &&
        (steering_mdeg < -11100))
    {
        xil_printf(
            "CONTROLLER PROOF: PASS\r\n"
        );
    }
    else
    {
        xil_printf(
            "CONTROLLER PROOF: FAIL\r\n"
        );

        vTaskDelete(NULL);
        return;
    }

    if (!sg90_mapper_map(
            &mapper_config,
            steering_deg,
            &pwm_command))
    {
        xil_printf(
            "SG90 mapper: FAILED\r\n"
        );

        vTaskDelete(NULL);
        return;
    }

    xil_printf(
        "PwmCommand: period_us=%d, pulse_width_us=%d\r\n",
        (int)pwm_command.period_us,
        (int)pwm_command.pulse_width_us
    );

    if ((pwm_command.period_us == 20000U) &&
        (pwm_command.pulse_width_us == 1388U))
    {
        xil_printf(
            "SG90 MAPPER PROOF: PASS\r\n"
        );

        xil_printf(
            "COMMON PIPELINE PORTABILITY: PASS\r\n"
        );
    }
    else
    {
        xil_printf(
            "SG90 MAPPER PROOF: FAIL\r\n"
        );

        xil_printf(
            "COMMON PIPELINE PORTABILITY: FAIL\r\n"
        );
    }

    vTaskDelete(NULL);
}

int main(void)
{
    xil_printf(
        "Zynq FreeRTOS starting...\r\n"
    );

    const BaseType_t result =
        xTaskCreate(
            portability_proof_task,
            "ControlProof",
            PORTABILITY_TASK_STACK_SIZE,
            NULL,
            PORTABILITY_TASK_PRIORITY,
            NULL
        );

    if (result != pdPASS)
    {
        xil_printf(
            "Task creation: FAILED\r\n"
        );

        for (;;)
        {
        }
    }

    vTaskStartScheduler();

    /*
     * Should never reach here.
     */
    xil_printf(
        "Scheduler start: FAILED\r\n"
    );

    for (;;)
    {
    }
}
