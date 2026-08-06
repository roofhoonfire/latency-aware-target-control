#ifndef LATC_PROTOCOL_CRC16_H
#define LATC_PROTOCOL_CRC16_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Calculates CRC-16/CCITT-FALSE.
 *
 * Parameters:
 *   Polynomial : 0x1021
 *   Initial    : 0xFFFF
 *   RefIn      : false
 *   RefOut     : false
 *   XorOut     : 0x0000
 *
 * The output value is written only when calculation succeeds.
 *
 * A null data pointer is permitted only when data_size is zero.
 */
bool crc16_ccitt_false_calculate(
    const uint8_t* data,
    size_t data_size,
    uint16_t* crc_out
);

#ifdef __cplusplus
}
#endif

#endif /* LATC_PROTOCOL_CRC16_H */
