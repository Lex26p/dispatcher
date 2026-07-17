#include <dispatcher/alarm/alarm_suppression_mode.hpp>

namespace dispatcher::alarm
{
    const char* to_string(AlarmSuppressionMode mode) noexcept
    {
        switch (mode)
        {
        case AlarmSuppressionMode::Shelved:
            return "shelved";

        case AlarmSuppressionMode::Suppressed:
            return "suppressed";

        case AlarmSuppressionMode::Inhibited:
            return "inhibited";
        }

        return "unknown";
    }

    bool is_operator_controlled(AlarmSuppressionMode mode) noexcept
    {
        return mode == AlarmSuppressionMode::Shelved;
    }

    bool is_system_controlled(AlarmSuppressionMode mode) noexcept
    {
        return mode == AlarmSuppressionMode::Suppressed
            || mode == AlarmSuppressionMode::Inhibited;
    }
}