#ifndef LATC_PC_INPUT_DETECTION_CSV_READER_H
#define LATC_PC_INPUT_DETECTION_CSV_READER_H

#include "input/detection_record.h"

namespace latc::input
{

[[nodiscard]] bool read_first_detection_csv(
    const char* csv_path,
    DetectionRecord* detection);

}  // namespace latc::input

#endif  // LATC_PC_INPUT_DETECTION_CSV_READER_H
