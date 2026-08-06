add_library(
    pc_uart_transport
    STATIC
    src/serial_port.cpp
)

target_include_directories(
    pc_uart_transport
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(
    pc_uart_transport
    PUBLIC
    cxx_std_17
)

target_compile_options(
    pc_uart_transport
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(
    target_control_pc
    src/main.cpp
)

target_compile_features(
    target_control_pc
    PRIVATE
    cxx_std_17
)

target_compile_options(
    target_control_pc
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(
    target_control_pc
    PRIVATE
    protocol_codec
    pc_uart_transport
)
