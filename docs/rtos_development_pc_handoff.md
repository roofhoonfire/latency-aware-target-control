# 기존 개발 PC 인계: RTOS 우선순위·수신 한계·마감시간 실측

작성: 2026-11-01, 한국 시간

## 1. 이 문서를 받은 작업자에게

이 프로젝트의 코드와 실제 보드 환경을 확인하고, **STM32부터 실제 마감시간 준수를 측정**해 주세요. 사용자는 코드 수정, 시험, 문제 분석에 권한을 부여했습니다. 기존 작업을 보존하면서 진행하고, 연결 장치나 기존 개발 환경에 관한 꼭 필요한 정보만 사용자에게 질문하세요.

최종 목적은 다음 면접 질문에 실제 근거로 답하는 것입니다.

1. 왜 RTOS를 사용했는가?
2. 제어 마감시간은 어떻게 정했고, 어떤 조건에서 만족했는가?
3. 수신·제어 태스크의 우선순위를 어떻게 결정했는가?
4. 개발 중 가장 어려웠던 제약과 해결 과정은 무엇인가?
5. 무엇을 어떻게 검증했으며, 결과의 적용 범위는 어디까지인가?

**설계 예산, 계산 결과, 호스트 시험 결과, 실제 보드 측정값을 반드시 구분하세요. 아직 보드 실측 결과는 없습니다.** 시험 결과가 기대와 다르면 원인을 분석하고 수정하세요. 개선 전후 차이가 없으면 그대로 보고하고, 이야기를 만들기 위해 실패나 개선 수치를 만들어서는 안 됩니다.

## 2. 저장소와 전달 파일

- Windows 작업 경로: `C:\Github\latency-aware-target-control`
- 작업 시작 기준 HEAD: `1463bcdae8f090c3e0a4caf2d0a69ae81fdee4d5`
- 기존 모델에 기록된 Linux 경로: `/home/user/Desktop/latency-aware-target-control` — 실제 경로를 확인하세요.
- 현재 변경은 **미커밋 상태**입니다. Git pull만으로 이 작업이 전달되지 않습니다.
- 함께 전달하는 `rtos_handoff_bundle.zip`에는 다음이 들어 있습니다.
  - `handoff.md`: 이 문서
  - `changes.patch`: 추적 중인 파일의 미커밋 변경
  - `snapshot/`: 변경·추가된 파일의 현재 내용
  - `manifest.json`: 기준 커밋, 파일 종류, 스냅샷 SHA-256
- 원래부터 존재했던 미추적 `docs/interview_chatgpt_handoff.md`는 이번 코드 변경 파일이 아니므로 번들에 포함하지 않았습니다.

기존 PC 저장소에서 먼저 `git status --short`와 `git rev-parse HEAD`를 확인하세요. 기존 수정사항을 덮어쓰거나 reset하지 마세요. 압축은 저장소와 별도 디렉터리에 풀고, 다음처럼 적용 가능성을 점검합니다.

```sh
git apply --check /압축을푼경로/changes.patch
```

검사를 통과하고 기존 작업과 충돌하지 않으면 패치를 적용합니다. `manifest.json`에서 `kind: new`인 파일은 `snapshot/`의 상대 경로대로 추가하세요. 이미 같은 경로가 있으면 내용을 비교·병합해야 합니다. 패치가 적용되지 않으면 `snapshot/`을 최종 구현 참고본으로 사용해 기존 PC의 변경과 병합하세요. 스냅샷으로 저장소 전체를 덮어쓰지 마세요.

## 3. 프로젝트 구성과 검증 범위

```text
Simulink 모의 탐지 → UDP → Linux C++ → UART 115200/8N1
 → 보드 UART ISR → 바이트 큐 → 수신 태스크의 프레임 처리
 → TargetState 큐 → 주기 제어 태스크 → 공통 제어기 → SG90 매퍼
 → STM32: PWM 백엔드에서 타이머 레지스터 적용
 → Zynq: PWM 명령 계산까지, 물리 PWM 백엔드 없음
```

- 공통 제어기와 매퍼는 `common/src/steering_controller.c`, `common/src/sg90_mapper.c`입니다. 이번 작업에서는 이 두 소스를 변경하지 않았습니다.
- 이전 보드 검증은 고정 좌표 `{515, -97}`에 대한 조향각 약 `-11.188°`, PWM 명령 `20000μs / 1388μs` 비교입니다. 최신 IRQ/스케줄링 코드의 실측 증거가 아닙니다.
- 목표 timeout은 400ms이며, 제어 태스크가 유효 목표를 반영한 시각이 기준입니다. 센서 측정 이후의 전송 지연을 검사하는 기능은 아닙니다.
- Simulink는 이번 타이밍 측정의 필수 요소가 아닙니다. UART 시험 도구로 입력을 공급할 수 있습니다.
- 실제 모터·PWM 파형 검증도 아직 미완료입니다. 이번에는 우선 소프트웨어 출력 마감시간을 측정합니다.

## 4. 이번에 반영한 변경

### Linux UDP 시각 필터

- `pc/include/input/detection_time_filter.h`
- `pc/src/main.cpp`의 `run_live_mode()`
- 마지막 채택한 `time_sec`보다 큰 샘플만 UART 프레임 생성으로 넘깁니다.
- 과거·동일 시각 샘플을 버리며, 버린 샘플은 기준 시각과 UART 순번을 갱신하지 않습니다.
- 한 프로그램 실행이 한 시뮬레이션 실행에 대응합니다. 시뮬레이션 재시작 시 Linux 프로그램도 재시작해야 합니다.
- 이는 순서 역전·중복 필터이며, 데이터의 실제 나이를 보장하지 않습니다.

### RTOS 스케줄링 v2

| 항목 | STM32 | Zynq |
| --- | --- | --- |
| 제어 태스크 우선순위 | `osPriorityAboveNormal` | `tskIDLE_PRIORITY + 2` |
| 수신 태스크 우선순위 | `osPriorityNormal` | `tskIDLE_PRIORITY + 1` |
| 주기 | 20ms | 20ms |
| 기존 RTOS tick | 1000Hz | 100Hz |

- 수신 태스크는 바이트 큐가 비면 블록됩니다. UART IRQ가 바이트를 수집합니다.
- 제어 태스크는 목표 큐를 기다리지 않고, 한 주기에 최대 4개를 처리합니다.
- `common/include/diagnostics/runtime_schedule.h`의 `runtime_next_release()`가 늦어진 주기를 건너뛰고 다음 미래 실행 시점을 선택합니다. 지난 작업을 연속 실행해 수신 태스크가 계속 밀리는 상황을 피하기 위한 정책입니다.
- `runtime_probe.h`에 `busy_max_us`, `busy_budget_misses`, `skipped_releases`를 추가했습니다.
- 두 보드 `main.c`에 우선순위·예산·버전 메타데이터를 추가했습니다.
- STM32 `.ioc`의 ControlTask 우선순위를 32로 수정했고, `.cproject`에 공통 include 경로를 추가했습니다. CubeMX 재생성 후에도 실제 생성 코드와 설정을 확인하세요.
- `tools/export_runtime_probe.gdb`에 새 메타데이터를 추가했습니다.
- `tools/check_realtime_trace.py`는 실제 보드 export를 기준으로 예산 초과를 판정합니다. 누락 메타데이터를 성공으로 취급하지 않습니다.
- `tools/uart_rx_exercise.py`에 `rate100`, `burst4` 입력을 추가하고 송신 간격을 누적 절대 시각 기준으로 바꿨습니다.

## 5. 시간 요구사항과 수치의 의미

| 수치 | 의미 / 근거 |
| --- | --- |
| 20ms 주기·출력 마감시간 | 기존 50Hz 제어 주기를 기준으로 정한 소프트웨어 계약. 물리 제어 안정성으로 도출한 수치는 아님 |
| 시작 시점 편차 1ms 이하 | 첫 관측 실행 시각을 기준으로 한 위상 편차의 설계 예산. 초기 dispatch 지연을 직접 측정하지 않음 |
| busy 구간 5ms 이하 | 수신 태스크에 CPU 시간을 남기기 위한 설계 예산. 실제 측정값이나 강제 선점 한도가 아님 |
| 11,520 byte/s | 115200bps ÷ 8N1의 바이트당 10bit |
| 128항목 ≈ 11.11ms | 빈 바이트 큐가 최대 선로 속도에서 채워지는 시간 |
| 5ms 동안 최대 58바이트 | 최대 선로 속도의 도착량 계산 |
| 초기 backlog ≤64일 때 최대 122항목 | 64+58. **초기 backlog와 실제 수신 비실행 구간을 만족한다는 조건부 계산** |
| 최대 200목표/s | 주기당 최대 4개 ÷ 20ms. 실제 보장 처리량이 아닌 구조상 상한 |
| UART 최대 768프레임/s | 11520 ÷ 프레임 크기 15byte. 따라서 지속 포화를 무손실 정상 조건으로 주장할 수 없음 |

5ms는 코드가 강제로 작업을 중단하는 제한이 아닙니다. 초과를 계측하는 기준입니다. 실제 CPU 점유와 수신 서비스 간격, ISR 지연을 함께 확인해야 합니다. STM32 UART 하드웨어의 오버런은 소프트웨어 바이트 큐가 커도 발생할 수 있습니다.

자세한 계산과 적용 조건은 `docs/realtime_contract.md`를 읽으세요.

## 6. 완료된 검증과 아직 안 한 검증

Windows에서 `tools/run_host_checks.cmd`가 통과했습니다.

- 공통 제어기·매퍼·프로토콜 기존 시험
- UDP 시각 필터의 과거·중복·비정상 시각·새 실행 처리
- 마감시간 경계, busy 예산 경계, 주기 건너뛰기, tick 순환
- UART IRQ 대역 시험 — 실제 MCU의 IRQ 지연/전기적 동작을 검증한 것은 아님
- Python 도구 시험 14개 통과 — 합성 fixture로 판정 로직을 시험한 결과
- STM32 프로젝트 XML과 `.ioc` 우선순위 확인

**미완료:** 최신 ARM 펌웨어 빌드, 실제 보드 실행, 시간·수신 부하 실측, 물리 PWM 파형 확인, 최악 응답시간 보장 증명.

Windows에서는 COM1만 확인됐고 개발 보드 연결과 ARM 빌드 도구를 확보하지 못했습니다. 기존 개발 PC에서 먼저 장치·도구를 확인하세요.

## 7. 가장 작은 구성으로 시작하기

1. STM32 한 대부터 진행합니다. 기존 STM32CubeIDE 프로젝트와 ST-LINK를 사용합니다.
2. UART 입력 부하 시험에는 기존 UART 연결을 재사용합니다. 현재 코드의 대상은 **USART1**입니다. ST-LINK 가상 COM 포트가 자동으로 USART1에 연결된다고 가정하지 말고 `MX_USART1_UART_Init()`과 MSP 핀 설정, 실제 배선을 확인하세요.
3. 서보·오실로스코프·Simulink 없이도 소프트웨어 타이밍 시험은 가능합니다.
4. 필요하면 내부 목표를 공급하는 별도 시험 모드를 추가할 수 있습니다. **이 모드는 아직 구현하지 않았습니다.** 그 결과는 통신 부하가 없는 시험으로 구분해야 합니다.
5. 단일 보드에서 측정 경로가 확인되면 동일 절차를 Zynq로 확장합니다.

## 8. 빌드 전 확인

- Git 심볼릭 링크를 확인하세요. Windows 체크아웃의 공통 소스 링크는 경로 문자열을 담은 placeholder로 보입니다. 기존 Linux의 정상 링크를 유지하고 Windows 디렉터리를 통째로 덮어쓰지 마세요.
- STM32의 공통 include, UART IRQ 소스 포함, 중복 IRQ 핸들러 여부를 확인합니다.
- Zynq는 `src/common_include` 링크와 공통 소스 링크, `zynq_uart_rx.c`, 프로토콜 소스 포함을 확인합니다.
- 두 보드의 초기화, 클록 값, compiler 최적화, RTOS tick, task stack/heap 여유를 기록합니다.
- STM32의 `DWT->CYCCNT`, Zynq의 `XTime_GetTime()` 카운터가 실제로 증가하는지 확인합니다.
- ARM 빌드 오류는 실제 환경에서 고치고 수정 내역을 남깁니다. 아직 이 소스가 ARM 빌드까지 통과했다고 전제하지 마세요.

Linux 호스트 시험은 기존 프로젝트가 구성되는 경우 다음처럼 실행할 수 있습니다.

```sh
cmake -S . -B build/handoff-host -DBUILD_TESTING=ON
cmake --build build/handoff-host -j
ctest --test-dir build/handoff-host --output-on-failure
python3 -m unittest discover -s tests/tools -v
```

## 9. 실제 측정 순서

먼저 낮은 입력 부하로 프레임 수신, 좌표와 조향각 변경, timeout 복귀를 확인하세요. 이후 동일 부하에서 같은 우선순위(A)와 제어 우선(B)을 비교합니다. **둘 다 v2 계측 코드를 사용**해야 합니다.

- A 설정: `LATC_CONTROL_TASK_PRIORITY=osPriorityNormal`(STM32), `=1`(Zynq)
- B 설정: 현재 기본값
- 우선순위를 바꿀 때마다 clean rebuild와 실제 export 메타데이터 확인
- 각 조건 최소 30초, 3회 반복을 기본으로 하되 최초 실행 확인은 짧게 수행 가능
- 테스트마다 reset하여 전체 실행 누적 통계를 초기화하고, 시작/종료의 idle 구간과 부하 구간을 구분
- 실행 중 breakpoint·single stepping·주기적 debugger polling·printf로 타이밍을 교란하지 말 것

아래 포트는 예시입니다. 실제 포트로 바꾸고 기존 Linux 송신기/터미널 등 다른 UART 사용 프로그램은 중지하세요.

```sh
# 정상 10Hz, 약 30초
python3 tools/uart_rx_exercise.py --port /dev/ttyUSB1 --case normal --count 300
# 100Hz, 약 30초
python3 tools/uart_rx_exercise.py --port /dev/ttyUSB1 --case rate100 --count 3000
# 40ms마다 4프레임, 약 30초
python3 tools/uart_rx_exercise.py --port /dev/ttyUSB1 --case burst4 --count 750
# 지속 선로 포화: 정상 무손실 요구와 별도의 과부하 시험
python3 tools/uart_rx_exercise.py --port /dev/ttyUSB1 --case burst --count 23040
```

추가로 무입력, 입력 중단, 잘못된 CRC/절단 프레임/잡음에서 복구를 시험합니다. 호스트 sleep만으로 정확한 입력 부하를 보장하지 말고 송신 시간·실제 프레임 수·보드 카운터를 함께 기록하세요.

## 10. 로그 추출·판정

실행 종료 후 보드를 한 번 halt하고 GDB 콘솔에서 export 스크립트를 실행합니다. 첫 시도에는 `tools/export_runtime_probe.gdb`의 실제 logging 경로와 GDB 버전 문법을 확인하세요.

```text
source /실제저장소경로/tools/export_runtime_probe.gdb
```

결과 `runtime_trace.csv`는 즉시 다음처럼 조건별로 보존하세요.

```text
results/rtos/stm32/equal/normal/run1.csv
results/rtos/stm32/control_higher/normal/run1.csv
results/rtos/stm32/control_higher/rate100/run1.csv
results/rtos/stm32/control_higher/burst4/run1.csv
results/rtos/stm32/control_higher/saturation/run1.csv
```

각 실행에 대해 도구 버전, 빌드 설정, 보드 클록, tick, 우선순위, 입력 조건, 실행 시간, firmware ELF와 소스 버전을 기록하세요.

```sh
python3 tools/check_realtime_trace.py 실제정상시험.csv
python3 tools/check_realtime_trace.py 실제과부하시험.csv --allow-input-loss
```

- 기본 최소 실행 주기 수는 1500입니다. 짧은 최초 확인은 `--min-samples`를 낮출 수 있지만 정식 30초 결과로 설명하지 마세요.
- A(동일 우선순위)는 v2의 우선순위 조건 때문에 판정상 실패할 수 있습니다. 그 설정 실패와 실제 timing miss를 구분해 원시 수치를 비교하세요.
- 과부하 옵션은 입력 손실만 허용합니다. 마감시간·busy 예산·시작 편차·누락 주기 위반을 통과시키지 않습니다.
- 도구가 `OBSERVED_PASS_NOT_WCET`를 반환해도 부하가 실제로 들어왔는지는 따로 확인해야 합니다. 무입력 로그를 부하 시험 성공으로 쓰지 마세요.
- 링 버퍼는 128주기만 보존합니다. 전체 실행 max/누적 miss와 마지막 구간 trace를 혼동하지 마세요.
- `deadline_misses`: 예정 기준 시각에서 출력 경로 완료까지 20ms 초과
- `busy_max_us`: 제어 시작부터 trace 기록 이후까지. 마지막 통계 갱신·스케줄링 호출은 포함하지 않으므로 별도 여유 필요
- `release_abs_max_us`: 첫 관측 실행을 기준으로 한 위상 편차. 최초 tick에 대한 실제 dispatch 지연과 다름
- `skipped_releases`: 실행하지 않고 건너뛴 예정 주기. 실행된 주기의 통계가 좋아도 이 값이 있으면 누락 없는 실행이 아님
- 데이터 무결성: `received_frames`, `accepted_targets`, `dropped_targets`, `uart_rx_queue_drops`, `uart_rx_error_events`, `uart_rx_overrun_events`

## 11. 실패하면 분석할 순서

1. 부하가 실제 의도한 속도인지, 잘못된 빌드/클록/최적화/디버거 간섭은 아닌지 확인
2. 작업 자체가 긴지, 시작이 늦는지, IRQ/다른 태스크로 실행 도중 지연되는지 분리
3. 바이트 큐 손실, 목표 큐 손실, UART 하드웨어 오버런을 구분
4. 제어 우선순위 상승으로 수신 서비스가 밀리는지 확인
5. 주기 누락·busy 초과를 정상으로 숨기지 않고 원인과 수정 전후 로그 보존
6. 필요하면 수신 처리 구간과 큐 high-water 계측 추가 — **현재 이 추가 계측은 구현되지 않음**
7. 요구사항을 수정해야 한다면 부하 범위·시간 예산의 근거를 기록하고 동일 조건으로 재시험

## 12. 최종 산출물

- 재현 가능한 ARM 빌드와 보드 실행 절차
- 원본 CSV, 설정 기록, 입력 부하 증거
- A/B 동일 조건의 실제 시작 편차·응답시간·busy 시간·누락·손실 비교표
- 실패 원인과 수정 이력. 실제 실패가 없으면 그 사실도 그대로 기록
- 관측한 부하 범위에서의 마감시간 준수 결론, 남은 WCET/물리 출력 검증 범위
- 위 5가지 면접 질문에 대한 간결한 답변. 수치는 실제 측정값과 설계·계산값을 구분

우선 읽을 문서: `docs/realtime_contract.md`, `docs/runtime_measurement.md`, `docs/uart_interrupt_validation.md`. 설계 내용을 확인한 뒤 **실제 환경에서 빌드와 최초 한 번의 보드 측정을 먼저 성공시키는 것**이 다음 작업입니다.
