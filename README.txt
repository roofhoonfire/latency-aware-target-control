# Latency-Aware PC–STM32 Target Control System

## Objective
PC에서 생성한 목표 좌표를 STM32에 전달하고,
통신 및 제어 지연을 측정하여 미래 목표 위치를 보정한다.

## Architecture
Target Input
→ PC Prediction
→ UART/CAN Transport
→ STM32 FreeRTOS
→ Virtual or Physical Actuator

## Minimum Success Criteria
- UART 기반 PC–STM32 E2E 통신
- FreeRTOS CommRxTask와 ControlTask 분리
- ACK 기반 지연 측정
- 요구사항과 테스트 결과 추적
- 문제 해결 보고서 2개 이상

## Extensions
- SocketCAN 및 실제 CAN
- Latency compensation
- Camera input
- Physical servo motor

## Development Method
ASPICE-inspired traceability
and AUTOSAR-inspired configuration-based code generation