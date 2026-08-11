# STM32 TargetCommand Queue Requirements

## Purpose

This document defines the requirements for transferring a
TargetCommand between STM32 FreeRTOS tasks using a CMSIS-RTOS2
message queue.

This stage verifies inter-task TargetCommand transfer independently
from UART reception and frame processing.

---

## STM32-TCMD-QUEUE-001 — Queue Creation

The STM32 firmware shall create a CMSIS-RTOS2 message queue capable
of storing TargetCommand objects.

---

## STM32-TCMD-QUEUE-002 — Queue Element Type

Each queue element shall contain one complete TargetCommand value.

The queue shall transfer TargetCommand data by value rather than
sharing a pointer to mutable command storage.

---

## STM32-TCMD-QUEUE-003 — Producer Task

The producer task shall be able to place a TargetCommand into the
message queue using osMessageQueuePut().

---

## STM32-TCMD-QUEUE-004 — Consumer Task

The consumer task shall retrieve a TargetCommand from the message
queue using osMessageQueueGet().

---

## STM32-TCMD-QUEUE-005 — Data Integrity

The TargetCommand retrieved by the consumer task shall contain the
same field values as the command placed into the queue by the
producer task.

---

## STM32-TCMD-QUEUE-006 — Deterministic Transfer Result

The firmware shall expose observable state indicating whether a
TargetCommand was successfully sent to and received from the queue.
