#pragma once

#include <string_view>

namespace dispatcher::alarm
{
    enum class AlarmState
    {
        Normal,
        Active,
        Acknowledged
    };

    constexpr std::string_view to_string(AlarmState state)
    {
        switch (state)
        {
        case AlarmState::Normal:
            return "normal";
        case AlarmState::Active:
            return "active";
        case AlarmState::Acknowledged:
            return "acknowledged";
        }

        return "unknown";
    }
}