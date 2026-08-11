# PC UART Transport Software Requirements

## 1. Purpose

This document defines the software requirements for the Linux PC UART
Transport module.

The PC UART Transport provides a byte-oriented communication path
between the PC application and an external UART device.

The transport module shall remain independent of application messages,
payload structures, and UART frame contents.

---

## 2. Scope

The PC UART Transport is responsible for:

* Opening a caller-specified Linux serial device
* Configuring the serial device
* Transmitting a caller-provided byte sequence
* Handling partial and interrupted write operations
* Closing the serial device
* Reporting transport-level success or failure
* Reporting whether the serial device is currently open

The PC UART Transport is not responsible for:

* Creating `TargetCommand` objects
* Serializing or deserializing `TargetCommand`
* Encoding or decoding UART frames
* Calculating or validating CRC values
* Interpreting message types
* Receiving UART data
* Parsing incoming UART frames
* Delivering commands to STM32 FreeRTOS tasks

UART reception and STM32-side processing are outside the scope of this
module.

---

## 3. Initial UART Configuration

The initial UART communication configuration shall be:

| Parameter             | Value    |
| --------------------- | --------:|
| Baud rate             | 115200   |
| Data bits             | 8        |
| Parity                | None     |
| Stop bits             | 1        |
| Hardware flow control | Disabled |
| Software flow control | Disabled |
| Operating mode        | Raw      |
| Platform              | Linux    |

This configuration is commonly described as:

```text
115200 8N1
```

---

## 4. Software Requirements

### PC-UART-SWREQ-001: Device Path Input

The transport shall accept a caller-provided Linux serial device path.

Example device paths include:

```text
/dev/ttyUSB0
/dev/ttyACM0
```

The device path shall not be hardcoded inside the transport module.

---

### PC-UART-SWREQ-002: Invalid Device Path Rejection

The transport shall reject:

* A null device path
* An empty device path
* A path that cannot be opened as a serial device

If opening fails, the transport shall remain in the closed state.

---

### PC-UART-SWREQ-003: Serial Device Opening

The transport shall open the serial device for both reading and writing.

Opening the device shall not assign the process as the controlling
terminal for that device.

The transport shall report whether the open operation succeeded.

---

### PC-UART-SWREQ-004: UART Configuration

After successfully opening the device, the transport shall configure it
as follows:

* 115200 baud
* 8 data bits
* No parity
* One stop bit
* Hardware flow control disabled
* Software flow control disabled
* Raw byte mode

---

### PC-UART-SWREQ-005: Configuration Failure Handling

If serial configuration fails after the device has been opened, the
transport shall:

1. Close the opened file descriptor
2. Return to the closed state
3. Report failure to the caller

The transport shall not retain a partially configured device.

---

### PC-UART-SWREQ-006: Protocol Independence

The transport shall operate on a caller-provided byte buffer.

The transport shall not depend on:

* `TargetCommand`
* `UartFrameView`
* UART message type values
* Payload layouts
* CRC algorithms
* UART frame constants

The transport shall transmit arbitrary byte sequences.

---

### PC-UART-SWREQ-007: Byte-Exact Transmission

The transport shall transmit bytes without modifying:

* Byte values
* Byte order
* Buffer contents

The byte sequence observed by the receiving endpoint shall match the
caller-provided byte sequence.

---

### PC-UART-SWREQ-008: Complete Transmission

The transport shall continue writing until:

* All requested bytes have been accepted by the operating system, or
* An unrecoverable write error occurs

The transport shall not assume that a single `write()` call transmits
the entire requested buffer.

---

### PC-UART-SWREQ-009: Interrupted Write Handling

If a write operation fails because it was interrupted by a signal and
the operating system reports `EINTR`, the transport shall retry the
write operation.

An interrupted system call shall not immediately be treated as an
unrecoverable transmission failure.

---

### PC-UART-SWREQ-010: Invalid Write Parameter Rejection

The transport shall reject a write request when:

* The data pointer is null and the requested size is nonzero
* The transmitted-byte output pointer is null

Parameter validation failure shall occur before transmission begins.

---

### PC-UART-SWREQ-011: Zero-Length Transmission

A write request with:

* A valid transmitted-byte output pointer
* A requested size of zero

shall succeed without calling the operating system write function.

The reported transmitted-byte count shall be zero.

The data pointer may be null when the requested size is zero.

---

### PC-UART-SWREQ-012: Closed-Port Transmission Rejection

The transport shall reject a nonzero transmission request when no
serial device is open.

No operating system write operation shall be attempted while the
transport is closed.

---

### PC-UART-SWREQ-013: Transmitted Byte Count

The transport shall report the number of bytes successfully written.

When transmission completes successfully, the reported count shall
equal the requested byte count.

If an error occurs after a partial transmission, the reported count
shall represent the number of bytes written before the error.

If parameter validation fails before transmission begins, the caller's
output value shall remain unchanged.

---

### PC-UART-SWREQ-014: Open-State Reporting

The transport shall provide a method that reports whether a valid
serial device is currently open.

The reported state shall satisfy the following transitions:

```text
Initial state             → Closed
Successful open           → Open
Failed open               → Closed
Successful close          → Closed
Configuration failure     → Closed
Object destruction        → Closed
```

---

### PC-UART-SWREQ-015: Resource Release

The transport shall release its Linux file descriptor when:

* The caller explicitly closes the transport
* The transport object is destroyed
* Serial configuration fails after opening

Closing an already closed transport shall be safe.

---

### PC-UART-SWREQ-016: Exclusive Descriptor Ownership

A transport object shall exclusively own its active Linux file
descriptor.

The initial implementation shall prevent accidental copying of an open
transport object.

This requirement prevents multiple objects from independently closing
the same file descriptor.

---

### PC-UART-SWREQ-017: No Explicit Dynamic Memory Allocation

The transport implementation shall not explicitly use dynamic memory
allocation.

The following shall not be used by the transport module:

```text
new
delete
malloc
calloc
realloc
free
```

The transport shall operate on caller-owned buffers.

---

### PC-UART-SWREQ-018: Language and Platform

The PC UART Transport shall:

* Be implemented in C++17
* Target Linux
* Use POSIX file-descriptor APIs
* Use the Linux/POSIX `termios` interface for UART configuration

---

### PC-UART-SWREQ-019: Error Reporting

Public transport operations that can fail shall report success or
failure to the caller.

The initial implementation is not required to throw C++ exceptions.

Detailed diagnostic information may be obtained from the operating
system error value where appropriate.

---

### PC-UART-SWREQ-020: Transmission Completion Meaning

A successful transport write shall mean that all requested bytes have
been accepted by the Linux operating system.

It shall not guarantee that every byte has already physically left the
UART transmitter at the exact moment the function returns.

Waiting for physical UART transmission completion is outside the
initial transport requirement and may be added later if required for
latency measurement.

---

## 5. Design Constraints

The transport implementation shall maintain the following dependency
direction:

```text
PC Application
    ↓
Protocol Codec
    ↓
UART Transport
    ↓
Linux POSIX API
```

The UART Transport shall not include protocol headers such as:

```text
protocol/target_command.h
protocol/target_command_codec.h
protocol/uart_frame.h
protocol/uart_frame_codec.h
protocol/crc16.h
```

The transport interface shall use only generic byte-buffer types and
transport-related types.

---

## 6. Planned Verification Mapping

| Requirement ID    | Planned verification                                                        |
| ----------------- | --------------------------------------------------------------------------- |
| PC-UART-SWREQ-001 | Open a caller-specified pseudo-terminal slave                               |
| PC-UART-SWREQ-002 | Test null, empty, nonexistent, and invalid paths                            |
| PC-UART-SWREQ-003 | Verify successful pseudo-terminal opening                                   |
| PC-UART-SWREQ-004 | Inspect configured `termios` attributes                                     |
| PC-UART-SWREQ-005 | Inject or reproduce configuration failure and verify closed state           |
| PC-UART-SWREQ-006 | Inspect transport header dependencies and transmit arbitrary bytes          |
| PC-UART-SWREQ-007 | Compare transmitted bytes with bytes received from a pseudo-terminal master |
| PC-UART-SWREQ-008 | Verify the write loop continues until the complete buffer is handled        |
| PC-UART-SWREQ-009 | Code inspection and interrupted-write test where practical                  |
| PC-UART-SWREQ-010 | Test null data and null output pointer handling                             |
| PC-UART-SWREQ-011 | Test zero-length transmission                                               |
| PC-UART-SWREQ-012 | Attempt transmission before opening                                         |
| PC-UART-SWREQ-013 | Verify success, validation failure, and partial-failure count semantics     |
| PC-UART-SWREQ-014 | Verify all open and closed state transitions                                |
| PC-UART-SWREQ-015 | Verify explicit close, repeated close, and destructor cleanup               |
| PC-UART-SWREQ-016 | Compile-time verification that copying is disabled                          |
| PC-UART-SWREQ-017 | Source-code inspection                                                      |
| PC-UART-SWREQ-018 | Linux C++17 build verification                                              |
| PC-UART-SWREQ-019 | Verify public operation return values                                       |
| PC-UART-SWREQ-020 | Design and source-code inspection                                           |

---

## 7. Planned Implementation Traceability

The detailed mapping will be completed after the transport interface and
implementation are created.

| Requirement group        | Planned implementation area                          |
| ------------------------ | ---------------------------------------------------- |
| Device opening and state | `SerialPort::open()`, `SerialPort::is_open()`        |
| UART configuration       | Internal serial configuration function               |
| Byte transmission        | `SerialPort::write_all()`                            |
| Resource cleanup         | `SerialPort::close()`, destructor                    |
| Ownership protection     | Deleted copy constructor and copy assignment         |
| Linux integration        | POSIX `open()`, `write()`, `close()`, `termios` APIs |

---

## 8. Out-of-Scope Items

The following items are outside Take 5-1 and the initial PC UART
Transport implementation:

* UART data reception
* Bidirectional request-response communication
* UART frame stream parsing
* STM32 UART initialization
* STM32 DMA reception
* STM32 interrupt reception
* FreeRTOS queue integration
* Acknowledgement messages
* Retry protocols
* Timeouts for remote acknowledgements
* Physical transmission completion using `tcdrain()`
* End-to-end latency measurement
* Automatic serial-device discovery
* Windows or macOS serial-port support
