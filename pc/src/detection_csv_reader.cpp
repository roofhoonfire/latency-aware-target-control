#include "input/detection_csv_reader.h"

#include <fstream>
#include <sstream>
#include <string>

namespace
{

constexpr const char* kExpectedHeader =
    "time_sec,target_x,target_y,target_vx,target_vy";

}  // namespace

namespace latc::input
{

bool read_first_detection_csv(
    const char* csv_path,
    DetectionRecord* detection)
{
    if (csv_path == nullptr || detection == nullptr)
    {
        return false;
    }

    std::ifstream input_file(csv_path);

    if (!input_file.is_open())
    {
        return false;
    }

    std::string header;

    if (!std::getline(input_file, header))
    {
        return false;
    }

    if (!header.empty() && header.back() == '\r')
    {
        header.pop_back();
    }

    if (header != kExpectedHeader)
    {
        return false;
    }

    std::string row;

    if (!std::getline(input_file, row))
    {
        return false;
    }

    std::stringstream row_stream(row);

    DetectionRecord parsed_detection{};

    char comma1 = '\0';
    char comma2 = '\0';
    char comma3 = '\0';
    char comma4 = '\0';

    if (!(row_stream
          >> parsed_detection.time_sec
          >> comma1
          >> parsed_detection.target_x_m
          >> comma2
          >> parsed_detection.target_y_m
          >> comma3
          >> parsed_detection.target_vx_mps
          >> comma4
          >> parsed_detection.target_vy_mps))
    {
        return false;
    }

    if (comma1 != ',' ||
        comma2 != ',' ||
        comma3 != ',' ||
        comma4 != ',')
    {
        return false;
    }

    *detection = parsed_detection;

    return true;
}

}  // namespace latc::input
