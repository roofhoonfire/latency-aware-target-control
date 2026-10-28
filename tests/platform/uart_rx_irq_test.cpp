#include "fake_platform.h"
#include "protocol/uart_rx_stream.h"
#include <cassert>
#include <cstring>
#include <deque>
#include <iostream>

static std::deque<UartRxByte> queued;
static std::deque<uint8_t> fifo;
static uint32_t capacity, status_reg, control_reg, enabled_mask, priority, irq_id;
static uint32_t wait_timeout, fifo_threshold, recv_timeout;
static uint32_t timeout_restarts;
static bool irq_enabled, fail_alloc;
static XInterruptHandler irq_handler;
static void *irq_reference;
static USART_TypeDef uart_regs{};
UART_HandleTypeDef huart1{&uart_regs};
XScuGic xInterruptController{};

extern "C" {
void fake_yield(BaseType_t) {}
osMessageQueueId_t osMessageQueueNew(uint32_t cap, uint32_t size, const void *) {
    assert(size == sizeof(UartRxByte)); capacity = cap;
    return fail_alloc ? nullptr : &queued;
}
osStatus_t osMessageQueuePut(osMessageQueueId_t q, const void *item, uint8_t, uint32_t timeout) {
    assert(q == &queued && timeout == 0);
    if (queued.size() >= capacity) return -1;
    queued.push_back(*static_cast<const UartRxByte *>(item)); return osOK;
}
osStatus_t osMessageQueueGet(osMessageQueueId_t q, void *item, uint8_t *, uint32_t timeout) {
    assert(q == &queued); wait_timeout = timeout;
    if (queued.empty()) return -1; // Models only API contract, not a scheduler.
    *static_cast<UartRxByte *>(item) = queued.front(); queued.pop_front(); return osOK;
}
QueueHandle_t xQueueCreate(UBaseType_t cap, UBaseType_t size) {
    return osMessageQueueNew(cap, size, nullptr);
}
BaseType_t xQueueSendFromISR(QueueHandle_t q, const void *item, BaseType_t *woken) {
    *woken = 1; return osMessageQueuePut(q, item, 0, 0) == osOK ? pdPASS : 0;
}
BaseType_t xQueueReceive(QueueHandle_t q, void *item, uint32_t timeout) {
    return osMessageQueueGet(q, item, nullptr, timeout) == osOK ? pdPASS : 0;
}
void fake_clear_ore(UART_HandleTypeDef *h) { h->Instance->SR = 0; }
void fake_enable_uart_it(UART_HandleTypeDef *, uint32_t it) { enabled_mask |= it; }
void HAL_NVIC_SetPriority(uint32_t id, uint32_t p, uint32_t sub) {
    irq_id = id; priority = p; assert(sub == 0);
}
void HAL_NVIC_ClearPendingIRQ(uint32_t) {}
void HAL_NVIC_EnableIRQ(uint32_t) { irq_enabled = true; }
XUartPs_Config *XUartPs_LookupConfig(uint16_t) {
    static XUartPs_Config config{XPS_UART1_BASEADDR}; return &config;
}
int XUartPs_CfgInitialize(XUartPs *p, XUartPs_Config *c, UINTPTR) {
    p->Config = *c; return XST_SUCCESS;
}
int XUartPs_SetBaudRate(XUartPs *, u32 baud) { assert(baud == 115200); return 0; }
void XUartPs_SetInterruptMask(XUartPs *, u32 mask) { enabled_mask = mask; }
void XUartPs_SetOperMode(XUartPs *, u8) {}
void XUartPs_SetFifoThreshold(XUartPs *, u8 n) { fifo_threshold = n; }
void XUartPs_SetRecvTimeout(XUartPs *, u8 n) { recv_timeout = n; }
u32 XUartPs_ReadReg(UINTPTR base, u32 offset) {
    assert(base == XPS_UART1_BASEADDR);
    if (offset == XUARTPS_ISR_OFFSET) return status_reg;
    if (offset == XUARTPS_CR_OFFSET) return control_reg;
    if (offset == XUARTPS_SR_OFFSET) return fifo.empty() ? XUARTPS_SR_RXEMPTY : 0;
    assert(offset == XUARTPS_FIFO_OFFSET && !fifo.empty());
    const auto byte = fifo.front(); fifo.pop_front(); return byte;
}
void XUartPs_WriteReg(UINTPTR, u32 offset, u32 value) {
    if (offset == XUARTPS_ISR_OFFSET) { status_reg &= ~value; return; }
    assert(offset == XUARTPS_CR_OFFSET);
    if (value & XUARTPS_CR_TORST) ++timeout_restarts;
    if (value & XUARTPS_CR_RXRST) fifo.clear();
    control_reg = value & ~XUARTPS_CR_RXRST;
}
BaseType_t xPortInstallInterruptHandler(uint8_t id, XInterruptHandler fn, void *ref) {
    irq_id = id; irq_handler = fn; irq_reference = ref; return pdPASS;
}
void vPortEnableInterrupt(uint8_t id) { assert(id == irq_id); irq_enabled = true; }
void XScuGic_SetPriorityTriggerType(XScuGic *, u32 id, u8 p, u8 trigger) {
    assert(id == irq_id && trigger == 1); priority = p;
}
}

static void inject(uint8_t byte, bool error = false)
{
#ifdef TEST_STM32
    uart_regs.SR = USART_SR_RXNE | (error ? USART_SR_ORE : 0);
    uart_regs.DR = byte;
    USART1_IRQHandler();
#else
    fifo.push_back(byte);
    status_reg = XUARTPS_IXR_RXOVR | (error ? XUARTPS_IXR_OVER : 0);
    irq_handler(irq_reference);
#endif
}

int main(int argc, char **argv)
{
    if (argc > 1 && std::strcmp(argv[1], "allocation-failure") == 0) {
        fail_alloc = true;
        assert(!uart_rx_start() && uart_rx_stats.ready == 0 && !irq_enabled);
        return 0;
    }
    assert(uart_rx_start() && uart_rx_stats.ready == 1 && irq_enabled);
    assert(!uart_rx_start()); // Initialization is deliberately once-only.
#ifdef TEST_STM32
    assert(irq_id == USART1_IRQn && priority == 5);
    assert(enabled_mask == (UART_IT_PE | UART_IT_ERR | UART_IT_RXNE));
#else
    assert(irq_id == 82 && priority >= (18U << 3));
    assert(fifo_threshold == 1 && recv_timeout > 0);
#endif
    UartRxByte item{};
    assert(!uart_rx_wait(&item) && wait_timeout == UINT32_MAX);
    inject(0xAA); inject(0x55);
    assert(uart_rx_wait(&item) && item.value == 0xAA && item.epoch == 0);
    assert(uart_rx_wait(&item) && item.value == 0x55 && item.epoch == 0);
    for (unsigned i = 0; i < UART_RX_QUEUE_CAPACITY; ++i) inject(0x11);
    inject(0x22); inject(0x33);
    assert(uart_rx_stats.queue_drops == 2 && uart_rx_stats.epoch == 2);
    for (unsigned i = 0; i < UART_RX_QUEUE_CAPACITY; ++i) {
        assert(uart_rx_wait(&item) && item.value == 0x11 && item.epoch == 0);
    }
    inject(0x44);
    assert(uart_rx_wait(&item) && item.value == 0x44 && item.epoch == 2);
    inject(0xEE, true);
    assert(queued.empty() && uart_rx_stats.error_events == 1 &&
           uart_rx_stats.overrun_events == 1 && uart_rx_stats.epoch == 3);
    inject(0x66);
    assert(uart_rx_wait(&item) && item.value == 0x66 && item.epoch == 3);
#ifndef TEST_STM32
    // An input stream longer than one ISR budget cannot monopolize the CPU
    // within a single handler invocation. A tail timeout delivers the rest.
    for (unsigned i = 0; i < 80; ++i) fifo.push_back(static_cast<uint8_t>(i));
    status_reg = XUARTPS_IXR_RXOVR; irq_handler(irq_reference);
    assert(fifo.size() == 16 && queued.size() == 64);
    status_reg = XUARTPS_IXR_TOUT; irq_handler(irq_reference);
    assert(fifo.empty() && queued.size() == 80);
    for (unsigned i = 0; i < 80; ++i) {
        assert(uart_rx_wait(&item) && item.value == i && item.epoch == 3);
    }
    assert(uart_rx_stats.fifo_resets == 1);
    const auto restarts = timeout_restarts;
    status_reg = XUARTPS_IXR_TOUT; irq_handler(irq_reference);
    assert(queued.empty() && timeout_restarts == restarts);
#endif
    std::cout << "IRQ configuration, blocking wait, byte ordering, overflow and recovery: PASS\n";
}
