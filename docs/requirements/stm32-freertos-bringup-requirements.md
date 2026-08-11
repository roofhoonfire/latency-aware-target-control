# STM32 FreeRTOS Bring-up Requirements

## Purpose

This document defines the requirements for initial FreeRTOS
integration on the STM32F429I-DISC1 controller.

This stage verifies the RTOS kernel and task scheduler independently
from UART command processing and actuator control.

---

## STM32-RTOS-001 — FreeRTOS Kernel Integration

The STM32 firmware shall include and initialize the FreeRTOS kernel.

---

## STM32-RTOS-002 — Scheduler Start

The STM32 firmware shall successfully start the FreeRTOS scheduler.

Normal application execution shall be performed by RTOS tasks after
the scheduler starts.

---

## STM32-RTOS-003 — Task Creation

At least two application tasks shall be created for scheduler
bring-up verification.

---

## STM32-RTOS-004 — Task Execution

Each bring-up task shall execute periodically and provide an
observable deterministic indication that it has been scheduled.

---

## STM32-RTOS-005 — HAL and RTOS Timebase Separation

The HAL timebase and FreeRTOS kernel tick shall not depend on the
same SysTick timebase.

The HAL timebase shall use TIM6 while SysTick is reserved for the
FreeRTOS kernel tick.

---

## STM32-RTOS-006 — Existing Firmware Build Compatibility

The firmware shall compile and link successfully after FreeRTOS
integration without removing the existing USART1 and common protocol
integration.
