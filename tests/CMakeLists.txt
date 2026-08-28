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

add_executable(
    uart_frame_validator_test
    common/uart_frame_validator_test.cpp
)

target_compile_options(
    uart_frame_validator_test
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(
    uart_frame_validator_test
    PRIVATE
    protocol_codec
)

add_test(
    NAME uart_frame_validator_test
    COMMAND uart_frame_validator_test
)

add_executable(
    steering_controller_test
    common/steering_controller_test.cpp
)

target_compile_options(
    steering_controller_test
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(
    steering_controller_test
    PRIVATE
    control_app
)

add_test(
    NAME steering_controller_test
    COMMAND steering_controller_test
)

add_executable(
    sg90_mapper_test
    common/sg90_mapper_test.cpp
)

target_compile_options(
    sg90_mapper_test
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(
    sg90_mapper_test
    PRIVATE
    actuator_service
)

add_test(
    NAME sg90_mapper_test
    COMMAND sg90_mapper_test
)
