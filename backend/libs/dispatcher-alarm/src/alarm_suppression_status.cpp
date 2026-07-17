#include <dispatcher/alarm/alarm_suppression_status.hpp>

namespace dispatcher::alarm
{
    const char* to_string(AlarmSuppressionStatus status) noexcept
    {
        switch (status)
        {
        case AlarmSuppressionStatus::Applied:
            return "applied";

        case AlarmSuppressionStatus::Cleared:
            return "cleared";

        case AlarmSuppressionStatus::UnknownAlarm:
            return "unknown_alarm";

        case AlarmSuppressionStatus::AlreadySuppressed:
            return "already_suppressed";

        case AlarmSuppressionStatus::NotSuppressed:
            return "not_suppressed";

        case AlarmSuppressionStatus::Expired:
            return "expired";

        case AlarmSuppressionStatus::InvalidCommand:
            return "invalid_command";
        }

        return "invalid_command";
    }

    bool is_success(AlarmSuppressionStatus status) noexcept
    {
        return status == AlarmSuppressionStatus::Applied
            || status == AlarmSuppressionStatus::Cleared;
    }

    bool is_failure(AlarmSuppressionStatus status) noexcept
    {
        return !is_success(status);
    }
}