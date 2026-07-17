#pragma once

#include <string>

namespace dispatcher::alarm
{
    struct AlarmConfigurationMetadata
    {
        std::string name;
        std::string description;
        std::string created_by;
    };
}