# STM32 / Zynq UART 인터럽트 수신 검증

작성일: 2026-10-19. **소스 구현과 호스트 테스트를 준비한 상태이며, ARM 빌드·보드 IRQ 동작·실시간 성능 실측은 미완료다.**

## 1. 바뀐 범위와 그대로 유지한 범위

```text
Simulink → UDP → Linux의 UART 프레임 생성 → UART 115200/8N1
  → [플랫폼별 UART ISR]
  → 바이트 큐: UartRxByte { epoch, value }, 기본 128항목
  → [수신 태스크: 큐에서 대기 → 공통 프레임 조립·CRC·역직렬화]
  → TargetState 큐: { x_cm, y_cm }, 기존 4항목
  → [제어 태스크: 기존 20 ms 주기, 최대 4개 목표 반영]
  → 공통 제어기 → 조향각[deg] → 공통 SG90 매핑 → PwmCommand[μs]
  → STM32: TIM4 출력 구현 / Zynq: 물리 출력 구현 없음
```

- 애플리케이션 태스크는 여전히 수신·제어 **2개**다. ISR은 별도 태스크가 아니다.
- 바이트 큐가 데이터 저장과 수신 태스크 깨우기를 함께 담당한다. 별도 알림 플래그는 필요 없다.
- 수신 태스크만 파서 상태를, 제어 태스크만 제어기 상태를 수정한다.
- 공통 `steering_controller.c`, `sg90_mapper.c`와 기존 제어 주기·설정·PWM 호출은 변경하지 않았다.
- 수신 태스크가 큐에 넣은 것은 프레임 형식/CRC를 통과한 좌표다. `x_cm > 0` 같은 의미상 유효성 판단은 기존 제어기가 담당한다.
- `now_ms`는 여전히 플랫폼 시각을 ms로 변환한 값이다. 목표 갱신 시각은 제어 태스크가 목표를 반영한 시각이며 센서 측정 시각이 아니다.
- 비교 전 구현은 STM32의 `HAL_UART_Receive()` 폴링 대기, Zynq의 최대 64바이트 폴링 + 1 tick 대기였다.

## 2. 플랫폼 구현

| 항목 | STM32F429 | Zynq-7000 PS |
| --- | --- | --- |
| 구현 파일 | `Core/Src/stm32_uart_rx.c` | `src/zynq_uart_rx.c` |
| 시작 시점 | 수신 태스크 시작 후 | 수신 태스크 시작 후, FreeRTOS GIC 초기화 이후 |
| IRQ | USART1 RXNE / PE / ERR | PS UART FIFO trigger / receive timeout / error |
| ISR 작업량 | SR→DR 읽기 1회, 바이트 큐 저장 | 최대 64바이트 FIFO 읽기, 바이트 큐 저장 |
| 태스크 대기 | `osMessageQueueGet(..., osWaitForever)` | `xQueueReceive(..., portMAX_DELAY)` |
| IRQ 내 큐 전달 | CMSIS-RTOS2의 ISR 지원 Put, timeout=0 | `xQueueSendFromISR` + `portYIELD_FROM_ISR` |
| 우선순위 | 현재 NVIC 숫자 5 | 현재 GIC 숫자 19를 3비트 shift한 0x98, level-sensitive |
| 오류 복구 | SR→DR로 오류 해제, 해당 바이트 폐기 | 오류 FIFO를 RX-only reset, 이후 수신 재개 |

STM32는 전용 RX ISR에서 레지스터를 읽는다. HAL의 `Receive_IT()` 완료 콜백/재등록 방식과 혼용하지 않는다. UART 초기 설정은 기존 HAL 설정을 사용한다. RX를 계속 활성화하므로 바이트마다 재등록할 공백이 없다. `HAL_UART_IRQHandler()`나 HAL Receive/Receive_IT/DMA를 USART1 RX에 추가 호출하지 않는다.

Zynq는 XUartPs로 설정하고 전용 ISR에서 FIFO를 읽는다. `XUartPs_Recv()` / `XUartPs_InterruptHandler()`와 혼용하지 않는다. 기존 FreeRTOS `xInterruptController`와 `xPortInstallInterruptHandler()`를 사용하므로 tick/예외 디스패처를 교체하지 않는다. 현 BSP의 인스턴스 `XPAR_XUARTPS_0`은 **주소 0xE0001000, 물리 UART1, IRQ 82**다. 이름만 보고 `XPAR_XUARTPS_0_INTR`(물리 UART0 IRQ 59)을 사용하면 잘못 연결된다. 코드에서 base address로 물리 IRQ를 선택한다.

공식 참고: [ST F4 UART 드라이버](https://github.com/STMicroelectronics/stm32f4xx-hal-driver/blob/master/Src/stm32f4xx_hal_uart.c), [AMD/Xilinx ARM_CA9 포트](https://github.com/Xilinx/embeddedsw/blob/master/ThirdParty/bsp/freertos10_xilinx/src/Source/portable/GCC/ARM_CA9/portmacro.h). 구현 대조 기준은 저장소에 포함된 HAL/BSP 버전이다.

## 3. 유실·오류 처리 규칙

Zynq timeout 카운터는 빈 FIFO/idle line에서도 소프트웨어 재시작으로 동작할 수 있다. 따라서 실제 바이트를 읽은 IRQ에서만 timeout을 재시작하고, 빈 timeout IRQ는 확인 후 종료한다. 무입력 상태에서 timeout IRQ가 연속 발생하지 않는지 보드에서 확인한다. [AMD 수신 timeout 동작 설명](https://docs.amd.com/r/en-US/ug585-zynq-7000-SoC-TRM/Receiver-Timeout-Mechanism)

ISR은 각 바이트에 `epoch`를 붙여 큐에 넣는다. 큐가 차서 바이트를 버리거나 하드웨어 오류가 발생하면 epoch를 증가시킨다. 수신 태스크는 **실제로 꺼낸 바이트의 epoch**가 달라지는 지점에서 미완성 프레임을 버린다. 그래서 큐 안에 오류 이전 데이터가 남아 있어도 오류 전후 바이트를 하나의 패킷으로 합치지 않는다. 오류 이전의 완전한 정상 프레임은 처리할 수 있다.

파서의 CRC 실패/잡음은 기존 sliding-window 재동기화를 사용한다. 하드웨어 오류가 없다면 CRC 실패 자체가 epoch를 바꾸지는 않는다. `rejected_windows`는 패킷 개수가 아니라 거부한 15바이트 창 개수다.

| 디버거 값 | 의미 |
| --- | --- |
| `uart_rx_stats.ready` | 초기화 성공 시 1. 실제 수신 성공은 별도로 확인 |
| `uart_rx_stats.irq_count` | UART ISR 진입 횟수. Zynq는 IRQ 1회에 여러 바이트 처리 가능 |
| `uart_rx_stats.bytes_read` | 레지스터/FIFO에서 읽은 바이트 수. UART 오류로 FIFO reset한 데이터 수는 알 수 없음 |
| `uart_rx_stats.queue_drops` | 바이트 큐가 차서 저장하지 못한 바이트 수 |
| `uart_rx_stats.error_events` | 오류 상태를 관측한 횟수. 손상 바이트의 정확한 개수는 아님 |
| `uart_rx_stats.overrun_events` | 하드웨어 overrun 관측 횟수 |
| `uart_rx_stats.fifo_resets` | Zynq 오류 처리 중 FIFO 폐기 횟수. STM32는 0 |
| `uart_rx_stats.last_error` | 마지막 플랫폼별 오류 비트. 두 보드의 비트값 자체를 비교하지 않음 |
| `uart_rx_gap_resets` | 수신 태스크가 epoch 변경을 만나 부분 프레임을 폐기한 횟수 |
| `received_frames` / `accepted_targets` | 통신 검증 통과 / 제어기가 받아들인 목표 수 |
| `dropped_targets` | 기존 TargetState 큐의 full로 버린 목표 수. 바이트 유실과 별개 |

오류 뒤 새 바이트가 아직 없으면 gap reset 카운터는 즉시 증가하지 않을 수 있다. epoch가 바뀐 첫 바이트를 태스크가 꺼낼 때 반영된다. 통신 오류가 곧바로 중립 출력을 강제하지는 않는다. 마지막 유효 목표 유지와 400 ms timeout 정책은 기존대로다.

## 4. 호스트 확인

Windows의 VS 2022 개발 환경에서:

```bat
tools\run_host_checks.cmd
```

기본 스크립트는 현재 설치된 VS 2022 Community 경로를 사용한다. 다른 버전은 먼저 해당 Developer Command Prompt를 열어 실행한다. Linux에서는 기존 CMake/CTest 빌드에 새 테스트가 등록되어 있다.

- 기존 공통 테스트 7개 + 유실 경계/epoch wrap 테스트 1개.
- **실제 신규 플랫폼 C 파일**을 하드웨어/RTOS 대역과 연결한 STM32·Zynq 테스트 각 1개. 바이트 순서, IRQ 번호/우선순위, 무기한 큐 대기 인자, 큐 초과, UART 오류 후 재개, Zynq ISR 처리량 상한을 검사한다.
- 각 플랫폼의 큐 할당 실패 경로를 별도 프로세스로 검사한다.
- Python 보고서와 입력 생성 도구 테스트.

대역 테스트는 하드웨어 레지스터의 전기적 동작, 실제 RTOS 선점, ISR 지연을 검증하지 않는다. POSIX serial 테스트는 Windows 스크립트 대상에서 제외된다.

## 5. 보드 빌드 준비

1. 원래 ARM 개발 환경에서 프로젝트를 연다. 이 Windows 체크아웃의 Git symlink placeholder를 실제 링크로 복원했는지 확인한다. STM32 `Core/Inc/protocol`, 제어·매핑 공통 소스 링크, Zynq `src/common_include` 등을 확인한다. 링크를 복사본으로 바꾸어 공통 소스를 이중 관리하지 않는다.
2. CubeIDE에서 `Core/Src/stm32_uart_rx.c`, Vitis에서 `src/zynq_uart_rx.c`가 빌드에 포함되도록 refresh/clean build한다. 앞서 추가한 protocol 소스들도 포함한다. 테스트의 `tests/platform/fakes`는 **보드 include path에 추가하지 않는다**.
3. STM32 map에서 `USART1_IRQHandler`가 `stm32_uart_rx.c`에 단 한 번 정의되는지 확인한다. 현재 `.ioc`의 USART1 NVIC 자동 생성은 사용하지 않고 전용 모듈이 수신 태스크 시작 후 활성화한다. CubeMX에서 별도 USART1 IRQ handler를 생성하면 중복 정의를 정리해야 한다.
4. Zynq에서 PS UART1(현재 IRQ 82), FreeRTOS 포트의 GIC 인스턴스, priority 0x98을 확인한다. standalone 방식의 전역 예외 핸들러를 추가 설치하지 않는다.
5. 추가 바이트 큐는 기본 128항목이며 현재 ABI에서 항목당 8바이트다. 큐 데이터 약 1 KiB와 RTOS 객체 메모리가 추가된다. heap 여유와 두 태스크 및 IRQ 스택 사용량을 점검한다. 기존 STM32 태스크 스택은 512바이트, Zynq는 각각 1024 stack words다.
6. 부팅 후 `uart_rx_stats.ready == 1`과 제어 주기 샘플 증가를 확인한다. 초기화 실패하면 수신 태스크는 종료하고 ready는 0이다. 정상 입력은 ready 확인 후 시작한다. 초기화 전 들어온 데이터는 유지 대상으로 삼지 않는다.

## 6. 보드 기능 시험

먼저 기존 Linux 송신기와 Simulink로 정상 경로를 확인한다. 아래 직접 UART 시험은 Linux 송신기를 종료한 후 수행한다. 같은 포트를 동시에 열지 않는다. 핀 전압·GND·UART 라우팅은 기존 보드 연결을 사용한다.

```sh
# 송신 내용만 확인 (Windows에서도 가능)
python tools/uart_rx_exercise.py --case recovery --dry-run

# Linux에서 실제 송신; 각 보드의 포트로 바꿔 실행
python3 tools/uart_rx_exercise.py --port /dev/ttyUSB1 --case normal --count 100
python3 tools/uart_rx_exercise.py --port /dev/ttyUSB1 --case fragmented
python3 tools/uart_rx_exercise.py --port /dev/ttyUSB1 --case recovery
python3 tools/uart_rx_exercise.py --port /dev/ttyUSB1 --case invalid
python3 tools/uart_rx_exercise.py --port /dev/ttyUSB1 --case burst --count 200
```

시험은 가능하면 매번 리셋하고, 입력 종료 후 디버거로 한 번 halt하여 확인한다. 짧은 입력은 이미 timeout 중립으로 돌아왔을 수 있으므로 trace에서 해당 구간을 찾는다. 반복 중 디버거 polling/중단은 타이밍 시험과 분리한다.

| 시험 | 기대 동작 / 합격 기준 |
| --- | --- |
| 무입력 5초 | ready=1, 수신 태스크가 바이트 큐에서 Blocked. UART IRQ가 지속 증가하지 않음. 제어 주기는 계속 실행 |
| 정상 10 Hz, 100프레임 | 수신/유효 목표 수 증가, 좌우 출력 교대, 바이트/목표 drop·UART 오류 0 |
| fragmented | 한 프레임을 1바이트씩 간격을 두고 보내도 정확히 1프레임 복원 |
| recovery | 잡음+CRC 손상+잘린 프레임을 보내도 마지막 정상 2프레임(seq 3,4) 복원. 손상 프레임을 목표로 반영하지 않음 |
| invalid | CRC가 맞는 x=0/-1은 수신 프레임 수에는 포함, 제어기에서 거부. 마지막 유효 목표의 timeout 시각을 연장하지 않음 |
| 입력 중단 | 마지막 유효 목표 반영 후 기존 400 ms timeout에 따라 중립 전환. tick/제어 주기 양자화 고려 |
| burst | 정해진 부하에서 수신/유실 수 기록. 목표 큐 drop과 바이트 큐 drop을 구분. 끝난 뒤 정상 프레임을 보내면 복원 가능 |
| 바이트 큐 강제 초과 | 기능 시험용으로 수신 태스크만 일시 suspend하되 CPU/IRQ는 계속 실행시켜 128바이트보다 많이 전송. 태스크 resume 후 정상 프레임 수신·gap reset 확인 |
| 하드웨어 overrun | 기능 시험에서 CPU를 halt한 동안 FIFO 용량 이상 송신한 뒤 resume. 오류/overrun 수 증가와 이후 정상 프레임 복원 확인. 이는 성능 수치로 쓰지 않음 |

태스크 suspend/resume은 해당 디버거의 RTOS 기능으로 수행한다. 지원되지 않으면 해당 강제 초과 시험을 미실시로 표시하고, 호스트 테스트만으로 보드 검증을 통과했다고 쓰지 않는다. 코드 전체를 halt하면 IRQ도 멈추므로 바이트 큐 full과 하드웨어 overrun을 구분할 수 없다. parity/framing 오류는 송신기 설정 변경 또는 계측 장비로 별도 주입하며, CRC 손상과 동일시하지 않는다.

기본 128바이트 큐는 115200/8N1 포화 입력에서 약 11.1 ms 분량이다. 지속 포화 수신이나 임의의 긴 태스크 지연을 보장하지 않는다. 시험 입력 빈도·burst 크기를 기록하고 처리 한계를 넘는 조건은 별도 부하 시험으로 보고한다.

## 7. 성능 비교와 시각 자료

기능 시험 통과 후 [기존 측정 절차](runtime_measurement.md)에 따라 같은 입력·빌드 옵션·측정 시간을 적용한다. 정상·오류·burst 시험의 통계를 섞지 않는다. 인터럽트도 제어 태스크를 선점하므로 `common_us`에는 그 시간이 포함될 수 있다.

```text
halt 상태에서 tools/export_runtime_probe.gdb 실행 → stm32.csv / zynq.csv
```

```sh
python3 tools/compare_runtime.py stm32.csv zynq.csv --output results/irq-run1
```

HTML/SVG 그래프, CSV/JSON 수치가 생성된다. HTML/수치 표에는 IRQ 횟수, 읽은 바이트, 바이트 큐 유실, 하드웨어 오류와 gap reset도 포함된다. 예전 export에 없던 IRQ 수치는 0으로 채우지 않고 미기록으로 표시한다. IRQ 수와 공통 실행시간만으로 CPU 사용률 개선을 계산하지 않는다. CPU 사용률 비교는 별도 RTOS runtime statistics/idle-time 계측이 필요하다.

완료 전 발표 표현: “두 플랫폼의 수신을 인터럽트 방식으로 구성하고, 두 태스크 및 공통 제어·매핑을 유지한 상태에서 검증 절차를 준비했습니다.” 실제 보드 시험이 끝난 뒤에만 ‘보드에서 검증했다’ 또는 성능 개선 수치를 추가한다.
