#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "xuartps.h"
#include "xscugic.h"
#include "xparameters.h"
#include "xparameters_ps.h"
#include "protocol/uart_rx_stream.h"

/* BSP instance 0 maps to physical UART1 in this project. Do not use the
 * misleading XPAR_XUARTPS_0_INTR alias (which names physical UART0). */
#if XPAR_XUARTPS_0_BASEADDR == XPS_UART1_BASEADDR
#define INPUT_UART_IRQ XPS_UART1_INT_ID
#elif XPAR_XUARTPS_0_BASEADDR == XPS_UART0_BASEADDR
#define INPUT_UART_IRQ XPS_UART0_INT_ID
#else
#error Map the selected PS UART base address to its physical interrupt ID
#endif

#define RX_ERRORS (XUARTPS_IXR_OVER | XUARTPS_IXR_FRAMING | XUARTPS_IXR_PARITY)
#define RX_INTERRUPTS (RX_ERRORS | XUARTPS_IXR_RXOVR | XUARTPS_IXR_TOUT)
#define RX_ISR_BYTE_BUDGET 64U

extern XScuGic xInterruptController; /* Owned by the FreeRTOS BSP. */
static XUartPs input_uart;
static QueueHandle_t byte_queue;
volatile UartRxStats uart_rx_stats = {0};

static void reset_rx_fifo(void)
{
    const UINTPTR base = input_uart.Config.BaseAddress;
    /* Preserve TX configuration. Reset only RX, then re-enable/restart timeout.
     * Like the BSP's baud setup, these are command writes, not a busy wait. */
    const u32 control = XUartPs_ReadReg(base, XUARTPS_CR_OFFSET) &
                        ~(XUARTPS_CR_RX_DIS | XUARTPS_CR_RXRST);
    XUartPs_WriteReg(base, XUARTPS_CR_OFFSET, control | XUARTPS_CR_RXRST);
    XUartPs_WriteReg(base, XUARTPS_CR_OFFSET,
                    control | XUARTPS_CR_RX_EN | XUARTPS_CR_TORST);
}

static void receive_error(u32 errors)
{
    uart_rx_stats.last_error = errors;
    ++uart_rx_stats.error_events;
    if ((errors & XUARTPS_IXR_OVER) != 0U) ++uart_rx_stats.overrun_events;
    ++uart_rx_stats.epoch;
    ++uart_rx_stats.fifo_resets;
    reset_rx_fifo(); /* Suspect FIFO contents are deliberately discarded. */
}

static void uart_rx_irq(void *reference)
{
    (void)reference;
    const UINTPTR base = input_uart.Config.BaseAddress;
    BaseType_t higher_priority_woken = pdFALSE;
    unsigned int read_count = 0U;
    u32 status = XUartPs_ReadReg(base, XUARTPS_ISR_OFFSET) & RX_INTERRUPTS;
    ++uart_rx_stats.irq_count;
    /* Acknowledge the snapshot before draining; new events remain pending. */
    XUartPs_WriteReg(base, XUARTPS_ISR_OFFSET, status);
    if ((status & RX_ERRORS) != 0U) {
        receive_error(status & RX_ERRORS);
        return;
    }
    for (unsigned int i = 0U; i < RX_ISR_BYTE_BUDGET; ++i) {
        /* Also catch an overrun/error occurring while the FIFO is drained. */
        status = XUartPs_ReadReg(base, XUARTPS_ISR_OFFSET) & RX_ERRORS;
        if (status != 0U) {
            XUartPs_WriteReg(base, XUARTPS_ISR_OFFSET, status);
            receive_error(status);
            break;
        }
        if ((XUartPs_ReadReg(base, XUARTPS_SR_OFFSET) & XUARTPS_SR_RXEMPTY) != 0U)
            break;
        const UartRxByte item = {uart_rx_stats.epoch,
            (uint8_t)XUartPs_ReadReg(base, XUARTPS_FIFO_OFFSET)};
        ++uart_rx_stats.bytes_read;
        ++read_count;
        if (xQueueSendFromISR(byte_queue, &item, &higher_priority_woken) != pdPASS) {
            ++uart_rx_stats.queue_drops;
            ++uart_rx_stats.epoch;
        }
    }
    /* Arm a tail timeout after RX activity. Do NOT restart an empty timeout:
     * RSTTO also runs on an idle line and would cause an endless IRQ train. */
    if (read_count != 0U) {
        XUartPs_WriteReg(base, XUARTPS_CR_OFFSET,
            XUartPs_ReadReg(base, XUARTPS_CR_OFFSET) | XUARTPS_CR_TORST);
    }
    portYIELD_FROM_ISR(higher_priority_woken);
}

bool uart_rx_start(void)
{
    if (byte_queue != NULL) return false;
    XUartPs_Config *config = XUartPs_LookupConfig(XPAR_XUARTPS_0_DEVICE_ID);
    if (config == NULL || XUartPs_CfgInitialize(&input_uart, config,
            config->BaseAddress) != XST_SUCCESS ||
        XUartPs_SetBaudRate(&input_uart, 115200U) != XST_SUCCESS) return false;
    XUartPs_SetInterruptMask(&input_uart, 0U);
    XUartPs_SetOperMode(&input_uart, XUARTPS_OPER_MODE_NORMAL);
    byte_queue = xQueueCreate(UART_RX_QUEUE_CAPACITY, sizeof(UartRxByte));
    if (byte_queue == NULL) return false;

    /* Called from the receive task after the scheduler/tick GIC setup. Reuse
     * the port's vector table; never install a standalone exception dispatcher. */
    if (xPortInstallInterruptHandler(INPUT_UART_IRQ, uart_rx_irq, NULL) != pdPASS)
        return false;
    XScuGic_SetPriorityTriggerType(&xInterruptController, INPUT_UART_IRQ,
        (configMAX_API_CALL_INTERRUPT_PRIORITY + 1U) << portPRIORITY_SHIFT, 1U);
    XUartPs_SetFifoThreshold(&input_uart, 1U);
    XUartPs_SetRecvTimeout(&input_uart, 4U);
    reset_rx_fifo();
    XUartPs_WriteReg(config->BaseAddress, XUARTPS_ISR_OFFSET, XUARTPS_IXR_MASK);
    uart_rx_stats.ready = 1U;
    vPortEnableInterrupt(INPUT_UART_IRQ);
    XUartPs_SetInterruptMask(&input_uart, RX_INTERRUPTS);
    return true;
}

bool uart_rx_wait(UartRxByte *item)
{
    return xQueueReceive(byte_queue, item, portMAX_DELAY) == pdPASS;
}
