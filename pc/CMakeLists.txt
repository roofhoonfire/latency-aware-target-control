add_executable(
    target_control_pc
    src/main.cpp
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
)