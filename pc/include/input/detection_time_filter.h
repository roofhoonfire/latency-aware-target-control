#ifndef LATC_PC_INPUT_DETECTION_TIME_FILTER_H
#define LATC_PC_INPUT_DETECTION_TIME_FILTER_H

#include <cmath>

namespace latc::input
{

/* One simulation run per instance. A backwards timestamp is stale input,
 * not evidence of a new run. Restart the live receiver for a new simulation.
 * This checks sample ordering, not sample age relative to wall-clock time.
 */
class DetectionTimeFilter final
{
public:
    [[nodiscard]] bool accept(double time_sec) noexcept
    {
        if (!std::isfinite(time_sec) ||
            (has_last_time_ && time_sec <= last_time_sec_))
        {
            return false;
        }

        last_time_sec_ = time_sec;
        has_last_time_ = true;
        return true;
    }

private:
    double last_time_sec_{0.0};
    bool has_last_time_{false};
};

}  // namespace latc::input

#endif  // LATC_PC_INPUT_DETECTION_TIME_FILTER_H
