#ifndef TARGET_COMMAND_CODEC_H

#define TARGET_COMMAND_CODEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "protocol/target_command.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TARGET_COMMAND_WIRE_SIZE 8U

bool target_command_serialize(
	const TargetCommand* command,
	uint8_t* buffer,
	size_t buffer_size
);

bool target_command_deserialize(
	const uint8_t * buffer,
	size_t buffer_size,
	TargetCommand* command
);


#ifdef __cplusplus
}
#endif

#endif

