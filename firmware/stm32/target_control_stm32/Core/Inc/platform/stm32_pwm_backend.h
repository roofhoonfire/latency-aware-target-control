#ifndef LATC_PLATFORM_STM32_PWM_BACKEND_H
#define LATC_PLATFORM_STM32_PWM_BACKEND_H

#include <stdbool.h>
#include <stdint.h>

#include "actuator/pwm_command.h"
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    TIM_HandleTypeDef *timer;
    uint32_t channel;

    /*
     * Timer counter frequency AFTER prescaler.
     *
     * Current TIM4 configuration:
     * 6.25 MHz / (PSC + 1)
     * = 6.25 MHz / 5
     * = 1.25 MHz.
     */
    uint32_t counter_frequency_hz;

    /*
     * Maximum number of counter states supported by
     * the selected timer configuration.
     *
     * TIM4 is a 16-bit timer -> 65536 counts.
     */
    uint32_t max_period_counts;
} Stm32PwmBackend;

bool stm32_pwm_backend_init(
    Stm32PwmBackend *backend,
    TIM_HandleTypeDef *timer,
    uint32_t channel,
    uint32_t counter_frequency_hz,
    uint32_t max_period_counts);

bool stm32_pwm_backend_start(
    Stm32PwmBackend *backend);

bool stm32_pwm_backend_apply(
    Stm32PwmBackend *backend,
    const PwmCommand *command);

#ifdef __cplusplus
}
#endif

#endif
