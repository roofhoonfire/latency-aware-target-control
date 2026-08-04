#ifndef LATC_PROTOCOL_TARGET_COMMAND_H
#define LATC_PROTOCOL_TARGET_COMMAND_H

#include <stdint.h>

/*
 * Logical application message sent from the PC to the STM32.
 *
 * This structure represents the meaning of the command.
 * It must not be transmitted by copying its memory directly.
 * Serialization and deserialization will be implemented separately.
 */
typedef struct
{
    uint16_t sequence;
    int16_t target_x;
    int16_t target_y;
    uint16_t prediction_ms;
} TargetCommand;

#endif
