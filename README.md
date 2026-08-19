# Latency-Aware Target Control

## Project Description

Latency-Aware Target Control is an embedded system integration project that connects synthetic vehicle perception on MATLAB/Simulink with a Linux C++ application and an STM32F429 real-time controller.

The current implementation establishes a continuous live data path from synthetic vision detection to `TargetCommand` reception inside a FreeRTOS `ControlTask`.

Valid target detections generated while Simulink is running are streamed to the Linux application over UDP, converted into application-level commands, transmitted to the STM32 over UART, validated, deserialized, and delivered through a FreeRTOS message queue.

A deterministic CSV replay path is also retained for reproducible testing and regression validation.

Based on this validated live pipeline, the project is being extended toward end-to-end latency measurement and actuation-time-aligned target prediction.

<p align="center">
  <img src="docs/images/system_architecture.png"
       alt="Latency-Aware Target Control System Architecture"
       width="650">
</p>

<p align="center">
  <em>Live system architecture from synthetic vision sensing to Linux command generation and STM32/FreeRTOS control.</em>
</p>

## Overview

The system models a two-layer control architecture:

* **Linux / High-Level Processing:** live perception input reception, command generation, protocol encoding, and UART transmission
* **STM32 / Real-Time Local Control:** frame validation, command reconstruction, RTOS communication, and control-side execution

A controlled vehicle scenario is generated using MATLAB/Simulink and Automated Driving Toolbox. `Vision Detection Generator` produces synthetic target detections with configurable measurement noise, detection probability, and false positives.

For each valid target detection, the following state is extracted:

```text
[time, x, y, vx, vy]
```

The primary integration path operates online:

```text
MATLAB / Simulink
    ↓
Synthetic Vision Detection
    ↓
Target Detection Filtering
    ↓
Live UDP Streaming
    ↓
Linux C++
    ↓
TargetCommand
    ↓
UART
    ↓
STM32F429 + FreeRTOS
    ↓
ControlTask
```

The Linux application receives detections continuously, converts each valid record into a `TargetCommand`, serializes it into an 8-byte payload, encapsulates it in a CRC-16 protected UART frame, and transmits the resulting frame to the STM32.

On the STM32F429, `CommRxTask` validates and deserializes each frame before sending the reconstructed command through a CMSIS-RTOS2 message queue to `ControlTask`.

A CSV-based input path is retained as a deterministic replay mode for protocol and integration testing.

## Motivation / Problem Definition

A target position is valid when it is observed, but a physical actuator cannot respond at that same instant.

In a distributed control system, target information passes through multiple stages before control is applied:

```text
Perception
    ↓
High-Level Processing
    ↓
Command Generation
    ↓
Communication
    ↓
MCU Scheduling
    ↓
Control Execution
    ↓
Physical Actuation
```

During this interval, a moving target continues to change its position. Therefore, a command generated directly from the latest detected position can already be stale when actuation occurs.

```text
Target state measured at t0
        ↓
Processing + Communication + Scheduling Delay
        ↓
Control applied at t0 + Δt
```

For a moving target:

```text
Target Position at t0 ≠ Target Position at t0 + Δt
```

This project treats the perception-to-actuation delay as a system-level control problem.

The intended latency-aware strategy combines target position, target velocity, and measured latency to estimate the target state at the expected actuation time.

```text
Current Target Position
        +
Target Velocity
        +
Measured Latency
        ↓
Predicted Future Target Position
        ↓
Control Command
```

For a constant-velocity target, the basic prediction model is:

```text
x_future = x_detected + vx × Δt
y_future = y_detected + vy × Δt
```

The final evaluation will compare:

* **Baseline:** control using the latest detected target position
* **Latency-Aware:** control using the predicted target position at the expected actuation time

The effectiveness of the latency-aware approach will be evaluated quantitatively using tracking error.

## System Architecture

The system is divided into three functional layers.

### 1. Simulation & Synthetic Sensing

MATLAB/Simulink and Automated Driving Toolbox provide a controlled and reproducible perception source.

The simulation layer generates:

* Detection timestamp
* Target position
* Target velocity
* Measurement noise
* Probabilistic detections
* False-positive detections

`Vision Detection Generator` outputs a detection bus during simulation.

The live extraction path selects the target vehicle using its actor ID and rejects false positives and detections belonging to other actors.

Each valid target detection is reduced to:

```text
[time, x, y, vx, vy]
```

and streamed immediately to the Linux application.

### 2. Linux High-Level Processing

The Linux C++ application supports two perception input modes:

```text
Live Mode
Simulink → UDP → UdpDetectionReceiver
                       ↓
                 DetectionRecord
```

```text
Replay Mode
CSV → DetectionCsvReader
              ↓
        DetectionRecord
```

Both paths produce the same internal `DetectionRecord`, allowing the protocol and control pipeline to remain independent of the perception input source.

The Linux application is responsible for:

* Receiving live synthetic detections
* Supporting deterministic CSV replay
* Converting physical coordinates into command units
* Constructing sequential `TargetCommand` messages
* Serializing commands into a fixed wire format
* Encoding CRC-protected UART frames
* Transmitting frames through a POSIX serial interface

The current baseline uses the latest detected position without future-position compensation.

Detection time and target velocity are retained for the latency-aware prediction stage.

### 3. STM32 Real-Time Local Controller

The STM32F429I-DISC1 runs FreeRTOS through CMSIS-RTOS2.

Two tasks separate communication from control-side processing:

* **`CommRxTask`**

  * Receives UART frames
  * Validates protocol fields and CRC
  * Deserializes `TargetCommand`
  * Sends commands to the RTOS message queue

* **`ControlTask`**

  * Waits for valid commands
  * Receives them through `targetCommandQueue`
  * Performs control-side processing

This separation keeps high-level perception and command generation on Linux while the STM32 handles communication validation and real-time task execution.

## Simulation & Synthetic Sensor

The current simulation uses a constant-velocity scenario with an ego vehicle and a moving target vehicle.

`Scenario Reader` provides the ground-truth actor states, while `Vision Detection Generator` models a non-ideal vision sensor.

### Sensor Configuration

| Parameter                   | Value         |
| --------------------------- | ------------- |
| Detection probability       | `0.70`        |
| False positives per image   | `1.0`         |
| Measurement noise           | Enabled       |
| Random seed                 | `42`          |
| Sensor update interval      | `0.1 s`       |
| Detection coordinate system | Ego Cartesian |

A fixed random seed is used to make the probabilistic sensor behavior reproducible.

### Measurement Noise

The synthetic measurement does not perfectly overlap the ground-truth target position.

<p align="center">
  <img src="docs/images/synthetic_sensor_validation_1.png"
       alt="Synthetic vision sensor measurement noise"
       width="750">
</p>

<p align="center">
  <em>Ground-truth target and noisy synthetic vision detection.</em>
</p>

### False Positive

The sensor can generate a detection where no target vehicle exists.

<p align="center">
  <img src="docs/images/synthetic_sensor_validation_2.png"
       alt="Synthetic vision sensor false positive"
       width="750">
</p>

<p align="center">
  <em>False-positive detection generated independently of the actual target vehicle.</em>
</p>

### Missed Detection

Because the detection probability is below `1.0`, the target can remain inside the sensor field of view without generating a corresponding detection.

<p align="center">
  <img src="docs/images/synthetic_sensor_validation_3.png"
       alt="Synthetic vision sensor missed detection"
       width="750">
</p>

<p align="center">
  <em>Target vehicle present in the scenario while the synthetic detector misses the target.</em>
</p>

### Live Detection Extraction

The target vehicle is identified using:

```text
targetActorId = 2
```

For each sensor update, detections are filtered using `TargetIndex`.

```text
TargetIndex < 0
    → False Positive

TargetIndex != 2
    → Ignore

TargetIndex == 2
    → Valid Target Detection
```

The selected detection contains:

```text
Measurement =
[x, y, z, vx, vy, vz]
```

The live interface uses:

```text
[time, x, y, vx, vy]
```

When the target is not detected, no target packet is transmitted for that sensor update.

This preserves the missed-detection behavior of the synthetic sensor instead of replacing missing observations with zero-valued measurements.

### Simulink-to-Linux Live Transport

A lightweight Simulink `Python Code` block acts only as a transport adapter.

It packs each valid target detection as:

```text
Little Endian
double × 5
```

Packet layout:

| Field       | Type     |    Size |
| ----------- | -------- | ------: |
| `time_sec`  | `double` | 8 bytes |
| `target_x`  | `double` | 8 bytes |
| `target_y`  | `double` | 8 bytes |
| `target_vx` | `double` | 8 bytes |
| `target_vy` | `double` | 8 bytes |

Total UDP payload:

```text
40 bytes
```

Transport endpoint:

```text
Address : 127.0.0.1
Port    : 51001
Protocol: UDP
```

The Python block performs transport only. Prediction, command generation, protocol encoding, and control logic remain in the Linux C++ and STM32 layers.

### CSV Replay

The original extraction workflow is retained for deterministic replay.

Valid target detections can still be exported to:

```text
simulation/simulink/target_detections.csv
```

with the format:

```text
time_sec,target_x,target_y,target_vx,target_vy
```

This path provides a fixed perception input for regression testing independently of live sensor execution.

## Linux / Protocol Design

The Linux application forms the boundary between synthetic perception and the STM32 controller.

### Detection Record

Both live UDP input and CSV replay are converted into the same internal representation:

| Field           | Description         | Unit |
| --------------- | ------------------- | ---- |
| `time_sec`      | Detection timestamp | s    |
| `target_x_m`    | Detected X position | m    |
| `target_y_m`    | Detected Y position | m    |
| `target_vx_mps` | Detected X velocity | m/s  |
| `target_vy_mps` | Detected Y velocity | m/s  |

Example live detection:

```text
t=0.100
x=5.147
y=-0.972
vx=5.226
vy=0.768
```

### Detection Input Modes

The Linux executable supports two input modes.

**Live mode**

```bash
./build/pc/target_control_pc \
    /dev/ttyACM0 \
    --live
```

```text
Simulink
    ↓ UDP
UdpDetectionReceiver
    ↓
DetectionRecord
```

**CSV replay mode**

```bash
./build/pc/target_control_pc \
    /dev/ttyACM0 \
    simulation/simulink/target_detections.csv
```

```text
CSV
    ↓
DetectionCsvReader
    ↓
DetectionRecord
```

The live UDP receiver accepts the 40-byte detection packet, reconstructs the five Little Endian `double` values, and exposes the result as a `DetectionRecord`.

### Coordinate Conversion

The command representation uses centimeters for target position.

```text
5.1475 m
   ↓ × 100
514.75 cm
   ↓ round
515
```

### TargetCommand

The application-level command shared between Linux and STM32 is:

| Field           | Type       |    Size | Description             |
| --------------- | ---------- | ------: | ----------------------- |
| `sequence`      | `uint16_t` | 2 bytes | Command sequence number |
| `target_x`      | `int16_t`  | 2 bytes | Target X position       |
| `target_y`      | `int16_t`  | 2 bytes | Target Y position       |
| `prediction_ms` | `uint16_t` | 2 bytes | Prediction horizon      |

The wire payload is fixed at 8 bytes:

```text
sequence      2 bytes
target_x      2 bytes
target_y      2 bytes
prediction_ms 2 bytes
```

All multi-byte values are serialized in Little Endian order.

The current baseline does not apply future-position compensation:

```text
prediction_ms = 0
```

Live mode assigns a new sequence number to each received target detection:

```text
seq = 1, 2, 3, 4, ...
```

Example:

```text
sequence      = 1
target_x      = 515
target_y      = -97
prediction_ms = 0
```

Serialized payload:

```text
01 00 03 02 9F FF 00 00
```

### UART Frame

The serialized payload is encapsulated in a CRC-protected UART frame.

| Field          | Value              |    Size |
| -------------- | ------------------ | ------: |
| SOF1           | `0xAA`             |  1 byte |
| SOF2           | `0x55`             |  1 byte |
| Version        | `0x01`             |  1 byte |
| Message Type   | `0x01`             |  1 byte |
| Payload Length | `0x08`             |  1 byte |
| Payload        | `TargetCommand`    | 8 bytes |
| CRC            | CRC-16/CCITT-FALSE | 2 bytes |

```text
SOF1 | SOF2 | Version | Type | Length | Payload | CRC
 AA     55      01      01      08      8 B      2 B
```

Total frame size:

```text
15 bytes
```

CRC configuration:

```text
Polynomial = 0x1021
Init       = 0xFFFF
RefIn      = false
RefOut     = false
XorOut     = 0x0000
```

Known CRC test vector:

```text
"123456789" → 0x29B1
```

Validated frame example:

```text
AA 55 01 01 08 01 00 03 02 9F FF 00 00 5E 5A
```

Protocol encode/decode round-trip behavior is verified independently through host-side tests.

The live runtime path therefore performs only the operations required for command transmission instead of repeating test-only decode and comparison work for every sensor update.

### Serial Transport

The Linux serial transport uses:

```text
Baud Rate : 115200
Data Bits : 8
Parity    : None
Stop Bits : 1
Mode      : Raw Serial
```

Typical device:

```text
/dev/ttyACM0
```

In live mode, the serial device is opened once and reused across the continuous detection stream.

```text
Serial Open
    ↓
Receive Detection
    ↓
Build TargetCommand
    ↓
Encode UART Frame
    ↓
Transmit
    ↓
Receive Next Detection
    ↓
...
```

The transport layer remains independent from the protocol layer. `SerialPort` handles raw byte transmission without interpreting `TargetCommand`, frame headers, or CRC fields.

## STM32 / FreeRTOS Architecture

The STM32F429I-DISC1 acts as the real-time local controller.

The receive path is:

```text
USART1
  ↓
CommRxTask
  ↓
UART Frame Validator
  ↓
TargetCommand Deserialize
  ↓
targetCommandQueue
  ↓
ControlTask
```

### CommRxTask

`CommRxTask` performs:

```text
HAL_UART_Receive()
    ↓
Frame Validation
    ↓
TargetCommand Deserialization
    ↓
osMessageQueuePut()
```

The task receives complete 15-byte frames, validates them, reconstructs `TargetCommand`, and sends the commands to the RTOS message queue.

It does not directly execute control logic.

### ControlTask

`ControlTask` performs:

```text
osMessageQueueGet()
    ↓
TargetCommand Receive
    ↓
Local Control Processing
```

The task blocks until a new command becomes available.

### Message Queue

| Item      | Configuration             |
| --------- | ------------------------- |
| Producer  | `CommRxTask`              |
| Consumer  | `ControlTask`             |
| Queue     | `targetCommandQueue`      |
| Item Type | `TargetCommand`           |
| Capacity  | 4 commands                |
| API       | CMSIS-RTOS2 Message Queue |

Queue creation:

```c
osMessageQueueNew(
    4,
    sizeof(TargetCommand),
    &targetCommandQueue_attributes
);
```

The queue provides an explicit boundary between communication processing and control execution.

### Frame Validation

Before a command enters the control path, the frame is checked for:

* Frame length
* Start-of-frame bytes
* Protocol version
* Message type
* Payload length
* CRC-16

Only a valid frame is deserialized and sent to `targetCommandQueue`.

## End-to-End Data Flow

The system now supports continuous live propagation of synthetic target detections.

Example Linux live output:

```text
[LIVE] seq=1  t=0.100  pos=(5.147, -0.972)  cmd=(515, -97)  pred=0ms
[LIVE] seq=2  t=0.200  pos=(5.629, -0.920)  cmd=(563, -92)  pred=0ms
[LIVE] seq=3  t=0.300  pos=(6.209, -0.891)  cmd=(621, -89)  pred=0ms
```

For the first detection:

```text
Synthetic Detection
├── time = 0.100 s
├── x    = 5.147 m
├── y    = -0.972 m
├── vx   = 5.226 m/s
└── vy   = 0.768 m/s
```

Linux generates:

```text
TargetCommand
├── sequence      = 1
├── target_x      = 515
├── target_y      = -97
└── prediction_ms = 0
```

Serialized payload:

```text
01 00 03 02 9F FF 00 00
```

UART frame:

```text
AA 55 01 01 08 01 00 03 02 9F FF 00 00 5E 5A
```

On STM32:

```text
received_command
    ↓
targetCommandQueue
    ↓
queue_received_command
    ↓
ControlTask
```

During live execution, STM32 debug observation confirms that received command sequence and target fields continue to update as new Simulink detections arrive.

Validated live path:

```text
Synthetic Vision Detection
    ↓
Target Actor Filtering
    ↓
Live UDP
    ↓
Linux C++
    ↓
DetectionRecord
    ↓
TargetCommand
    ↓
UART Frame + CRC-16
    ↓
STM32 CommRxTask
    ↓
Frame Validation / Deserialization
    ↓
FreeRTOS Message Queue
    ↓
ControlTask
```

## Validation

Validation is performed at the protocol, simulation, transport, and hardware-integration levels.

### Host-Side Tests

Host-side verification covers:

* `TargetCommand` serialization / deserialization
* CRC-16/CCITT-FALSE
* UART frame encoding / decoding
* UART frame validation
* Linux serial transport

### Synthetic Sensor Validation

The simulation verifies:

* Measurement noise
* Probabilistic detection
* False positives
* Missed detections
* Detection time / position / velocity extraction
* Target actor filtering

### Live UDP Transport Validation

The Simulink-to-Linux transport was validated independently before integration into the main application.

The validation sequence was:

```text
Simulink Test Signals
    ↓
UDP
    ↓
Standalone Receiver
    ↓
Verify [time, x, y, vx, vy]
```

followed by:

```text
Actual Vision Detection
    ↓
Target Actor Filtering
    ↓
UDP
    ↓
Linux C++ UdpDetectionReceiver
    ↓
DetectionRecord
```

The C++ receiver successfully reconstructed the same live target detections previously available only through CSV replay.

### End-to-End Hardware Validation

The following image shows command integrity across the Linux host and the STM32/FreeRTOS queue boundary.

<p align="center">
  <img src="docs/images/e2e_validation.png"
       alt="End-to-end validation from Linux host to STM32 FreeRTOS controller"
       width="950">
</p>

<p align="center">
  <em>Linux transmission and STM32/FreeRTOS reception of the same TargetCommand.</em>
</p>

Single-command integrity validation:

```text
Linux TargetCommand
{ sequence=1, target_x=515, target_y=-97, prediction_ms=0 }

            ↓ UART

STM32 received_command
{ sequence=1, target_x=515, target_y=-97, prediction_ms=0 }

            ↓ FreeRTOS Queue

queue_received_command
{ sequence=1, target_x=515, target_y=-97, prediction_ms=0 }
```

Integration flags:

```text
rx_complete             = 1
queue_send_success      = 1
queue_receive_success   = 1
queue_data_match        = 1
```

Continuous live validation additionally confirmed that `received_command` and `queue_received_command` continue to update while Simulink is running.

Validation summary:

| Verification                           | Result   |
| -------------------------------------- | -------- |
| Host protocol tests                    | **PASS** |
| Synthetic sensor validation            | **PASS** |
| Simulink live target extraction        | **PASS** |
| Simulink → Linux UDP streaming         | **PASS** |
| Linux C++ UDP reception                | **PASS** |
| Continuous Linux command generation    | **PASS** |
| Linux serial transmission              | **PASS** |
| STM32 UART reception                   | **PASS** |
| UART frame validation                  | **PASS** |
| TargetCommand deserialization          | **PASS** |
| FreeRTOS queue send / receive          | **PASS** |
| Continuous command update inside STM32 | **PASS** |

Overall validated path:

```text
Simulink
→ Live UDP
→ Linux C++
→ UART
→ STM32
→ FreeRTOS Queue
→ ControlTask
```

**Result: PASS**

## Build & Run

### Build

```bash
cmake -S . -B build
cmake --build build -j
```

Run host-side tests:

```bash
ctest --test-dir build --output-on-failure
```

### Live Mode

Open:

```text
simulation/simulink/scenario_sensor_model.slx
```

Start the Linux application:

```bash
./build/pc/target_control_pc \
    /dev/ttyACM0 \
    --live
```

The application opens the serial device and UDP receiver, then waits for live detections:

```text
Input Mode       : LIVE UDP
UDP Listen       : 127.0.0.1:51001
Waiting for Simulink detections...
```

Run `scenario_sensor_model.slx`.

Each valid target detection is streamed directly to Linux and transmitted to the STM32:

```text
[LIVE] seq=1  t=0.100  pos=(5.147, -0.972)  cmd=(515, -97)  pred=0ms
[LIVE] seq=2  t=0.200  pos=(5.629, -0.920)  cmd=(563, -92)  pred=0ms
[LIVE] seq=3  t=0.300  pos=(6.209, -0.891)  cmd=(621, -89)  pred=0ms
```

### CSV Replay Mode

For deterministic replay, run the Simulink model and extract the logged detections:

```text
Run scenario_sensor_model.slx
    ↓
Run extract_target_detections
    ↓
Generate target_detections.csv
```

Generated file:

```text
simulation/simulink/target_detections.csv
```

Run:

```bash
./build/pc/target_control_pc \
    /dev/ttyACM0 \
    simulation/simulink/target_detections.csv
```

CSV replay remains available for reproducible integration and regression testing.

## Repository Structure

```text
.
├── common/
│   ├── include/protocol/        # Shared protocol definitions
│   └── src/                     # Shared protocol implementation
│
├── pc/
│   ├── include/
│   │   ├── input/
│   │   │   ├── detection_record.h
│   │   │   ├── detection_csv_reader.h
│   │   │   └── udp_detection_receiver.h
│   │   └── transport/           # Linux serial transport
│   │
│   └── src/
│       ├── detection_csv_reader.cpp
│       ├── udp_detection_receiver.cpp
│       ├── serial_port.cpp
│       └── main.cpp
│
├── simulation/
│   └── simulink/                # Driving scenario and synthetic sensor model
│
├── tests/                       # Host-side unit tests
│
└── docs/
    └── images/                  # Architecture and validation images
```

Protocol sources under `common/` are shared between the Linux host and STM32 firmware, maintaining a single source of truth for serialization, CRC, UART framing, and frame validation.

## Current Status

### Implemented

* ✅ `TargetCommand` wire format
* ✅ Serialization / deserialization
* ✅ CRC-16/CCITT-FALSE
* ✅ UART frame encoding / decoding / validation
* ✅ Linux serial transport
* ✅ Host-side unit tests
* ✅ STM32 UART reception
* ✅ FreeRTOS task scheduling
* ✅ CMSIS-RTOS2 message queue communication
* ✅ Driving scenario generation
* ✅ Synthetic vision sensor integration
* ✅ Measurement noise
* ✅ Probabilistic detection
* ✅ False-positive and missed detections
* ✅ Detection time / position / velocity extraction
* ✅ Live target actor filtering
* ✅ Simulink → Linux live UDP streaming
* ✅ Linux C++ UDP detection receiver
* ✅ Deterministic CSV replay path
* ✅ Continuous `DetectionRecord` → `TargetCommand` generation
* ✅ Sequential live command transmission
* ✅ Linux → UART → STM32 communication
* ✅ STM32 frame validation and deserialization
* ✅ `CommRxTask` → Message Queue → `ControlTask`
* ✅ Continuous Simulink → Linux → STM32 → FreeRTOS validation

### In Progress

* ⏳ End-to-end latency instrumentation
* ⏳ Latency-aware future target prediction
* ⏳ PWM / servo actuation
* ⏳ Baseline vs. latency-aware tracking-error evaluation

## Roadmap

| Phase   | Description                                   | Status        |
| ------- | --------------------------------------------- | ------------- |
| Phase 1 | Communication Protocol & RTOS Path            | ✅ Complete    |
| Phase 2 | Synthetic Sensor & Live Streaming Integration | ✅ Complete    |
| Phase 3 | End-to-End Latency Instrumentation            | ⏳ In Progress |
| Phase 4 | Latency-Aware Target Prediction               | ⏳ Planned     |
| Phase 5 | PWM / Servo Actuation                         | ⏳ Planned     |
| Phase 6 | Quantitative Baseline Comparison              | ⏳ Planned     |

The final evaluation will compare baseline tracking error against latency-aware prediction under identical target-motion and latency conditions.

## Tech Stack

### Embedded / Real-Time

* STM32F429I-DISC1
* FreeRTOS
* CMSIS-RTOS2
* STM32 HAL
* UART

### Linux / Application

* C
* C++17
* CMake
* CTest
* Ubuntu Linux
* POSIX Serial / `termios`
* POSIX UDP Socket

### Simulation

* MATLAB R2026a
* Simulink
* Automated Driving Toolbox
* Python 3.10 — UDP transport adapter
