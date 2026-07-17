#pragma once

namespace dispatcher::alarm
{
    enum class AlarmSuppressionMode
    {
        Shelved,
        Suppressed,
        Inhibited
    };

    [[nodiscard]] const char* to_string(
        AlarmSuppressionMode mode
    ) noexcept;

    [[nodiscard]] bool is_operator_controlled(
        AlarmSuppressionMode mode
    ) noexcept;

    [[nodiscard]] bool is_system_controlled(
        AlarmSuppressionMode mode
    ) noexcept;
}