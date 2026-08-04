#include <iostream>

#include "protocol/target_command.h"

int main()
{
    const TargetCommand command{
        1U,      // sequence
        1000,    // target_x
        -1000,   // target_y
        40U      // prediction_ms
    };

    std::cout
        << "Latency-Aware Target Control PC mock started.\n"
        << "TargetCommand\n"
        << "  sequence      : " << command.sequence << '\n'
        << "  target_x      : " << command.target_x << '\n'
        << "  target_y      : " << command.target_y << '\n'
        << "  prediction_ms : " << command.prediction_ms << '\n';

    return 0;
}
