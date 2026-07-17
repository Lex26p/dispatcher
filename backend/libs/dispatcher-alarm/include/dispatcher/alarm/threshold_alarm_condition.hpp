#pragma once

#include <dispatcher/alarm/alarm_condition_type.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <string_view>

namespace dispatcher::alarm
{
    enum class ThresholdAlarmEvaluationStatus
    {
        Evaluated,
        UnsupportedValueType
    };

    constexpr std::string_view to_string(
        ThresholdAlarmEvaluationStatus status
    )
    {
        switch (status)
        {
        case ThresholdAlarmEvaluationStatus::Evaluated:
            return "evaluated";
        case ThresholdAlarmEvaluationStatus::UnsupportedValueType:
            return "unsupported_value_type";
        }

        return "unknown";
    }

    class ThresholdAlarmEvaluationResult
    {
    public:
        ThresholdAlarmEvaluationResult(
            ThresholdAlarmEvaluationStatus status,
            bool active
        );

        [[nodiscard]] ThresholdAlarmEvaluationStatus status() const noexcept;

        [[nodiscard]] bool evaluated() const noexcept;

        [[nodiscard]] bool active() const noexcept;

        [[nodiscard]] bool normal() const noexcept;

    private:
        ThresholdAlarmEvaluationStatus status_;
        bool active_;
    };

    class ThresholdAlarmCondition
    {
    public:
        ThresholdAlarmCondition(
            AlarmConditionType condition_type,
            double threshold
        );

        [[nodiscard]] AlarmConditionType condition_type() const noexcept;

        [[nodiscard]] double threshold() const noexcept;

        [[nodiscard]] ThresholdAlarmEvaluationResult evaluate(
            const dispatcher::telemetry::TelemetryValue& telemetry_value
        ) const;

    private:
        AlarmConditionType condition_type_;
        double threshold_;
    };
}