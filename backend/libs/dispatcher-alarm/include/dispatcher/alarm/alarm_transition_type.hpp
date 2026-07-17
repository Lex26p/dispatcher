#pragma once

#include <string_view>

namespace dispatcher::alarm
{
    enum class AlarmTransitionType
    {
        None,
        Activated,
        Acknowledged,
        Cleared
    };

    constexpr std::string_view to_string(AlarmTransitionType transition_type)
    {
        switch (transition_type)
        {
        case AlarmTransitionType::None:
            return "none";
        case AlarmTransitionType::Activated:
            return "activated";
        case AlarmTransitionType::Acknowledged:
            return "acknowledged";
        case AlarmTransitionType::Cleared:
            return "cleared";
        }

        return "unknown";
    }
}