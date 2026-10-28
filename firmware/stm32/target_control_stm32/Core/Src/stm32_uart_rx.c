#include "main.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "protocol/uart_rx_stream.h"

extern UART_HandleTypeDef huart1;
static osMessageQueueId_t byte_queue;
volatile UartRxStats uart_rx_stats = {0};

#define RX_ERRORS (USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE)

bool uart_rx_start(void)
{
    if (byte_queue != NULL) return false;
    byte_queue = osMessageQueueNew(UART_RX_QUEUE_CAPACITY, sizeof(UartRxByte), NULL);
    if (byte_queue == NULL) return false;

    /* This module exclusively owns USART1 RX. Do not mix HAL receive/IT/DMA
     * APIs with it. SR then DR clears RXNE and pending receive errors on F4. */
    __HAL_UART_CLEAR_OREFLAG(&huart1);
    HAL_NVIC_SetPriority(USART1_IRQn,
                        configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0U);
    HAL_NVIC_ClearPendingIRQ(USART1_IRQn);
    uart_rx_stats.ready = 1U;
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_PE);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_ERR);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    return true;
}

bool uart_rx_wait(UartRxByte *item)
{
    return osMessageQueueGet(byte_queue, item, NULL, osWaitForever) == osOK;
}

/* Strong definition replaces the startup file's weak default handler.
 * One register read per interrupt, no framing, logging, waits or control work.
 * CMSIS-RTOS2 maps queue Put(timeout=0) in an ISR to FreeRTOS FromISR APIs. */
void USART1_IRQHandler(void)
{
    const uint32_t status = huart1.Instance->SR;
    ++uart_rx_stats.irq_count;
    if ((status & (USART_SR_RXNE | RX_ERRORS)) == 0U) return;
    const uint8_t byte = (uint8_t)huart1.Instance->DR;
    if ((status & USART_SR_RXNE) != 0U) ++uart_rx_stats.bytes_read;
    if ((status & RX_ERRORS) != 0U) {
        uart_rx_stats.last_error = status & RX_ERRORS;
        ++uart_rx_stats.error_events;
        if ((status & USART_SR_ORE) != 0U) ++uart_rx_stats.overrun_events;
        ++uart_rx_stats.epoch;
        return; /* Discard the suspect byte; next byte starts a new epoch. */
    }
    const UartRxByte item = {uart_rx_stats.epoch, byte};
    if (osMessageQueuePut(byte_queue, &item, 0U, 0U) != osOK) {
        ++uart_rx_stats.queue_drops;
        ++uart_rx_stats.epoch;
    }
}
