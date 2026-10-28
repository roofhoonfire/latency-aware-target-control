#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "xil_printf.h"
#include "xtime_l.h"
#include "control/steering_controller.h"
#include "actuator/sg90_mapper.h"
#include "protocol/uart_rx_stream.h"
#include "diagnostics/runtime_probe.h"

#define CONTROL_PERIOD_MS 20U
#define TARGET_QUEUE_CAPACITY 4U
static QueueHandle_t target_queue;
volatile RuntimeProbe runtime_probe = {0};
volatile uint32_t received_frames = 0U;
volatile uint32_t dropped_targets = 0U;
volatile uint32_t rejected_windows = 0U;
volatile uint32_t accepted_targets = 0U;
volatile uint32_t invalid_targets = 0U;
volatile uint32_t map_failures = 0U;
volatile uint32_t last_sequence = 0U;
volatile uint32_t uart_rx_gap_resets = 0U;
volatile uint32_t probe_clock_hz = COUNTS_PER_SECOND;
volatile uint32_t probe_period_us = CONTROL_PERIOD_MS * 1000U;
volatile uint32_t probe_tick_hz = configTICK_RATE_HZ;

static uint32_t platform_now_ms(void)
{
    return (uint32_t)(((uint64_t)xTaskGetTickCount() * 1000U) /
                      configTICK_RATE_HZ);
}
static XTime counter_now(void)
{
    XTime now;
    XTime_GetTime(&now);
    return now;
}
static uint32_t elapsed_us(XTime counts)
{
    return (uint32_t)((counts * 1000000ULL) / COUNTS_PER_SECOND);
}

static void receive_task(void *argument)
{
    (void)argument;
    UartRxStream stream = {0};
    TargetCommand command;
    if (!uart_rx_start()) {
        vTaskDelete(NULL);
        return;
    }
    for (;;) {
        UartRxByte item;
        if (uart_rx_wait(&item)) {
            if (uart_rx_stream_push(&stream, &item, &command)) {
                const TargetState target = {command.target_x, command.target_y};
                ++received_frames;
                last_sequence = command.sequence;
                if (xQueueSend(target_queue, &target, 0U) != pdPASS)
                    ++dropped_targets;
            }
        }
        rejected_windows = stream.frame.rejected_windows;
        uart_rx_gap_resets = stream.gap_resets;
    }
}

static void control_task(void *argument)
{
    (void)argument;
    const SteeringControllerConfig config = {2.8f, 1.0f, 30.0f, 400U};
    const Sg90MapperConfig mapper = {30.0f, 20000U, 1200U, 1500U, 1800U};
    SteeringController controller;
    if (!steering_controller_init(&controller, &config)) {
        vTaskDelete(NULL);
        return;
    }
    const TickType_t period_ticks = pdMS_TO_TICKS(CONTROL_PERIOD_MS);
    configASSERT(period_ticks > 0U);
    /* Align the high-resolution release reference to an RTOS tick edge. */
    TickType_t wake = xTaskGetTickCount();
    vTaskDelayUntil(&wake, 1U);
    XTime expected = counter_now();
    const XTime period_counts =
        ((XTime)period_ticks * COUNTS_PER_SECOND) / configTICK_RATE_HZ;
    for (;;) {
        const XTime started = counter_now();
        TargetState target;
        for (unsigned int i = 0U; i < TARGET_QUEUE_CAPACITY; ++i) {
            if (xQueueReceive(target_queue, &target, 0U) != pdPASS) break;
            if (steering_controller_update_target(&controller, &target,
                                                   platform_now_ms()))
                ++accepted_targets;
            else ++invalid_targets;
        }
        const uint32_t now_ms = platform_now_ms();
        PwmCommand pwm = {0};
        const XTime common_started = counter_now();
        const float angle = steering_controller_step(&controller, now_ms);
        const bool mapped = sg90_mapper_map(&mapper, angle, &pwm);
        const XTime common_finished = counter_now();
        if (!mapped) ++map_failures;
        /* No physical PWM backend on Zynq: compare common_us across boards.
         * work_us covers this platform's queue/control work only. */
        const XTime finished = counter_now();
        const int64_t error_counts = (int64_t)(started - expected);
        RuntimeSample sample = {0};
        sample.common_us = elapsed_us(common_finished - common_started);
        sample.work_us = elapsed_us(finished - started);
        sample.release_error_us = (int32_t)(
            error_counts * 1000000LL / (int64_t)COUNTS_PER_SECOND);
        sample.steering_mdeg = (int32_t)(angle * 1000.0f);
        sample.pulse_us = pwm.pulse_width_us;
        sample.target_x_cm = controller.last_target.x_cm;
        sample.target_y_cm = controller.last_target.y_cm;
        runtime_probe_record(&runtime_probe, sample, CONTROL_PERIOD_MS * 1000U);
        expected += period_counts;
        vTaskDelayUntil(&wake, period_ticks);
    }
}

int main(void)
{
    target_queue = xQueueCreate(TARGET_QUEUE_CAPACITY, sizeof(TargetState));
    if (target_queue == NULL) return 1;
    xil_printf("Zynq IRQ RX -> receive task -> target queue -> control\r\n");
    /* Equal application priorities, matching the STM32 reference tasks. */
    if (xTaskCreate(receive_task, "CommRx", 1024U, NULL,
                    tskIDLE_PRIORITY + 1U, NULL) != pdPASS ||
        xTaskCreate(control_task, "Control", 1024U, NULL,
                    tskIDLE_PRIORITY + 1U, NULL) != pdPASS)
        return 1;
    vTaskStartScheduler();
    return 1;
}
