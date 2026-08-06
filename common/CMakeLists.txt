add_library(
    protocol_types
    INTERFACE
)

target_include_directories(
    protocol_types
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(
    protocol_codec
    STATIC
    src/crc16.c
    src/target_command_codec.c
    src/uart_frame_codec.c
)

target_link_libraries(
    protocol_codec
    PUBLIC
    protocol_types
)

target_compile_options(
    protocol_codec
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
