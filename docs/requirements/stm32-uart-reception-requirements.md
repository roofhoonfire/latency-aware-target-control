STM-UART-RX-001

The STM32 shall configure USART1 for
115200 baud, 8 data bits, no parity,
1 stop bit, and no hardware flow control.

STM-UART-RX-002

The STM32 shall receive the byte sequence
transmitted through the ST-LINK Virtual COM Port
without modification.

Verification Method:
Hardware integration test

Test Input:
AA 55 01 01 08 01 00 E8 03 18 FC 28 00 9F E2

Expected:
rx_buffer contains identical 15 bytes.

Result:
PASS
