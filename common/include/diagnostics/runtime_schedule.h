#ifndef LATC_RUNTIME_SCHEDULE_H
#define LATC_RUNTIME_SCHEDULE_H
#include <stdint.h>

/* Engineering budgets, not measured WCET or plant stability requirements. */
#define LATC_CONTROL_PERIOD_MS 20U
#define LATC_CONTROL_BUSY_BUDGET_US 5000U

/* Select a strictly future release instead of running overdue jobs back-to-back.
 * period > 0; period and scheduling lag must be less than half the tick range.
 * Return the number of periods advanced, including the ordinary next period.
 */
static inline uint32_t runtime_next_release(
    uint32_t previous, uint32_t now, uint32_t period, uint32_t *next)
{
    const uint32_t candidate = previous + period;
    const int32_t late = (int32_t)(now - candidate);
    const uint32_t steps = late >= 0 ? (uint32_t)late / period + 2U : 1U;
    *next = previous + steps * period;
    return steps;
}
#endif
