# Latency-Aware Target Control

## Hardware-Independent Embedded Control Architecture

`latency-aware-target-control` is an embedded control integration project that explores how the same **hardware-independent control application** can be executed across heterogeneous embedded platforms.

The project originally started from a latency-aware target-control problem, but the current architecture has evolved toward a broader systems question:

> **Can the same control application be reused without modification across different embedded processors, RTOS environments, timer peripherals, and hardware APIs?**

The current reference platform is:

- **STM32F429I-DISC1**
  - Cortex-M4
  - FreeRTOS
  - CMSIS-RTOS2
  - STM32 HAL

The next portability target is:

- **Zybo Z7 / Zynq-7000 Processing System**
  - Cortex-A9
  - FreeRTOS or bare-metal runtime
  - Platform-specific peripheral backend

The central design principle is to separate:

```text
What the application wants to do

from

How each hardware platform performs it
```

The current STM32 implementation has completed the software path from synthetic perception to PWM timer command generation.

---

## Project Goal

The primary project goal is:

> Run the same Hardware-Independent Common Control Application on STM32F429 and Zynq-7000 PS without modifying the common control source code, and quantitatively verify functional and real-time equivalence.

The project therefore focuses on three boundaries:

1. **Communication boundary**
   - UART / UDP / RTOS communication must not leak into control logic.

2. **Control boundary**
   - The Common Control Application operates only on semantic target state and time.

3. **Actuator boundary**
   - Steering semantics, servo mapping, and hardware PWM generation are separated into independent layers.

The final portability target is:

```text
Same semantic input
        ↓
Same Common Control Application
        ↓
Same desired steering command
        ↓
Platform-independent actuator command
        ↓
Different hardware backends
```

---

# System Architecture

The current architecture is:

```text
MATLAB / Simulink
Synthetic Vision Detection
        ↓
Target Actor Filtering
        ↓
Live UDP
        ↓
Linux C++
DetectionRecord
        ↓
TargetCommand
        ↓
UART Frame + CRC-16
        ↓
────────────────────────────────────────
        Embedded Platform Boundary
────────────────────────────────────────
        ↓
Platform Runtime / Communication
UART RX
RTOS Task
Local Clock
        ↓
Common Protocol Layer
Frame Validation
CRC
Deserialization
        ↓
TargetState
        ↓
────────────────────────────────────────
Hardware-Independent Common Application
────────────────────────────────────────
        ↓
Steering Controller
        ↓
Desired Steering Angle [deg]
        ↓
SG90 Actuator Mapping
        ↓
PwmCommand
{ period_us, pulse_width_us }
        ↓
────────────────────────────────────────
Platform-Specific PWM Backend
────────────────────────────────────────
        ↓
STM32 TIM4_CH2 / PB7
        ↓
SG90 Servo
```

The important architectural boundary is:

```text
TargetState
    ↓
Common Control Application
    ↓
Desired Steering Angle
```

The Common Control Application does **not** depend on:

```text
UART
DMA
Interrupt
FreeRTOS
CMSIS-RTOS2
Message Queue
HAL_GetTick()
STM32 HAL
STM32 Timer APIs
Zynq BSP
PWM Registers
SG90 electrical details
```

---

# Simulation & Synthetic Perception

MATLAB/Simulink and Automated Driving Toolbox provide a controlled perception source.

The scenario contains an ego vehicle and a moving target vehicle.

`Vision Detection Generator` models a non-ideal perception sensor.

## Sensor Configuration

| Parameter | Value |
| --- | --- |
| Detection probability | `0.70` |
| False positives per image | `1.0` |
| Measurement noise | Enabled |
| Random seed | `42` |
| Sensor update interval | `0.1 s` |
| Coordinate system | Ego Cartesian |

The sensor therefore reproduces:

- Measurement noise
- Probabilistic detection
- False positives
- Missed detections

The target vehicle is identified using:

```text
targetActorId = 2
```

Detection filtering behavior:

```text
TargetIndex < 0
    → False positive
    → Ignore

TargetIndex != 2
    → Other actor
    → Ignore

TargetIndex == 2
    → Valid target
```

For each valid detection:

```text
[time, x, y, vx, vy]
```

is extracted.

If the target is not detected, **no packet is transmitted**.

Missing detection is therefore represented by:

```text
No new TargetState update
```

rather than by a synthetic zero-valued target.

---

## Simulink → Linux UDP Transport

The Simulink transport adapter sends:

```text
double × 5
```

in Little Endian order.

Packet layout:

| Field | Type | Size |
| --- | --- | ---: |
| `time_sec` | `double` | 8 bytes |
| `target_x` | `double` | 8 bytes |
| `target_y` | `double` | 8 bytes |
| `target_vx` | `double` | 8 bytes |
| `target_vy` | `double` | 8 bytes |

Total payload:

```text
40 bytes
```

UDP endpoint:

```text
Address : 127.0.0.1
Port    : 51001
```

The Python Code block inside Simulink acts only as a transport adapter.

It does not implement:

- Control logic
- Protocol logic
- Steering logic
- Actuator logic

---

# Linux Application

The Linux C++ application supports two input modes.

## Live Mode

```text
Simulink
    ↓ UDP
UdpDetectionReceiver
    ↓
DetectionRecord
```

## CSV Replay Mode

```text
CSV
    ↓
DetectionCsvReader
    ↓
DetectionRecord
```

Both input paths produce:

```cpp
struct DetectionRecord
{
    double time_sec;
    double target_x_m;
    double target_y_m;
    double target_vx_mps;
    double target_vy_mps;
};
```

This keeps the downstream protocol path independent of the perception source.

---

# TargetCommand Transport Contract

The existing UART protocol remains unchanged.

```c
typedef struct
{
    uint16_t sequence;
    int16_t target_x;
    int16_t target_y;
    uint16_t prediction_ms;
} TargetCommand;
```

Wire payload:

```text
sequence       2 bytes
target_x       2 bytes
target_y       2 bytes
prediction_ms  2 bytes
──────────────────────
Total          8 bytes
```

All multi-byte values use Little Endian encoding.

Target position is converted from meters to centimeters:

```text
5.147 m
    ↓ × 100
514.7 cm
    ↓ round
515
```

The current control architecture uses only `target_x` and `target_y`.

`sequence` remains useful protocol/debug metadata.

`prediction_ms` currently remains:

```text
0
```

and is not used by the Common Control Application.

---

# UART Frame

`TargetCommand` is transmitted inside a CRC-protected frame.

| Field | Value | Size |
| --- | --- | ---: |
| SOF1 | `0xAA` | 1 |
| SOF2 | `0x55` | 1 |
| Version | `0x01` | 1 |
| Message Type | `0x01` | 1 |
| Payload Length | `0x08` | 1 |
| Payload | `TargetCommand` | 8 |
| CRC | CRC-16/CCITT-FALSE | 2 |

Total frame size:

```text
15 bytes
```

Frame:

```text
SOF1 | SOF2 | Version | Type | Length | Payload | CRC
 AA     55      01      01      08      8 B      2 B
```

CRC configuration:

```text
Polynomial = 0x1021
Init       = 0xFFFF
RefIn      = false
RefOut     = false
XorOut     = 0x0000
```

Known test vector:

```text
"123456789" → 0x29B1
```

Serial configuration:

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

---

# Semantic TargetState Boundary

A major architectural change was introduced between the communication layer and the control application.

The UART protocol still transports:

```text
TargetCommand
```

but the Common Control Application receives only:

```c
typedef struct
{
    int16_t x_cm;
    int16_t y_cm;
} TargetState;
```

The runtime performs:

```text
TargetCommand
    ↓
Protocol Metadata Removal
    ↓
TargetState
```

Example:

```text
TargetCommand
{
    sequence      = 17,
    target_x      = 515,
    target_y      = -97,
    prediction_ms = 0
}

        ↓

TargetState
{
    x_cm = 515,
    y_cm = -97
}
```

This prevents protocol details from entering the control application.

---

# STM32 Runtime Architecture

The STM32F429 uses FreeRTOS through CMSIS-RTOS2.

The runtime contains two major tasks:

```text
CommRxTask
    ↓
targetCommandQueue
    ↓
ControlTask
```

Despite the historical queue name, the queue now stores:

```text
TargetState
```

not `TargetCommand`.

Queue configuration:

| Item | Configuration |
| --- | --- |
| Producer | `CommRxTask` |
| Consumer | `ControlTask` |
| Queue | `targetCommandQueue` |
| Item Type | `TargetState` |
| Capacity | `4` |
| API | CMSIS-RTOS2 Message Queue |

Current queue creation:

```c
targetCommandQueueHandle =
    osMessageQueueNew(
        4,
        sizeof(TargetState),
        &targetCommandQueue_attributes);
```

---

## CommRxTask

`CommRxTask` is responsible only for communication processing.

```text
UART Receive
    ↓
Frame Validation
    ↓
CRC Validation
    ↓
TargetCommand Deserialize
    ↓
TargetState Mapping
    ↓
osMessageQueuePut()
```

`CommRxTask` does **not** modify controller state.

This maintains a single-owner control-state model.

---

## ControlTask

`ControlTask` is the sole owner of the `SteeringController`.

Current control period:

```text
20 ms
```

Control frequency:

```text
50 Hz
```

The synthetic sensor updates every:

```text
100 ms
```

Therefore sensor arrival and control execution are intentionally decoupled.

The task performs:

```text
Drain available TargetState messages
        ↓
controller_update_target()
        ↓
controller_step()
        ↓
Desired Steering Angle
        ↓
SG90 Mapper
        ↓
PwmCommand
        ↓
STM32 PWM Backend
```

The controller continues executing every 20 ms even when no new target detection arrives.

---

# Hardware-Independent Steering Controller

The Common Steering Controller uses a Pure-Pursuit-inspired geometric steering calculation.

Coordinate convention:

```text
+x = Forward
+y = Left
-y = Right
-x = Behind
```

Steering convention:

```text
Positive = Left
Negative = Right
Zero     = Straight
```

The controller uses:

```text
Ld = sqrt(x² + y²)

alpha = atan2(y, x)

delta =
    atan(
        2 × L × sin(alpha)
        ──────────────────
               Ld
    )
```

Equivalent form:

```text
delta =
    atan(
        2 × L × y
        ───────────
          x² + y²
    )
```

where:

```text
L = wheelbase = 2.8 m
```

Current controller parameters:

| Parameter | Value |
| --- | ---: |
| Wheelbase | `2.8 m` |
| Steering deadband | `±1°` |
| Steering saturation | `±30°` |
| Target timeout | `400 ms` |
| Control period | `20 ms` |

---

# Invalid / Missing Target Policy

The controller distinguishes between:

```text
Protocol invalid
```

and:

```text
Semantic invalid
```

Protocol-invalid frames are discarded before reaching the Common Control Application.

Examples:

- Invalid SOF
- Invalid protocol version
- Invalid message type
- Invalid payload length
- CRC mismatch
- Deserialize failure

Semantic target validation happens inside the Common Control Application.

Current invalid target rule includes:

```text
x <= 0
```

which rejects targets at or behind the ego vehicle.

Behavior:

```text
Valid target
    ↓
Update last valid target
    ↓
Compute steering

Invalid target
    ↓
Reject update
    ↓
Keep previous valid target

No new valid target
    ↓
Hold last steering

elapsed >= 400 ms
    ↓
Neutral steering
    ↓
0°
```

The timeout policy is therefore:

```text
Reject
→ Hold Last
→ Timeout
→ Neutral
```

---

# Common Controller API

The application intentionally separates asynchronous target arrival from periodic control execution.

```c
bool steering_controller_update_target(
    SteeringController *controller,
    const TargetState *target,
    uint32_t now_ms);
```

and:

```c
float steering_controller_step(
    const SteeringController *controller,
    uint32_t now_ms);
```

Time acquisition is the responsibility of the platform runtime.

The Common Controller receives:

```text
now_ms
```

but does not call:

```text
HAL_GetTick()
osKernelGetTickCount()
xTaskGetTickCount()
```

itself.

On the current STM32 runtime:

```text
FreeRTOS tick rate = 1 kHz
```

so:

```text
1 tick = 1 ms
```

and the runtime can pass `osKernelGetTickCount()` directly as `now_ms`.

---

# Actuator Abstraction

The control application outputs only:

```text
Desired Steering Angle [deg]
```

The actuator path is separated into additional layers.

```text
Desired Steering Angle
        ↓
SG90 Mapper
        ↓
PwmCommand
        ↓
Platform PWM Backend
```

---

## PwmCommand

The generic PWM command is:

```c
typedef struct
{
    uint32_t period_us;
    uint32_t pulse_width_us;
} PwmCommand;
```

This type does not contain:

```text
STM32
TIM4
ARR
CCR
Prescaler
Zynq
SG90
```

It represents only the physical meaning of a PWM command.

---

# SG90 Mapper

The SG90 mapper converts:

```text
Steering Angle [deg]
```

into:

```text
PWM Period [us]
PWM Pulse Width [us]
```

The mapping is configurable.

```c
typedef struct
{
    float steering_limit_deg;

    uint32_t period_us;

    uint32_t pulse_at_negative_limit_us;
    uint32_t pulse_at_center_us;
    uint32_t pulse_at_positive_limit_us;
} Sg90MapperConfig;
```

The mapper supports both physical mounting directions.

For example:

```text
Negative steering
    ↓
Shorter pulse
```

or:

```text
Negative steering
    ↓
Longer pulse
```

can both be represented by configuration.

The Common Steering Controller therefore never changes because of servo installation direction.

---

## SG90 Test Fixture

Host tests currently use:

```text
-30° → 1200 us
  0° → 1500 us
+30° → 1800 us
```

These values are **test fixtures only**.

They are not yet the final calibrated SG90 runtime values.

Final values will be determined after physical servo calibration.

---

# STM32 Generic PWM Backend

The STM32 backend receives only:

```text
PwmCommand
```

and converts microseconds into timer counts.

Current PWM resource:

```text
Timer   : TIM4
Channel : CH2
GPIO    : PB7
```

Current CubeMX bring-up configuration:

```text
Prescaler      = 4
Counter Period = 24999
PWM Pulse      = 1875
PWM Mode       = PWM Mode 1
Polarity       = High
```

The configured counter frequency is:

```text
1.25 MHz
```

which means:

```text
1 timer count = 0.8 us
```

Therefore:

```text
20,000 us
    ↓
25,000 counts
    ↓
ARR = 24,999
```

and:

```text
1,500 us
    ↓
1,875 counts
    ↓
CCR2 = 1,875
```

The backend is responsible for:

```text
period_us
    ↓
ARR

pulse_width_us
    ↓
CCR
```

but does not know that the signal is controlling an SG90.

---

# Current Full STM32 Software Path

The complete validated STM32 software path is:

```text
Simulink
    ↓
Synthetic Detection
    ↓
UDP
    ↓
Linux C++
    ↓
DetectionRecord
    ↓
TargetCommand
    ↓
UART Frame + CRC-16
    ↓
STM32 UART
    ↓
CommRxTask
    ↓
Frame Validation
    ↓
TargetCommand Deserialize
    ↓
TargetState
    ↓
FreeRTOS Queue
    ↓
ControlTask
    ↓
Common Steering Controller
    ↓
desired_steering_deg
    ↓
SG90 Mapper
    ↓
PwmCommand
    ↓
STM32 PWM Backend
    ↓
TIM4_CH2 / PB7
```

---

# Deterministic Integration Validation

A deterministic CSV replay path is retained for regression testing.

Example first detection:

```text
time = 0.1 s
x    ≈ 5.147 m
y    ≈ -0.972 m
vx   ≈ 5.226 m/s
vy   ≈ 0.768 m/s
```

Linux converts this to approximately:

```text
target_x = 515 cm
target_y = -97 cm
```

The expected steering command is approximately:

```text
-11.x°
```

The STM32 runtime was observed producing the expected negative steering value.

This validates:

```text
Known TargetState
    ↓
Common Controller
    ↓
Expected Steering Sign
    ↓
Expected Steering Magnitude
```

---

# Hold / Timeout Validation

The runtime was also validated with a one-shot target update.

Observed behavior:

```text
Valid target arrives
    ↓
Steering changes
    ↓
No new target
    ↓
Previous steering remains active
    ↓
400 ms timeout
    ↓
Steering returns to 0°
```

Debug latches confirmed both:

```text
hold_before_timeout_seen = 1
timeout_neutral_seen     = 1
```

---

# Actuation-Path Validation

The complete software actuator chain was validated using STM32 debug values.

```text
desired_steering_deg
        ↓
sg90_mapper_map()
        ↓
pwm_period_us
pwm_pulse_width_us
        ↓
stm32_pwm_backend_apply()
        ↓
TIM4 CCR2
```

This confirms software propagation from:

```text
Common Control Output
```

to:

```text
STM32 Timer Compare Command
```

Physical servo motion is intentionally treated as a separate hardware-validation checkpoint.

---

# Host-Side Tests

The host test suite currently contains seven CTest targets.

Coverage includes:

### Protocol

- `TargetCommand` serialization / deserialization
- CRC-16/CCITT-FALSE
- UART frame encoding
- UART frame validation

### Linux Transport

- Serial transport behavior

### Common Steering Controller

- Center target
- Positive / negative steering
- Deadband
- Positive / negative saturation
- Invalid target rejection
- Invalid target must not replace last valid target
- Hold-last behavior
- Exact timeout-to-neutral behavior
- Invalid controller configuration

### SG90 Mapper

- Center mapping
- Positive / negative limit
- Linear interpolation
- Out-of-range clamping
- Reversed servo direction
- Invalid configuration
- Invalid arguments

Current result:

```text
7 / 7 CTest targets PASS
```

---

# Portability Validation Strategy

The STM32 implementation is the reference implementation.

The next phase will port the same Common Control Application to the Zynq-7000 Processing System.

The intended architecture is:

```text
                 Common Control Application
                           │
           ┌───────────────┴───────────────┐
           │                               │
           ↓                               ↓
     STM32 Runtime                    Zynq Runtime
           │                               │
      STM32 HAL                        Zynq BSP
           │                               │
      TIM4 / UART                    Platform PWM/UART
```

The Common Application must remain unchanged.

---

## Portability Metrics

### 1. Common Application Modified LOC

Target:

```text
STM32 → Zynq port

Modified LOC in Common Control Application = 0
```

---

### 2. Platform Dependencies

The Common Application should contain:

```text
0 references
```

to:

```text
HAL_
STM32
Zynq
FreeRTOS
CMSIS
xQueue
osMessageQueue
Timer registers
Platform-specific #ifdef
```

---

### 3. Functional Equivalence

The same `TargetState` sequence will be replayed on:

```text
Host
STM32
Zynq
```

and the resulting steering commands compared.

Metrics:

```text
Mean Absolute Error
Maximum Absolute Error
```

Expected difference should remain within floating-point implementation tolerance.

---

### 4. PWM Equivalence

Representative steering commands:

```text
-30°
-15°
  0°
+15°
+30°
```

will be converted to PWM commands.

Measured physical pulse width will later be compared with the commanded pulse width.

Metric:

```text
PWM Pulse Error
=
|Measured Pulse Width - Commanded Pulse Width|
```

Without a shaft encoder, this validates PWM signal equivalence rather than exact physical servo-angle equivalence.

---

### 5. Real-Time Execution

The Common Control Application will be measured on both embedded platforms.

Candidate metrics:

```text
controller_step() execution time
P99 execution time
Maximum execution time
Deadline miss rate
Control-loop jitter
```

The goal is not to prove that one platform is faster.

The goal is:

> Both platforms must execute the same control application within the same real-time control requirement.

---

# Repository Structure

```text
.
├── common/
│   ├── include/
│   │   ├── protocol/
│   │   │   ├── target_command.h
│   │   │   ├── target_command_codec.h
│   │   │   ├── uart_frame.h
│   │   │   ├── uart_frame_codec.h
│   │   │   ├── uart_frame_validator.h
│   │   │   └── crc16.h
│   │   │
│   │   ├── control/
│   │   │   ├── target_state.h
│   │   │   └── steering_controller.h
│   │   │
│   │   └── actuator/
│   │       ├── pwm_command.h
│   │       └── sg90_mapper.h
│   │
│   └── src/
│       ├── crc16.c
│       ├── target_command_codec.c
│       ├── uart_frame_codec.c
│       ├── uart_frame_validator.c
│       ├── steering_controller.c
│       └── sg90_mapper.c
│
├── pc/
│   ├── include/
│   │   ├── input/
│   │   │   ├── detection_record.h
│   │   │   ├── detection_csv_reader.h
│   │   │   └── udp_detection_receiver.h
│   │   └── transport/
│   │
│   └── src/
│       ├── detection_csv_reader.cpp
│       ├── udp_detection_receiver.cpp
│       ├── serial_port.cpp
│       └── main.cpp
│
├── firmware/
│   └── stm32/
│       └── target_control_stm32/
│           ├── Core/
│           │   ├── Inc/
│           │   │   ├── control/
│           │   │   ├── actuator/
│           │   │   └── platform/
│           │   │       └── stm32_pwm_backend.h
│           │   │
│           │   └── Src/
│           │       ├── main.c
│           │       ├── steering_controller.c
│           │       ├── sg90_mapper.c
│           │       └── stm32_pwm_backend.c
│           │
│           └── target_control_stm32.ioc
│
├── simulation/
│   └── simulink/
│       ├── scenario_sensor_model.slx
│       ├── constant_velocity_scenario.mat
│       ├── extract_target_detections.m
│       └── target_detections.csv
│
├── tests/
│   ├── common/
│   └── pc/
│
└── docs/
    └── images/
```

Shared Common source files are reused by the STM32 firmware through project links rather than duplicated implementations.

This maintains a single source of truth for:

```text
Protocol
Target semantics
Control algorithm
Actuator mapping
```

---

# Build & Run

## Host Build

```bash
cmake -S . -B build
cmake --build build -j
```

Run all host tests:

```bash
ctest --test-dir build --output-on-failure
```

Expected current result:

```text
100% tests passed
0 tests failed out of 7
```

---

## Live Mode

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

Then run the Simulink model.

Flow:

```text
Simulink
→ UDP
→ Linux
→ UART
→ STM32
```

---

## CSV Replay Mode

For deterministic validation:

```bash
./build/pc/target_control_pc \
    /dev/ttyACM0 \
    simulation/simulink/target_detections.csv
```

The current CSV mode is useful for:

- Regression testing
- Known-input steering verification
- STM32 debugger inspection
- Actuator-path validation

---

## STM32 Build

Open:

```text
firmware/stm32/target_control_stm32
```

in STM32CubeIDE.

Then:

```text
Build
→ Flash
→ Debug
→ Resume
```

Current STM32 timer resource:

```text
TIM4_CH2
PB7
```

---

# Current Status

## Completed

- ✅ Shared `TargetCommand` protocol
- ✅ Explicit Little Endian serialization
- ✅ CRC-16/CCITT-FALSE
- ✅ Generic UART frame encoding
- ✅ UART frame validation
- ✅ Linux POSIX serial transport
- ✅ Simulink synthetic driving scenario
- ✅ Vision Detection Generator
- ✅ Detection noise / missed detections / false positives
- ✅ Target actor filtering
- ✅ Live Simulink → Linux UDP transport
- ✅ Deterministic CSV replay
- ✅ Linux `DetectionRecord`
- ✅ Linux `TargetCommand` generation
- ✅ Linux → STM32 UART communication
- ✅ STM32 `CommRxTask`
- ✅ FreeRTOS / CMSIS-RTOS2 queue
- ✅ Semantic `TargetState` boundary
- ✅ Hardware-independent Steering Controller
- ✅ Pure-Pursuit-inspired steering
- ✅ Steering deadband
- ✅ Steering saturation
- ✅ Invalid target rejection
- ✅ Hold-last behavior
- ✅ 400 ms timeout-to-neutral
- ✅ 20 ms periodic `ControlTask`
- ✅ Same Common Controller source cross-built for STM32
- ✅ Live steering output on STM32
- ✅ Deterministic expected steering validation
- ✅ Generic `PwmCommand`
- ✅ SG90 actuator mapper
- ✅ Reversed servo-direction configuration
- ✅ STM32 generic PWM backend
- ✅ TIM4_CH2 / PB7 PWM timer bring-up
- ✅ Common Controller → Mapper → PWM Backend integration
- ✅ STM32 CCR update validation
- ✅ 7 / 7 Host CTest targets passing

---

# Pending Hardware Validation

The STM32 software reference implementation is complete through PWM timer command generation.

The remaining physical actuator validation is:

- ⏳ External regulated 5 V supply for SG90
- ⏳ SG90 physical connection
- ⏳ Neutral pulse calibration
- ⏳ Safe positive / negative pulse-limit calibration
- ⏳ Replacement of provisional SG90 pulse fixtures with measured values
- ⏳ PB7 physical waveform measurement with oscilloscope or logic analyzer
- ⏳ Physical servo movement validation

The current software test values:

```text
1200 us
1500 us
1800 us
```

must not be interpreted as final calibrated actuator requirements.

---

# Roadmap

| Phase | Description | Status |
| --- | --- | --- |
| Phase 1 | Communication Protocol | ✅ Complete |
| Phase 2 | Simulink Live Perception Integration | ✅ Complete |
| Phase 3 | Semantic TargetState Boundary | ✅ Complete |
| Phase 4 | Hardware-Independent Control Application | ✅ Complete |
| Phase 5 | STM32 Runtime Integration | ✅ Complete |
| Phase 6 | SG90 Mapping / Generic PWM Command | ✅ Complete |
| Phase 7 | STM32 PWM Backend | ✅ Complete |
| Phase 8 | STM32 Full Software Actuation Path | ✅ Complete |
| Phase 9 | Physical SG90 Calibration / Measurement | ⏳ Pending |
| Phase 10 | Zynq-7000 PS Runtime Port | ⏳ Planned |
| Phase 11 | STM32 ↔ Zynq Functional Equivalence | ⏳ Planned |
| Phase 12 | Real-Time / PWM Portability Evaluation | ⏳ Planned |

---

# Current Milestone

The current validated reference path is:

```text
Synthetic Perception
        ↓
Linux High-Level Processing
        ↓
Shared Communication Protocol
        ↓
STM32 Platform Runtime
        ↓
Semantic TargetState
        ↓
Hardware-Independent Common Controller
        ↓
Desired Steering Angle
        ↓
Hardware-Independent SG90 Mapping
        ↓
Generic PwmCommand
        ↓
STM32 Platform PWM Backend
        ↓
TIM4_CH2 / PB7
```

The next major development phase is:

```text
Zybo Z7 / Zynq-7000 PS
        ↓
Platform Runtime
        ↓
Same TargetState
        ↓
Same Common Controller
        ↓
Same Actuator Mapping
        ↓
Zynq PWM Backend
```

The central portability requirement is:

```text
Common Control Application modification
STM32 → Zynq

= 0 LOC
```

---

# Tech Stack

## Embedded / Real-Time

- STM32F429I-DISC1
- Cortex-M4
- FreeRTOS
- CMSIS-RTOS2
- STM32 HAL
- UART
- Hardware Timer / PWM

## Target Portability Platform

- Zybo Z7
- Zynq-7000 Processing System
- Cortex-A9
- FreeRTOS or bare-metal runtime

## Linux / Application

- C
- C++17
- Ubuntu Linux
- CMake
- CTest
- POSIX Serial / `termios`
- POSIX UDP Socket

## Simulation

- MATLAB R2026a
- Simulink
- Automated Driving Toolbox
- Python Code block for UDP transport

---

# Design Summary

This project is no longer centered on a single MCU implementation.

Its core engineering problem is the separation of:

```text
Application Semantics
```

from:

```text
Platform Mechanisms
```

The resulting architecture is:

```text
Platform-Specific Input Runtime
        ↓
Common Semantic Interface
        ↓
Common Control Application
        ↓
Common Actuator Semantics
        ↓
Platform-Specific Output Backend
```

The STM32F429 implementation now serves as the reference platform.

The next step is to reproduce the same behavior on the Zynq-7000 Processing System while preserving the Common Control Application without source modification.

<!-- Updated from the previous project README supplied in this conversation. :contentReference[oaicite:0]{index=0} -->