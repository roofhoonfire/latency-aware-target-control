#ifndef LATC_CONTROL_TARGET_STATE_H
#define LATC_CONTROL_TARGET_STATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Hardware-independent semantic target representation.
 *
 * Coordinate convention:
 *   x_cm > 0 : forward
 *   y_cm > 0 : left
 *   y_cm < 0 : right
 *
 * Units:
 *   centimeter
 *
 * This type contains no protocol metadata and no
 * platform-specific information.
 */
typedef struct
{
    int16_t x_cm;
    int16_t y_cm;
} TargetState;

#ifdef __cplusplus
}
#endif

#endif /* LATC_CONTROL_TARGET_STATE_H */
