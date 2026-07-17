#pragma once

namespace dispatcher::alarm
{
    enum class AlarmSuppressionReason
    {
        Maintenance,
        Nuisance,
        Testing,
        Commissioning,
        OperatorDecision,
        ExternalInterlock,
        Unknown
    };

    [[nodiscard]] const char* to_string(
        AlarmSuppressionReason reason
    ) noexcept;

    [[nodiscard]] bool is_known_reason(
        AlarmSuppressionReason reason
    ) noexcept;
}