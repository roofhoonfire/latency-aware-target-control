#ifndef LATC_PC_INPUT_DETECTION_RECORD_H
#define LATC_PC_INPUT_DETECTION_RECORD_H

namespace latc::input
{

struct DetectionRecord
{
    double time_sec;
    double target_x_m;
    double target_y_m;
    double target_vx_mps;
    double target_vy_mps;
};

}  // namespace latc::input

#endif  // LATC_PC_INPUT_DETECTION_RECORD_H
