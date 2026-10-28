#ifndef LATC_TEST_FAKE_PLATFORM_H
#define LATC_TEST_FAKE_PLATFORM_H
/* Host behavioural doubles, not replacement BSP headers. Actual firmware
 * translation units are compiled against these only by the platform tests. */
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef void *QueueHandle_t;
typedef void *osMessageQueueId_t;
typedef int osStatus_t;
#define pdFALSE 0
#define pdPASS 1
#define osOK 0
#define osWaitForever UINT32_MAX
#define portMAX_DELAY UINT32_MAX
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5U
#define configMAX_API_CALL_INTERRUPT_PRIORITY 18U
#define portPRIORITY_SHIFT 3U
void fake_yield(BaseType_t value);
#define portYIELD_FROM_ISR(value) fake_yield(value)
osMessageQueueId_t osMessageQueueNew(uint32_t, uint32_t, const void *);
osStatus_t osMessageQueuePut(osMessageQueueId_t, const void *, uint8_t, uint32_t);
osStatus_t osMessageQueueGet(osMessageQueueId_t, void *, uint8_t *, uint32_t);
QueueHandle_t xQueueCreate(UBaseType_t, UBaseType_t);
BaseType_t xQueueSendFromISR(QueueHandle_t, const void *, BaseType_t *);
BaseType_t xQueueReceive(QueueHandle_t, void *, uint32_t);

typedef struct { volatile uint32_t SR, DR; } USART_TypeDef;
typedef struct { USART_TypeDef *Instance; } UART_HandleTypeDef;
extern UART_HandleTypeDef huart1;
#define USART1_IRQn 37U
#define USART_SR_PE 1U
#define USART_SR_FE 2U
#define USART_SR_NE 4U
#define USART_SR_ORE 8U
#define USART_SR_RXNE 32U
#define UART_IT_PE 1U
#define UART_IT_ERR 2U
#define UART_IT_RXNE 4U
void fake_clear_ore(UART_HandleTypeDef *);
void fake_enable_uart_it(UART_HandleTypeDef *, uint32_t);
#define __HAL_UART_CLEAR_OREFLAG(h) fake_clear_ore(h)
#define __HAL_UART_ENABLE_IT(h, i) fake_enable_uart_it(h, i)
void HAL_NVIC_SetPriority(uint32_t, uint32_t, uint32_t);
void HAL_NVIC_ClearPendingIRQ(uint32_t);
void HAL_NVIC_EnableIRQ(uint32_t);
void USART1_IRQHandler(void);

typedef uint8_t u8;
typedef uint32_t u32;
typedef uintptr_t UINTPTR;
typedef struct { UINTPTR BaseAddress; } XUartPs_Config;
typedef struct { XUartPs_Config Config; } XUartPs;
typedef struct { int unused; } XScuGic;
extern XScuGic xInterruptController;
typedef void (*XInterruptHandler)(void *);
#define XST_SUCCESS 0
#define XUARTPS_OPER_MODE_NORMAL 0U
#define XPAR_XUARTPS_0_DEVICE_ID 0U
#define XPAR_XUARTPS_0_BASEADDR 0xE0001000U
#define XPS_UART0_BASEADDR 0xE0000000U
#define XPS_UART1_BASEADDR 0xE0001000U
#define XPS_UART0_INT_ID 59U
#define XPS_UART1_INT_ID 82U
#define XUARTPS_CR_OFFSET 0x00U
#define XUARTPS_ISR_OFFSET 0x14U
#define XUARTPS_SR_OFFSET 0x2CU
#define XUARTPS_FIFO_OFFSET 0x30U
#define XUARTPS_CR_RX_DIS 0x08U
#define XUARTPS_CR_RX_EN 0x04U
#define XUARTPS_CR_RXRST 0x01U
#define XUARTPS_CR_TORST 0x40U
#define XUARTPS_IXR_RXOVR 0x01U
#define XUARTPS_IXR_OVER 0x20U
#define XUARTPS_IXR_FRAMING 0x40U
#define XUARTPS_IXR_PARITY 0x80U
#define XUARTPS_IXR_TOUT 0x100U
#define XUARTPS_IXR_MASK 0x3FFFU
#define XUARTPS_SR_RXEMPTY 0x02U
XUartPs_Config *XUartPs_LookupConfig(uint16_t);
int XUartPs_CfgInitialize(XUartPs *, XUartPs_Config *, UINTPTR);
int XUartPs_SetBaudRate(XUartPs *, u32);
void XUartPs_SetInterruptMask(XUartPs *, u32);
void XUartPs_SetOperMode(XUartPs *, u8);
void XUartPs_SetFifoThreshold(XUartPs *, u8);
void XUartPs_SetRecvTimeout(XUartPs *, u8);
u32 XUartPs_ReadReg(UINTPTR, u32);
void XUartPs_WriteReg(UINTPTR, u32, u32);
BaseType_t xPortInstallInterruptHandler(uint8_t, XInterruptHandler, void *);
void vPortEnableInterrupt(uint8_t);
void XScuGic_SetPriorityTriggerType(XScuGic *, u32, u8, u8);
#ifdef __cplusplus
}
#endif
#endif
