# UART Frame Software Requirements

## 1. Purpose

This document defines the software requirements for encoding and decoding UART frames exchanged between the PC application and the STM32 controller.

The UART frame provides:

* Frame boundary identification
* Protocol version identification
* Message type identification
* Payload length information
* Payload integrity verification

This document defines the requirements for a complete UART frame. UART byte-stream reception, partial-frame buffering, timeout handling, and stream resynchronization will be specified separately.

---

## 2. Scope

The UART frame codec shall be shared by:

* The Linux PC application
* The STM32 firmware
* Host-based unit tests

The codec shall convert an application payload into a complete UART frame and validate a complete received frame.

The initial supported application message is `TargetCommand`.

---

## 3. UART Frame Format

A UART frame shall have the following format.

| Offset |    Size | Field          | Description                    |
| -----: | ------: | -------------- | ------------------------------ |
|      0 |  1 byte | SOF 1          | Start-of-frame byte `0xAA`     |
|      1 |  1 byte | SOF 2          | Start-of-frame byte `0x55`     |
|      2 |  1 byte | Version        | Protocol version               |
|      3 |  1 byte | Message Type   | Application message identifier |
|      4 |  1 byte | Payload Length | Payload size in bytes          |
|      5 | N bytes | Payload        | Serialized application message |
|  5 + N | 2 bytes | CRC-16         | Frame integrity value          |

The total frame size shall be:

```text
Total Frame Size = Payload Length + 7 bytes
```

For a `TargetCommand` payload:

```text
Payload Length   = 8 bytes
Total Frame Size = 15 bytes
```

---

## 4. Protocol Constants

| Name                               |  Value | Description                      |
| ---------------------------------- | -----: | -------------------------------- |
| `UART_FRAME_SOF_1`                 | `0xAA` | First start-of-frame byte        |
| `UART_FRAME_SOF_2`                 | `0x55` | Second start-of-frame byte       |
| `UART_FRAME_VERSION`               | `0x01` | Initial protocol version         |
| `UART_MESSAGE_TYPE_TARGET_COMMAND` | `0x01` | TargetCommand message identifier |
| `UART_FRAME_HEADER_SIZE`           |    `5` | Bytes before the payload         |
| `UART_FRAME_CRC_SIZE`              |    `2` | CRC field size                   |
| `UART_FRAME_OVERHEAD_SIZE`         |    `7` | Header and CRC size              |

---

## 5. Functional Requirements

### SWR-UART-FRM-001 — Frame Field Order

The UART frame encoder shall write the frame fields in the following order:

```text
SOF 1
SOF 2
Version
Message Type
Payload Length
Payload
CRC-16
```

### SWR-UART-FRM-002 — Start-of-Frame Bytes

Every encoded UART frame shall begin with the following two bytes:

```text
0xAA 0x55
```

The decoder shall reject a frame when either start-of-frame byte is incorrect.

### SWR-UART-FRM-003 — Protocol Version

The encoder shall write `0x01` into the Version field.

The decoder shall reject a frame whose Version field is not supported.

The initial implementation shall support only Version `0x01`.

### SWR-UART-FRM-004 — Message Type

The frame shall contain a one-byte Message Type field.

The Message Type value for a `TargetCommand` payload shall be:

```text
0x01
```

The frame codec shall preserve the Message Type value during encoding and decoding.

Application-level code shall determine whether the decoded Message Type is supported.

### SWR-UART-FRM-005 — Payload Length

The Payload Length field shall represent the exact number of payload bytes contained in the frame.

The Payload Length field shall be one byte and shall therefore represent values from 0 to 255 bytes.

The encoder shall reject a payload whose size is greater than 255 bytes.

### SWR-UART-FRM-006 — TargetCommand Payload Length

A frame whose Message Type is `UART_MESSAGE_TYPE_TARGET_COMMAND` shall contain an 8-byte payload.

The eight payload bytes shall be generated using the `TargetCommand` payload codec.

The `TargetCommand` structure memory shall not be copied directly into the UART frame.

### SWR-UART-FRM-007 — Total Frame Size

The required output frame size shall be calculated as:

```text
Payload Length + UART_FRAME_OVERHEAD_SIZE
```

The UART frame overhead shall be seven bytes.

A TargetCommand UART frame shall therefore be exactly 15 bytes.

### SWR-UART-FRM-008 — CRC Algorithm

The frame codec shall use CRC-16/CCITT-FALSE with the following parameters:

| Parameter         |    Value |
| ----------------- | -------: |
| Width             |  16 bits |
| Polynomial        | `0x1021` |
| Initial value     | `0xFFFF` |
| Input reflection  |    False |
| Output reflection |    False |
| Final XOR value   | `0x0000` |

The standard test input:

```text
123456789
```

shall produce:

```text
0x29B1
```

### SWR-UART-FRM-009 — CRC Coverage

The CRC shall be calculated over the following fields:

```text
Version
Message Type
Payload Length
Payload
```

The CRC calculation shall not include:

```text
SOF 1
SOF 2
CRC field
```

### SWR-UART-FRM-010 — CRC Byte Order

The 16-bit CRC value shall be stored in Little Endian order.

```text
First CRC byte  = lower 8 bits
Second CRC byte = upper 8 bits
```

### SWR-UART-FRM-011 — Encoder Input Validation

The frame encoder shall return failure when:

* The output frame pointer is null.
* The payload pointer is null while the payload size is greater than zero.
* The payload size is greater than 255 bytes.
* The output frame buffer is smaller than the required frame size.

A null payload pointer may be accepted when the payload size is zero.

### SWR-UART-FRM-012 — Encoder Buffer Safety

The encoder shall not write beyond the output frame buffer provided by the caller.

The encoder shall write exactly the number of bytes required for the encoded frame when encoding succeeds.

### SWR-UART-FRM-013 — Decoder Frame Size Validation

The decoder shall reject a frame when:

* The supplied frame is smaller than the minimum seven-byte frame size.
* The supplied frame size does not equal `Payload Length + 7`.
* The payload length would cause access beyond the supplied frame buffer.

### SWR-UART-FRM-014 — Decoder Frame Validation

The decoder shall reject a frame when any of the following conditions occurs:

* Invalid SOF 1
* Invalid SOF 2
* Unsupported protocol version
* Invalid frame size
* Payload length mismatch
* CRC mismatch

### SWR-UART-FRM-015 — Decoder Output Protection

The decoder shall not expose a decoded frame as valid unless all frame validation checks have succeeded.

The decoder shall not modify the caller-visible decoded output when frame validation fails.

### SWR-UART-FRM-016 — Encode/Decode Consistency

For every valid supported payload, encoding followed by decoding shall preserve:

* Protocol version
* Message type
* Payload length
* Every payload byte

### SWR-UART-FRM-017 — Dynamic Memory

The UART frame codec shall not allocate dynamic memory.

The caller shall provide all payload, frame, and decoded-output buffers.

### SWR-UART-FRM-018 — C and C++ Compatibility

The public UART frame codec interface shall be usable from both C and C++ code.

The implementation shall be usable by both the PC application and STM32 firmware.

---

## 6. Error-Handling Policy

The frame encoder and decoder shall indicate success or failure through their return values.

A failed frame shall not be passed to the application-message decoder.

A frame with a CRC mismatch shall be discarded.

CRC detects corrupted data but does not recover or retransmit it.

Acknowledgment and retransmission are outside the scope of the UART frame codec.

---

## 7. Initial Message Type Assignment

| Message Type | Name            | Payload Size |
| -----------: | --------------- | -----------: |
|       `0x01` | `TargetCommand` |      8 bytes |

Additional message types may be assigned later without changing the common UART frame layout.

Potential future messages include:

* Heartbeat
* Control status
* Latency report
* Error report

These future message types are not required in the initial implementation.

---

## 8. Verification Traceability

| Requirement      | Planned Verification                       |
| ---------------- | ------------------------------------------ |
| SWR-UART-FRM-001 | Known complete frame byte-pattern test     |
| SWR-UART-FRM-002 | Valid and invalid SOF tests                |
| SWR-UART-FRM-003 | Supported and unsupported version tests    |
| SWR-UART-FRM-004 | Message Type preservation test             |
| SWR-UART-FRM-005 | Zero, normal, and oversized payload tests  |
| SWR-UART-FRM-006 | TargetCommand payload-size test            |
| SWR-UART-FRM-007 | Required frame-size test                   |
| SWR-UART-FRM-008 | CRC standard check-value test              |
| SWR-UART-FRM-009 | Known CRC coverage test                    |
| SWR-UART-FRM-010 | CRC byte-order test                        |
| SWR-UART-FRM-011 | Null pointer and insufficient-buffer tests |
| SWR-UART-FRM-012 | Sentinel-based buffer-overrun test         |
| SWR-UART-FRM-013 | Truncated and oversized frame tests        |
| SWR-UART-FRM-014 | Corrupted header, payload, and CRC tests   |
| SWR-UART-FRM-015 | Output-unchanged-on-failure test           |
| SWR-UART-FRM-016 | Encode/decode round-trip test              |
| SWR-UART-FRM-017 | Code review                                |
| SWR-UART-FRM-018 | C implementation and C++ unit-test build   |

---

## 9. Out of Scope

The following items are not part of the initial UART frame codec:

* Opening or configuring a Linux serial port
* STM32 UART peripheral configuration
* DMA or interrupt-based UART reception
* Partial-frame buffering
* UART receive timeout handling
* Byte-stream state machine
* SOF search and stream resynchronization
* Acknowledgment messages
* Retransmission
* Message queuing
* Stale-command rejection
* Sequence-number comparison

These items will be specified in later PC transport and STM32 receiver requirements.

