#pragma once

#include <string_view>

namespace dispatcher::alarm
{
    enum class AlarmSeverity
    {
        Info,
        Warning,
        Critical
    };

    constexpr std::string_view to_string(AlarmSeverity severity)
    {
        switch (severity)
        {
        case AlarmSeverity::Info:
            return "info";
        case AlarmSeverity::Warning:
            return "warning";
        case AlarmSeverity::Critical:
            return "critical";
        }

        return "unknown";
    }
}