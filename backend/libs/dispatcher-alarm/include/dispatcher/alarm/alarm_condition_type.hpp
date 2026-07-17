#pragma once

#include <string_view>

namespace dispatcher::alarm
{
    enum class AlarmConditionType
    {
        High,
        HighHigh,
        Low,
        LowLow
    };

    constexpr std::string_view to_string(AlarmConditionType type)
    {
        switch (type)
        {
        case AlarmConditionType::High:
            return "high";
        case AlarmConditionType::HighHigh:
            return "high_high";
        case AlarmConditionType::Low:
            return "low";
        case AlarmConditionType::LowLow:
            return "low_low";
        }

        return "unknown";
    }
}