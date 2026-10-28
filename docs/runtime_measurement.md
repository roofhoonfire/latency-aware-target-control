# Live Zynq input and runtime comparison

## Status and scope

Source implementation added; ARM firmware builds, live UART reception, and board
timing measurements are **pending**. Host checks do not establish board operation.
The common controller and SG90 mapper implementations remain unchanged.

Both runtimes retain this structure:

```
Simulink -> UDP -> existing Linux C++ sender -> UART 115200/8N1
        -> UART IRQ -> byte queue -> receive task -> TargetState queue -> periodic control task
        -> common controller -> SG90 mapper -> PwmCommand
STM32 only: PwmCommand -> STM32 timer backend -> output pin
```

The receive task owns stream framing and protocol validation. The control task
alone owns controller state and writes the runtime probe. The four-entry queue
carries only TargetState; no instrumentation enters the controller interface.
Both runtimes drain at most four entries per control cycle, reject a new enqueue
when full, and count drops. This replaces STM32's previous blocking enqueue and
fixed 15-byte read with nonblocking enqueue and a resynchronizing byte stream.
`rejected_windows` counts rejected sliding windows, **not corrupted frames**.
Missing bytes/noise need not permanently misalign all subsequent frames.
Both boards now receive through interrupts; the receive task blocks on a separate
128-entry byte queue. Known byte loss is tagged by an epoch, so parsing restarts
at the discontinuity. See [IRQ build/validation procedure](uart_interrupt_validation.md).

## Build and live input

1. On the original Linux development machine, open the existing CubeIDE and
   Vitis projects. Verify Git symlinks are real links: Zynq `src/common_include`,
   `src/steering_controller.c`, and `src/sg90_mapper.c`. This Windows checkout
   contains Git symlink placeholder files; it is not an ARM build validation.
2. In Vitis, refresh sources so the added `protocol_sources.c` (CRC),
   `target_command_codec.c`, and `uart_frame_validator.c` are compiled as separate
   translation units. Also build `zynq_uart_rx.c` (Zynq) and `stm32_uart_rx.c`
   (STM32 Core/Src). They include repository common sources rather than copies.
   Do not separately add the same protocol implementations to the build.
3. Build both firmwares; inspect linker map and memory use. Check task stack
   high-water marks before long runs. STM32 retains its existing 512-byte task
   stacks; Zynq task stack sizes are 1024 FreeRTOS stack words.
4. Flash/launch Zynq; verify PS UART1 routing to the board's USB UART and select
   the correct serial device. The configured BSP exposes `XPAR_XUARTPS_0`.
5. Use the existing sender, replacing the serial-device path as appropriate:

   ```sh
   ./build/pc/target_control_pc /dev/ttyUSB1 --live
   ```

6. Run the existing Simulink model. Verify `received_frames`, `accepted_targets`,
   target coordinates and steering/PWM trace values change. Stop the sender and
   inspect the transition to neutral after the configured 400 ms timeout. Send
   bad CRC/noise then a valid frame and confirm recovery.

Do not open another receiver on the same host serial port while sending. Zynq
prints a startup message only; continuous timing-path UART logging is disabled.
There is no task-side UART polling. STM32 handles RXNE/errors in its USART1 ISR;
Zynq handles FIFO/error/timeout interrupts, draining at most 64 bytes per ISR.
The byte queue wakes the receive task. `dropped_targets` counts full semantic
queues; `uart_rx_stats.queue_drops` counts dropped bytes, and `overrun_events`
counts observed hardware overrun events. These are different loss mechanisms.
Saturated UART throughput is not established; qualify the intended input rate.

## What PWM verification means

Three distinct observations must be reported separately:

1. **Command calculation:** mapper produces period/pulse widths in microseconds.
   Both runtimes implement this; the new Zynq live path still needs board testing.
2. **Peripheral programming:** STM32 backend writes ARR/CCR and starts PWM.
   Existing debugger evidence concerns this software path. Zynq has no PWM backend.
3. **Physical output:** measure period, high time and voltage on the actual pin
   with an oscilloscope/logic analyzer. This is pending. A register value does not
   prove pin routing, timer-clock accuracy, or calibrated servo position.

No new Zynq PL logic or physical actuator driver is introduced by this change.

## Timing definitions

| Field | Measurement boundary |
| --- | --- |
| common_us | controller step + SG90 mapper only; excludes target update, queue handling, PWM backend and statistics |
| work_us | control-cycle work from before queue draining to after platform work; STM32 includes PWM/backend/debug latches, Zynq has no backend |
| release_error_us | signed actual start minus periodic high-resolution reference; reference phase set at first release after tick alignment |
| deadline_misses | observed application-work completion later than reference + 20 ms; excludes probe bookkeeping itself |

Use **common_us** for comparable code boundaries. Do not call the different
work_us boundaries a processor speed comparison. Durations include interrupt and
task preemption; they are response-time observations, not isolated CPU-cycle
cost or a WCET bound. Probe bookkeeping and clock reads perturb execution.

STM32 uses DWT CYCCNT / SystemCoreClock; Zynq uses the 64-bit global timer /
COUNTS_PER_SECOND. Confirm both counters advance on hardware. STM32 DWT wraps;
unsigned short-interval subtraction handles normal wrap. Signed phase error is
valid only within half a counter wrap (roughly 12 seconds at 180 MHz); reset the
board after debugger pauses/very long stalls. Do not change clocks mid-run.
Integer microsecond durations truncate: 0 means less than 1 us at this resolution,
not no work. Time acquisition is outside the timed common pipeline on both boards.

RTOS time is explicitly converted to milliseconds. Default ticks differ: STM32
1000 Hz, Zynq 100 Hz. Both request 20 ms, exactly representable with these defaults.
The Zynq release and timeout quantization is still coarser. Keep OS tick settings
documented; differing OS/cache/compiler settings are part of the measured system.

## Reproducible measurement procedure

1. Record commit, board/CPU, actual core/timer frequency, compiler/version and
   optimization flags, FPU/ABI, cache state, tick rate, priorities and input rate.
   Match optimization level and application configuration. No breakpoints,
   stepping, semihosting or periodic debug polling while timing.
2. Validate functional output first. For comparison runs, replay the **same input
   sequence and pacing** through the existing UDP receiver, separately per board.
   Keep live input for the integration demonstration. The existing C++ CSV mode
   transmits only its first row; it is not full timed replay. A fixed-seed Simulink
   restart alone still requires checking the delivered sequence.
   A full recorded sequence can now be replayed through that same UDP path:

   ```sh
   ./build/pc/target_control_pc /dev/ttyUSB1 --live
   # In another terminal on the sender host, with Simulink UDP stopped:
   python3 tools/replay_detections.py simulation/simulink/target_detections.csv
   ```

   Repeat the same file separately with the STM32 serial port. The helper preserves
   CSV timing gaps, but host scheduling and arrival phase still vary. A file shorter
   than the measurement run leads to timeout/idle samples; use an appropriately
   long recording and label the retained window.
3. Start from board reset each run. Use equal run durations (e.g. 30 seconds),
   repeat at least three times, and halt once at the end. Run no-input, live/replay,
   invalid-input and disconnect/timeout cases separately. Confirm both queues
   have zero drops and hardware error counters remain zero in nominal runs.
   Do not merge idle/timeout timing with active-target timing without labeling it.
4. Halt the processor; source `tools/export_runtime_probe.gdb` from the debugger's
   GDB console (IDE may require the `source` command in its debugger console).
   The script writes `runtime_trace.csv` on the debugger host. Rename/copy it to
   `stm32.csv` or `zynq.csv` before exporting the other board. The relative output
   path is the debugger process working directory; change `set logging file` to
   an explicit writable path if needed. GDB logging syntax may need adapting to
   the installed debugger version; the script does not require GDB Python.
5. The debugger must expose complete `RuntimeProbe` debug type information.
   If IDE export is preferred, use the same CSV column names and metadata fields.
   Never export a running probe: 64-bit totals and trace entries are not atomic.
6. Generate the deliverables:

   ```sh
   python3 tools/compare_runtime.py stm32.csv zynq.csv --output results/run1
   ```

   Outputs: standalone `report.html`, shareable `comparison.svg`, numeric
   `summary.csv`, and `summary.json`. There are no fabricated example timings.
   The report does not automatically establish input/output equivalence.

The ring retains **128 cycles (about 2.56 s)**, in chronological order on export.
Whole-run min/max/totals/deadline counts cover all cycles since reset; the ring
mean describes only the retained window. Report both sample counts. Observed
maximum is not a guaranteed worst-case bound. Reset before each independent run;
never zero the probe while its writer is running. Counters are not intended for
multi-year continuous operation.

## Presentation claim after measurement

Before hardware testing: "Implemented live-input integration and a common
measurement/export procedure while preserving the receive/queue/control split."

After builds, functional checks and repeated board measurements: "Separated
platform dependencies from the common control logic and compared observed
execution time and scheduling behavior under documented matching workloads."

Hardware independence does not make platform performance equal. Retain the
distinction between common-source portability, observed timing, and physical PWM.
