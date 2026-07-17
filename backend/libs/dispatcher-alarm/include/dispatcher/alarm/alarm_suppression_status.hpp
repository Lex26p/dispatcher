#pragma once

namespace dispatcher::alarm
{
    enum class AlarmSuppressionStatus
    {
        Applied,
        Cleared,

        UnknownAlarm,
        AlreadySuppressed,
        NotSuppressed,
        Expired,
        InvalidCommand
    };

    [[nodiscard]] const char* to_string(
        AlarmSuppressionStatus status
    ) noexcept;

    [[nodiscard]] bool is_success(
        AlarmSuppressionStatus status
    ) noexcept;

    [[nodiscard]] bool is_failure(
        AlarmSuppressionStatus status
    ) noexcept;
}