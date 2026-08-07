add_executable(
    target_command_codec_test
    common/target_command_codec_test.cpp
)

target_compile_options(
    target_command_codec_test
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(
    target_command_codec_test
    PRIVATE
    protocol_codec
)

add_test(
    NAME target_command_codec_test
    COMMAND target_command_codec_test
)


add_executable(
    crc16_test
    common/crc16_test.cpp
)

target_compile_options(
    crc16_test
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(
    crc16_test
    PRIVATE
    protocol_codec
)

add_test(
    NAME crc16_test
    COMMAND crc16_test
)


add_executable(
    uart_frame_codec_test
    common/uart_frame_codec_test.cpp
)

target_compile_options(
    uart_frame_codec_test
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(
    uart_frame_codec_test
    PRIVATE
    protocol_codec
)

add_test(
    NAME uart_frame_codec_test
    COMMAND uart_frame_codec_test
)

add_executable(
    serial_port_test
    pc/serial_port_test.cpp
)

target_compile_features(
    serial_port_test
    PRIVATE
    cxx_std_17
)

target_compile_options(
    serial_port_test
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(
    serial_port_test
    PRIVATE
    pc_uart_transport
)

add_test(
    NAME serial_port_test
    COMMAND serial_port_test
)
