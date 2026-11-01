# RTOS timing contract and qualification — v2

Status: engineering design and host regression checks. **No new ARM build,
board timing measurement, PWM waveform measurement, or WCET proof is claimed.**
See `runtime_measurement.md` for hardware setup and trace export. Earlier
same-priority descriptions describe the v1 baseline, not the v2 source.

## Requirements and rationale

| Item | v2 requirement / scope |
| --- | --- |
| Control period | 20 ms, preserving the existing 50 Hz software schedule |
| Output deadline | Computation and, on STM32, PWM register application complete within 20 ms of the scheduled release |
| Release error budget | Absolute observed start error <= 1 ms |
| Busy interval budget | <= 5 ms from task start through trace recording; excludes final counter accounting and scheduling calls |
| Normal input qualification | 10 frames/s, then 100 evenly spaced frames/s; separately four-frame bursts every 40 ms |
| Nominal acceptance | No byte/target drops, hardware RX errors, missed output deadlines, busy budget violations, or skipped releases |
| Overload/fault runs | Loss is reported and may be allowed explicitly; timing limits remain in force |

The 20 ms deadline is a **software contract chosen from the existing period**,
not a proven vehicle dynamics/stability requirement. The 1 ms and 5 ms values
are engineering allocations to be qualified, not measurements. A physical
control requirement may require revising all three. Zynq has no PWM backend:
its deadline ends at command generation, not physical actuation.

## Why receive service constrains control execution

115200 baud, 8N1 uses 10 bits per byte: 11520 bytes/s. A 15-byte frame occupies
about 1.302 ms on the wire. The 128-item byte queue stores at most 128 bytes of
payload (each queue item also contains an epoch).

* From empty, line-rate arrivals fill 128 entries in about **11.11 ms**.
* A 20 ms interval admits about **231 bytes**, exceeding that queue.
* A 5 ms interval admits at most **58 bytes** under a line-rate arrival bound.
* If the initial backlog is at most 64 bytes, 5 ms with no receive-task service
  leaves at most **122/128** entries, six entries of headroom.

These are conditional queue bounds, not proof that the initial backlog or service
gap actually satisfies the assumptions. Include trace/accounting tail, timer
daemon execution, interrupt interference and any other higher-priority work in
the real receive service gap. The remaining six bytes correspond to only about
0.52 ms at saturation. Sustained service must also drain faster than arrivals.
The observed busy interval alone is insufficient to establish that bound.

STM32 must additionally service its UART hardware in time: byte queue capacity
does not protect against a UART overrun before the ISR reads the data register.
At line rate the byte interval is about **86.8 us**; masked interrupts and ISR
latency need their own bound/measurement. Zynq has a different FIFO/IRQ path.

The semantic queue holds four targets, drained at most four times per 20 ms:
its theoretical processing ceiling is **200 targets/s**, while the UART can
carry **768 frames/s**. Sustained line saturation therefore cannot promise
lossless target delivery. Saturation is an overload test, not a nominal workload.

## Scheduling changes

* STM32: Control `osPriorityAboveNormal`, receive `osPriorityNormal`.
* Zynq: Control `tskIDLE_PRIORITY + 2`, receive `tskIDLE_PRIORITY + 1`.
* The receive task blocks on an ISR-fed byte queue when empty. The control task
  never waits for target arrival and processes at most four queued targets.
* Both control loops retain absolute periodic releases. After an overrun, select
  the next strictly future release, count skipped releases and preserve phase.
  Do not execute a backlog of expired jobs consecutively and starve receive work.
* UART IRQ priority is unchanged. Task priority does not override interrupt
  priority. Tick rates remain STM32 1000 Hz / Zynq 100 Hz; 20 ms is representable
  on both, but their time quantization is different.

For a controlled A/B build, override `LATC_CONTROL_TASK_PRIORITY` with
`osPriorityNormal` (STM32) or `1` (Zynq), retaining the same v2 instrumentation
and input. Do not compare old uninstrumented output against new timing data.
Record the exact compiler, optimization, clocks, commit and priority metadata.

## Guarantee argument still to discharge on hardware

For each defined workload, bound the worst-case response time from scheduled
release to application output: task computation + release delay + interrupt
interference + preemption + blocking must fit the 20 ms deadline. Bound the
receive service gap and sustained receive processing capacity separately.
The common float math library, critical sections, scheduler/timer daemon, cache
and memory configuration are part of this analysis. Observed maxima from finite
tests are evidence, not proven WCET or a guarantee for arbitrary traffic.

The current overrun policy preserves scheduler recovery and records failure; it
does not make a missed deadline acceptable or guarantee safe physical behavior.
No automatic neutral-on-timing-fault policy has been validated for a real plant.

## Qualification procedure

1. Build both ARM projects with real common-source symlinks and verify stacks,
   clocks and generated BSP settings. The Windows host checks cannot do this.
2. For each board run nominal 10 Hz, 100 Hz, and four-frame/40 ms burst inputs,
   at least 30 seconds each, three runs per condition. Use an external monotonic
   paced sender; verify actual delivered workload. Simulink time is not wall time.
3. Also run no input, input stop/timeout, malformed frames, and sustained UART
   load as separately labelled cases. Never run two senders on one UART.
4. Record trace counters without breakpoints or printf in the periodic path.
   Halt once after the run and export with `tools/export_runtime_probe.gdb`.
   Compare equal-priority and control-higher builds with the same workloads.
5. Run `python tools/check_realtime_trace.py actual_board_trace.csv`.
   Use `--allow-input-loss` only for deliberately overloaded/fault runs. Missing
   v2 metadata fails validation. The default minimum is 1500 executed cycles.
   Capture received frame counts and workload evidence; an idle run passing the
   timing checker does not qualify loaded operation.
6. Inspect byte drops, target drops, UART overruns and functional output as well
   as timing. Measure the actual PWM waveform separately on STM32. A register
   application deadline is not the time the servo physically responds.

`deadline_misses` measures output-path completion before trace bookkeeping.
`busy_max_us` includes trace recording; its short final accounting/scheduling
tail must be covered by separate margin. `skipped_releases` catches schedules
that were not executed, even if executed-cycle statistics appear acceptable.
Release error is relative to the first observed release phase, not an external
clock or the exact hardware tick interrupt edge. Initial dispatch latency is
not measured by that phase reference.

Example Linux traffic commands (run separately, with the normal sender stopped):

```sh
python3 tools/uart_rx_exercise.py --port /dev/ttyUSB1 --case normal --count 300
python3 tools/uart_rx_exercise.py --port /dev/ttyUSB1 --case rate100 --count 3000
python3 tools/uart_rx_exercise.py --port /dev/ttyUSB1 --case burst4 --count 750
python3 tools/uart_rx_exercise.py --port /dev/ttyUSB1 --case burst --count 23040
```

The first three request about 30 seconds of nominal traffic, the last about
30 seconds of wire saturation. Absolute host deadlines avoid accumulating write
time into every interval; OS scheduling can still alter the actual waveform.
Halt after at least 1500 executed control cycles and record sender duration and
frame counts separately. Inspect idle/timeout samples at the beginning/end.

## Interview wording before hardware qualification

“가장 고민한 부분은 제어의 마감시간과 수신 버퍼의 처리 한계를 함께
맞추는 일이었습니다. 20ms 제어 주기만 보면 충분해 보이지만, UART
115200bps에서는 128항목 바이트 큐가 약 11.1ms 만에 찰 수 있다는
제약을 계산했습니다. 그래서 제어 우선순위를 높이면서 5ms 점유 예산을
따로 정하고, 초과 시 밀린 주기를 연속 실행하지 않도록 했습니다.
호스트에서는 주기 건너뛰기와 시간 예산 판정의 경계 동작을 시험했습니다.
현재 수치는 설계 예산과 계산 결과이며, 두 보드의 실제 마감시간 준수는
동일 부하 조건의 측정으로 확인해야 합니다.”

This records work done in this change. Do not present it as an earlier measured
optimization or invent before/after jitter, WCET, CPU usage, or zero-miss results.

### 면접 답변 — 현재 완료 범위

1. **왜 RTOS를 사용했나요?** 비동기 수신과 주기 제어를 태스크로 분리하고,
   수신 대기와 제어의 예정 실행 시각을 각각 관리하기 위해 사용했습니다.
   큐로 데이터를 전달하고 제어에 더 높은 우선순위를 부여해 실행 정책을
   명시했습니다. 실제 시간 요구 충족은 분석과 측정으로 확인해야 합니다.
2. **마감시간은 어떤 기준인가요?** 기존 제어 주기가 20ms이므로 이번 작업의
   소프트웨어 출력 마감시간도 20ms로 정의했습니다. 물리 제어 안정성을
   입증한 수치는 아닙니다. 수신 큐의 용량까지 고려해 제어 작업의 점유
   예산은 별도로 5ms로 정했습니다.
3. **우선순위는 어떻게 정했나요?** 두 태스크의 동일 우선순위를 변경해
   제어를 수신보다 높게 배치했습니다. 수신은 바이트 큐에서 대기하고,
   제어는 목표 큐를 기다리지 않고 최대 4개만 처리합니다. 초과 지연 시
   지난 주기를 연속 실행하지 않고 다음 미래 주기로 넘어가도록 했습니다.
4. **어려웠던 점은 무엇인가요?** 제어 마감시간만 만족하면 수신도 괜찮을
   것이라고 볼 수 없다는 점입니다. 128바이트 큐가 최대 선로 속도에서
   약 11.1ms 만에 찬다는 제약을 계산하고, 제어 응답시간과 수신 처리량을
   함께 검토했습니다. 제어 우선순위를 높이는 결정에도 수신 지연의
   비용이 있으므로 시간 예산과 초과 계측을 함께 추가했습니다.
5. **검증했고 실시간성을 보장하나요?** 호스트에서 마감시간 경계, 예산
   초과 판정, 지연된 주기 건너뛰기와 tick 순환을 시험했습니다. 실측 로그의
   판정 도구와 부하 생성 도구도 준비했습니다. 보드에서 실시간성을
   보장했다고 말할 단계는 아니며, 지정 부하에서 실행시간·시작 편차·누락·
   수신 손실을 측정하고 최악 응답시간 분석으로 보완해야 합니다.
