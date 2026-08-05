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