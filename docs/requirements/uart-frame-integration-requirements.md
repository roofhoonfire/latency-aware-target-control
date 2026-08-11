UART-FRAME-INT-001

STM32 프로젝트는 PC와 동일한 common protocol source를 중복 복사 없이 사용해야 한다.

UART-FRAME-INT-002

STM32 application에서

uart_frame_validate_target_command()

를 호출할 수 있어야 한다.

UART-FRAME-INT-003

STM32 ARM GCC build에서 validator와 CRC 구현이 정상적으로 컴파일 및 링크되어야 한다.
