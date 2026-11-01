#include <cassert>
#include <cstdint>
#include <iostream>
#include "diagnostics/runtime_probe.h"
#include "diagnostics/runtime_schedule.h"
#include "protocol/target_frame_stream.h"
#include "protocol/crc16.h"

int main()
{
    uint8_t frame[TARGET_FRAME_SIZE] = {0xAA, 0x55, 1, 1, 8};
    TargetCommand original = {7, 515, -97, 0};
    assert(target_command_serialize(&original, frame + 5, 8));
    uint16_t crc = 0;
    assert(crc16_ccitt_false_calculate(frame + 2, 11, &crc));
    frame[13] = static_cast<uint8_t>(crc);
    frame[14] = static_cast<uint8_t>(crc >> 8);
    TargetFrameStream stream{};
    TargetCommand decoded{};
    unsigned accepted = 0;
    // Noise, a truncated prefix, corruption, then consecutive valid frames.
    const uint8_t noise[] = {0, 0xAA, 0x55, 1, 1, 8, 9};
    for (auto b : noise) accepted += target_frame_stream_push(&stream, b, &decoded);
    uint8_t broken[TARGET_FRAME_SIZE];
    for (unsigned i = 0; i < TARGET_FRAME_SIZE; ++i) broken[i] = frame[i];
    broken[8] ^= 1;
    for (auto b : broken) accepted += target_frame_stream_push(&stream, b, &decoded);
    assert(accepted == 0);
    for (int repeat = 0; repeat < 2; ++repeat)
        for (auto b : frame) accepted += target_frame_stream_push(&stream, b, &decoded);
    assert(accepted == 2 && decoded.target_x == 515 && decoded.target_y == -97);
    assert(stream.rejected_windows > 0);

    RuntimeProbe probe{};
    RuntimeSample s{};
    s.common_us = 3; s.work_us = 100; s.release_error_us = -10;
    runtime_probe_record(&probe, s, 20000);
    s.common_us = 9; s.work_us = 150; s.release_error_us = 19900;
    runtime_probe_record(&probe, s, 20000);
    assert(probe.samples == 2 && probe.common_min_us == 3);
    assert(probe.common_max_us == 9 && probe.common_total_us == 12);
    assert(probe.deadline_misses == 1 && probe.release_abs_max_us == 19900);
    RuntimeProbe boundary_probe{};
    RuntimeSample boundary{};
    boundary.release_error_us = 1000;
    boundary.work_us = 19000;
    runtime_probe_record(&boundary_probe, boundary, 20000);
    assert(boundary_probe.deadline_misses == 0);
    boundary.work_us = 19001;
    runtime_probe_record(&boundary_probe, boundary, 20000);
    assert(boundary_probe.deadline_misses == 1);
    runtime_probe_busy(&probe, 5000, 5000);
    assert(probe.busy_budget_misses == 0);
    runtime_probe_busy(&probe, 5001, 5000);
    assert(probe.busy_budget_misses == 1 && probe.busy_max_us == 5001);
    uint32_t next = 0;
    assert(runtime_next_release(100, 105, 20, &next) == 1 && next == 120);
    assert(runtime_next_release(100, 120, 20, &next) == 2 && next == 140);
    assert(runtime_next_release(100, 169, 20, &next) == 4 && next == 180);
    assert(runtime_next_release(UINT32_MAX - 9, UINT32_MAX - 4, 20, &next)
           == 1 && next == 10);
    assert(runtime_next_release(UINT32_MAX - 9, 11, 20, &next)
           == 2 && next == 30);
    for (unsigned i = 0; i < RUNTIME_TRACE_CAPACITY; ++i)
        runtime_probe_record(&probe, RuntimeSample{}, 20000);
    assert(probe.trace_next == 2 && probe.trace[1].cycle == 129);
    std::cout << "Runtime stream recovery, metrics, and trace wrap: PASS\n";
}
