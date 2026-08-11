# UART Frame STM32 Hardware Integration Test

## Test Objective

PC에서 생성 및 송신한 TargetCommand UART Frame이
STM32F429I-DISC1에서 동일한 바이트 배열로 수신되고,
UART Frame Validator에 의해 정상 Frame으로 판정되는지 검증한다.

## Related Requirements

- UART Frame Protocol Requirements
- PC UART Transport Requirements
- STM32 UART Reception Requirements
- UART Frame Validation Requirements

## Test Environment

- Ubuntu 22.04.5 LTS
- STM32F429I-DISC1
- USART1
- /dev/ttyACM0
- 115200 baud
- 8N1
- No flow control

## Test Input

TargetCommand:

- sequence = 1
- target_x = 1000
- target_y = -1000
- prediction_ms = 40

Expected UART Frame:

AA 55 01 01 08 01 00 E8 03 18 FC 28 00 9F E2

## Acceptance Criteria

1. PC에서 15-byte UART Frame이 정상 송신될 것.
2. STM32 rx_buffer가 송신 Frame과 완전히 동일할 것.
3. uart_frame_validate_target_command()가 UART_FRAME_VALID를 반환할 것.

## Test Result

Received Frame:

AA 55 01 01 08 01 00 E8 03 18 FC 28 00 9F E2

Validation Result:

UART_FRAME_VALID

## Result

PASS
