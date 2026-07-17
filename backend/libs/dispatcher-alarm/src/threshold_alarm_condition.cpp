#include <dispatcher/alarm/threshold_alarm_condition.hpp>

#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/telemetry/tag_value.hpp>

#include <cstdint>
#include <optional>

namespace
{
    std::optional<double> numeric_value(
        const dispatcher::telemetry::TagValue& value
    )
    {
        using dispatcher::domain::DataType;

        switch (value.type())
        {
        case DataType::Int32:
            return static_cast<double>(value.as<std::int32_t>());

        case DataType::Int64:
            return static_cast<double>(value.as<std::int64_t>());

        case DataType::Float32:
            return static_cast<double>(value.as<float>());

        case DataType::Float64:
            return value.as<double>();

        case DataType::Boolean:
        case DataType::String:
            return std::nullopt;
        }

        return std::nullopt;
    }
}

namespace dispatcher::alarm
{
    ThresholdAlarmEvaluationResult::ThresholdAlarmEvaluationResult(
        ThresholdAlarmEvaluationStatus status,
        bool active
    )
        : status_(status)
        , active_(active)
    {
    }

    ThresholdAlarmEvaluationStatus ThresholdAlarmEvaluationResult::status()
        const noexcept
    {
        return status_;
    }

    bool ThresholdAlarmEvaluationResult::evaluated() const noexcept
    {
        return status_ == ThresholdAlarmEvaluationStatus::Evaluated;
    }

    bool ThresholdAlarmEvaluationResult::active() const noexcept
    {
        return evaluated() && active_;
    }

    bool ThresholdAlarmEvaluationResult::normal() const noexcept
    {
        return evaluated() && !active_;
    }

    ThresholdAlarmCondition::ThresholdAlarmCondition(
        AlarmConditionType condition_type,
        double threshold
    )
        : condition_type_(condition_type)
        , threshold_(threshold)
    {
    }

    AlarmConditionType ThresholdAlarmCondition::condition_type() const noexcept
    {
        return condition_type_;
    }

    double ThresholdAlarmCondition::threshold() const noexcept
    {
        return threshold_;
    }

    ThresholdAlarmEvaluationResult ThresholdAlarmCondition::evaluate(
        const dispatcher::telemetry::TelemetryValue& telemetry_value
    ) const
    {
        const auto value = numeric_value(telemetry_value.value());

        if (!value.has_value())
        {
            return ThresholdAlarmEvaluationResult(
                ThresholdAlarmEvaluationStatus::UnsupportedValueType,
                false
            );
        }

        using dispatcher::alarm::AlarmConditionType;

        switch (condition_type_)
        {
        case AlarmConditionType::High:
        case AlarmConditionType::HighHigh:
            return ThresholdAlarmEvaluationResult(
                ThresholdAlarmEvaluationStatus::Evaluated,
                value.value() > threshold_
            );

        case AlarmConditionType::Low:
        case AlarmConditionType::LowLow:
            return ThresholdAlarmEvaluationResult(
                ThresholdAlarmEvaluationStatus::Evaluated,
                value.value() < threshold_
            );
        }

        return ThresholdAlarmEvaluationResult(
            ThresholdAlarmEvaluationStatus::UnsupportedValueType,
            false
        );
    }
}