#ifndef LATC_TARGET_FRAME_STREAM_H
#define LATC_TARGET_FRAME_STREAM_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "protocol/uart_frame.h"
#include "protocol/uart_frame_validator.h"
#include "protocol/target_command_codec.h"

/* Fixed V1 target-frame stream adapter. A sliding window recovers alignment
 * after noise, truncated frames, or CRC failures, including embedded SOFs.
 * Owned by the receive task only. No OS or peripheral access.
 */
#define TARGET_FRAME_SIZE (UART_FRAME_OVERHEAD_SIZE + TARGET_COMMAND_WIRE_SIZE)
typedef struct {
    uint8_t bytes[TARGET_FRAME_SIZE];
    size_t used;
    uint32_t rejected_windows;
} TargetFrameStream;

static inline bool target_frame_stream_push(
    TargetFrameStream *s, uint8_t byte, TargetCommand *out)
{
    s->bytes[s->used++] = byte;
    if (s->used < TARGET_FRAME_SIZE) return false;
    if (uart_frame_validate_target_command(s->bytes, TARGET_FRAME_SIZE)
            == UART_FRAME_VALID &&
        target_command_deserialize(s->bytes + UART_FRAME_PAYLOAD_OFFSET,
                                   TARGET_COMMAND_WIRE_SIZE, out)) {
        s->used = 0U;
        return true;
    }
    memmove(s->bytes, s->bytes + 1, TARGET_FRAME_SIZE - 1U);
    s->used = TARGET_FRAME_SIZE - 1U;
    ++s->rejected_windows;
    return false;
}
#endif
