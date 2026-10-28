#ifndef LATC_RUNTIME_PROBE_H
#define LATC_RUNTIME_PROBE_H

#include <stdint.h>

/* Runtime-only instrumentation. No clock, OS, or peripheral dependencies.
 * Single writer; halt the processor before reading/exporting the trace.
 * Durations include interrupts/preemption. They are observations, not WCET.
 */
#define RUNTIME_TRACE_CAPACITY 128U
typedef struct {
    uint32_t cycle;
    uint32_t common_us;
    uint32_t work_us;
    int32_t release_error_us;
    int32_t steering_mdeg;
    uint32_t pulse_us;
    int32_t target_x_cm;
    int32_t target_y_cm;
} RuntimeSample;

typedef struct {
    uint32_t samples;
    uint32_t common_min_us;
    uint32_t common_max_us;
    uint64_t common_total_us;
    uint32_t work_max_us;
    uint32_t release_abs_max_us;
    uint32_t deadline_misses;
    uint32_t trace_next;
    RuntimeSample trace[RUNTIME_TRACE_CAPACITY];
} RuntimeProbe;

static inline void runtime_probe_record(
    volatile RuntimeProbe *p, RuntimeSample s, uint32_t period_us)
{
    const uint32_t abs_error = s.release_error_us < 0
        ? (uint32_t)(-(int64_t)s.release_error_us)
        : (uint32_t)s.release_error_us;
    if (p->samples == 0U || s.common_us < p->common_min_us)
        p->common_min_us = s.common_us;
    if (s.common_us > p->common_max_us) p->common_max_us = s.common_us;
    if (s.work_us > p->work_max_us) p->work_max_us = s.work_us;
    if (abs_error > p->release_abs_max_us) p->release_abs_max_us = abs_error;
    if ((int64_t)s.release_error_us + s.work_us > period_us)
        ++p->deadline_misses;
    p->common_total_us += s.common_us;
    s.cycle = p->samples;
    volatile RuntimeSample *slot = &p->trace[p->trace_next];
    slot->cycle = s.cycle;
    slot->common_us = s.common_us;
    slot->work_us = s.work_us;
    slot->release_error_us = s.release_error_us;
    slot->steering_mdeg = s.steering_mdeg;
    slot->pulse_us = s.pulse_us;
    slot->target_x_cm = s.target_x_cm;
    slot->target_y_cm = s.target_y_cm;
    p->trace_next = (p->trace_next + 1U) % RUNTIME_TRACE_CAPACITY;
    ++p->samples;
}
#endif
