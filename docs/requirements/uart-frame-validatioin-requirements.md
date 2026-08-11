# UART Frame Validation Requirements

## Purpose

This document defines the requirements for validating UART frames
received by the STM32 controller.

The validation shall be independent of the UART hardware transport
and operate on a received byte buffer.

---

## UART-FRAME-VAL-001 — Start of Frame Validation

The validator shall accept a frame only when the first two bytes are:

- SOF1 = 0xAA
- SOF2 = 0x55

If either SOF byte is incorrect, the frame shall be rejected.

---

## UART-FRAME-VAL-002 — Protocol Version Validation

The validator shall accept protocol version 0x01.

A frame containing an unsupported protocol version shall be rejected.

---

## UART-FRAME-VAL-003 — Message Type Validation

The validator shall accept message type 0x01 as a TargetCommand frame.

A frame containing an unsupported message type shall be rejected.

---

## UART-FRAME-VAL-004 — Payload Length Validation

For a TargetCommand frame, the payload length shall be exactly 8 bytes.

A TargetCommand frame whose payload length is not 8 bytes shall be rejected.

---

## UART-FRAME-VAL-005 — Frame Length Validation

A TargetCommand UART frame shall contain exactly 15 bytes:

- 2 bytes SOF
- 1 byte Version
- 1 byte Message Type
- 1 byte Payload Length
- 8 bytes Payload
- 2 bytes CRC16

A buffer whose length does not match the expected frame length shall be rejected.

---

## UART-FRAME-VAL-006 — CRC Validation

The validator shall calculate CRC-16/CCITT-FALSE over:

- Version
- Message Type
- Payload Length
- Payload

The SOF bytes and received CRC bytes shall not be included in the CRC calculation.

The calculated CRC shall match the received CRC.

The received CRC shall be interpreted as Little Endian.

A frame with a CRC mismatch shall be rejected.

---

## UART-FRAME-VAL-007 — Validation Result

The validator shall return a deterministic result indicating whether
the received frame is valid or invalid.

The validator shall not modify the received frame buffer.
