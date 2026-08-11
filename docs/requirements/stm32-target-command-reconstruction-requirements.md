# STM32 TargetCommand Reconstruction Requirements

## Purpose

This document defines the requirements for reconstructing a
TargetCommand from a validated UART frame on the STM32 controller.

TargetCommand reconstruction shall be performed only after the
received UART frame has successfully passed TargetCommand frame
validation.

---

## STM32-TCMD-RX-001 — Validation Before Reconstruction

The STM32 application shall attempt TargetCommand reconstruction
only when the received UART frame has been successfully validated
as a TargetCommand frame.

A frame that fails validation shall not be deserialized as a
TargetCommand.

---

## STM32-TCMD-RX-002 — Payload Selection

The STM32 application shall use the payload of the validated
TargetCommand UART frame as the input to TargetCommand
deserialization.

The payload size shall be TARGET_COMMAND_WIRE_SIZE.

---

## STM32-TCMD-RX-003 — TargetCommand Deserialization

The STM32 application shall reconstruct the TargetCommand by using
target_command_deserialize().

The application shall not manually reinterpret the serialized
payload as a TargetCommand structure in memory.

---

## STM32-TCMD-RX-004 — Reconstruction Failure Handling

If TargetCommand deserialization fails, the received command shall
not be considered available for further control processing.

The application shall expose a deterministic indication of whether
TargetCommand reconstruction succeeded or failed.

---

## STM32-TCMD-RX-005 — Known Frame Reconstruction

For the following valid UART frame:

AA 55 01 01 08 01 00 E8 03 18 FC 28 00 9F E2

the reconstructed TargetCommand shall contain:

- sequence = 1
- target_x = 1000
- target_y = -1000
- prediction_ms = 40
