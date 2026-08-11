# STM32 UART to Queue Integration Requirements

## Purpose

This document defines the requirements for integrating UART reception,
UART frame validation, TargetCommand reconstruction, and FreeRTOS
message queue transfer on the STM32.

---

## STM32-UART-QUEUE-001 — UART Ownership

UART frame reception shall be performed by CommRxTask after the
FreeRTOS scheduler has started.

---

## STM32-UART-QUEUE-002 — Frame Validation

CommRxTask shall validate each received TargetCommand UART frame using
uart_frame_validate_target_command().

An invalid frame shall not be placed into the TargetCommand queue.

---

## STM32-UART-QUEUE-003 — TargetCommand Reconstruction

A valid TargetCommand frame shall be reconstructed using
target_command_deserialize().

Raw payload memory shall not be cast directly to TargetCommand.

---

## STM32-UART-QUEUE-004 — Queue Transfer

A successfully reconstructed TargetCommand shall be placed into
targetCommandQueue using osMessageQueuePut().

---

## STM32-UART-QUEUE-005 — ControlTask Reception

ControlTask shall retrieve TargetCommand values from
targetCommandQueue using osMessageQueueGet().

---

## STM32-UART-QUEUE-006 — End-to-End Data Integrity

For the known PC test command, ControlTask shall receive:

- sequence = 1
- target_x = 1000
- target_y = -1000
- prediction_ms = 40

---

## STM32-UART-QUEUE-007 — Invalid Command Isolation

If UART reception, frame validation, or TargetCommand deserialization
fails, the invalid command shall not become available to ControlTask.
