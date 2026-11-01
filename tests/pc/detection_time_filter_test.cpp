#include <iostream>
#include <limits>

#include "input/detection_time_filter.h"

#define CHECK(condition)                                      \
    do                                                        \
    {                                                         \
        if (!(condition))                                     \
        {                                                     \
            std::cerr << "[FAIL] line " << __LINE__            \
                      << ": " << #condition << '\n';           \
            return 1;                                         \
        }                                                     \
    } while (false)

int main()
{
    latc::input::DetectionTimeFilter filter;
    // Zero is a valid first simulation timestamp; equal samples are duplicates.
    CHECK(filter.accept(0.0));
    CHECK(!filter.accept(0.0));
    CHECK(filter.accept(10.2));
    CHECK(!filter.accept(10.1));
    CHECK(!filter.accept(10.2));
    // Rejecting 10.1 must not lower the accepted timestamp to 10.1.
    CHECK(!filter.accept(10.15));
    CHECK(filter.accept(10.3));

    // Malformed timestamps must neither initialize nor advance the filter.
    latc::input::DetectionTimeFilter invalid_first;
    CHECK(!invalid_first.accept(std::numeric_limits<double>::quiet_NaN()));
    CHECK(!invalid_first.accept(std::numeric_limits<double>::infinity()));
    CHECK(!invalid_first.accept(-std::numeric_limits<double>::infinity()));
    CHECK(invalid_first.accept(0.0));
    CHECK(!filter.accept(std::numeric_limits<double>::quiet_NaN()));
    CHECK(!filter.accept(std::numeric_limits<double>::infinity()));
    CHECK(filter.accept(10.4));

    // A clock reset is rejected in the current run, but a new run starts fresh.
    CHECK(!filter.accept(0.0));
    CHECK(!filter.accept(0.1));
    CHECK(!filter.accept(10.35));
    latc::input::DetectionTimeFilter new_run;
    CHECK(new_run.accept(0.0));
    CHECK(new_run.accept(0.1));

    std::cout << "[PASS] detection timestamp ordering, duplicates, invalid values, "
                 "and new-run reset\n";
    return 0;
}
