#include "platform/stm32_pwm_backend.h"

#include <stddef.h>

#define MICROSECONDS_PER_SECOND 1000000ULL

static bool microseconds_to_counts(
    uint32_t microseconds,
    uint32_t counter_frequency_hz,
    uint32_t *out_counts)
{
    if ((microseconds == 0U) ||
        (counter_frequency_hz == 0U) ||
        (out_counts == NULL))
    {
        return false;
    }

    const uint64_t scaled =
        (uint64_t)microseconds *
        (uint64_t)counter_frequency_hz;

    /*
     * Round to nearest timer count.
     */
    const uint64_t counts =
        (scaled + (MICROSECONDS_PER_SECOND / 2ULL)) /
        MICROSECONDS_PER_SECOND;

    if ((counts == 0ULL) ||
        (counts > UINT32_MAX))
    {
        return false;
    }

    *out_counts = (uint32_t)counts;

    return true;
}

bool stm32_pwm_backend_init(
    Stm32PwmBackend *backend,
    TIM_HandleTypeDef *timer,
    uint32_t channel,
    uint32_t counter_frequency_hz,
    uint32_t max_period_counts)
{
    if ((backend == NULL) ||
        (timer == NULL) ||
        (counter_frequency_hz == 0U) ||
        (max_period_counts == 0U))
    {
        return false;
    }

    backend->timer = timer;
    backend->channel = channel;
    backend->counter_frequency_hz =
        counter_frequency_hz;
    backend->max_period_counts =
        max_period_counts;

    return true;
}

bool stm32_pwm_backend_start(
    Stm32PwmBackend *backend)
{
    if ((backend == NULL) ||
        (backend->timer == NULL))
    {
        return false;
    }

    return HAL_TIM_PWM_Start(
        backend->timer,
        backend->channel) == HAL_OK;
}

bool stm32_pwm_backend_apply(
    Stm32PwmBackend *backend,
    const PwmCommand *command)
{
    if ((backend == NULL) ||
        (backend->timer == NULL) ||
        (command == NULL))
    {
        return false;
    }

    if ((command->period_us == 0U) ||
        (command->pulse_width_us > command->period_us))
    {
        return false;
    }

    uint32_t period_counts = 0U;
    uint32_t pulse_counts = 0U;

    if (!microseconds_to_counts(
            command->period_us,
            backend->counter_frequency_hz,
            &period_counts))
    {
        return false;
    }

    if (!microseconds_to_counts(
            command->pulse_width_us,
            backend->counter_frequency_hz,
            &pulse_counts))
    {
        return false;
    }

    if ((period_counts == 0U) ||
        (period_counts > backend->max_period_counts) ||
        (pulse_counts > period_counts))
    {
        return false;
    }

    /*
     * Up-counting PWM:
     *
     * period_counts = ARR + 1
     * therefore ARR = period_counts - 1.
     */
    __HAL_TIM_SET_AUTORELOAD(
        backend->timer,
        period_counts - 1U);

    __HAL_TIM_SET_COMPARE(
        backend->timer,
        backend->channel,
        pulse_counts);

    return true;
}
