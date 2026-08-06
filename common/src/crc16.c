#include "protocol/crc16.h"

bool crc16_ccitt_false_calculate(
    const uint8_t* data,
    size_t data_size,
    uint16_t* crc_out)
{
    if (crc_out == NULL)
    {
        return false;
    }

    if ((data == NULL) && (data_size > 0U))
    {
        return false;
    }

    uint16_t crc = 0xFFFFU;

    for (size_t byte_index = 0U;
         byte_index < data_size;
         ++byte_index)
    {
        crc ^= (uint16_t)(
            (uint16_t)data[byte_index] << 8U
        );

        for (uint8_t bit_index = 0U;
             bit_index < 8U;
             ++bit_index)
        {
            if ((crc & 0x8000U) != 0U)
            {
                crc = (uint16_t)(
                    (crc << 1U) ^ 0x1021U
                );
            }
            else
            {
                crc = (uint16_t)(crc << 1U);
            }
        }
    }

    *crc_out = crc;

    return true;
}
