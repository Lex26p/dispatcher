#pragma once

#include <dispatcher/alarm/alarm_definition.hpp>
#include <dispatcher/alarm/alarm_evaluation_result.hpp>
#include <dispatcher/alarm/alarm_state_store.hpp>
#include <dispatcher/alarm/threshold_alarm_condition.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

namespace dispatcher::alarm
{
    class AlarmEvaluator
    {
    public:
        explicit AlarmEvaluator(AlarmStateStore& state_store);

        [[nodiscard]] AlarmEvaluationResult evaluate(
            const AlarmDefinition& alarm_definition,
            const ThresholdAlarmCondition& condition,
            const dispatcher::telemetry::TelemetryValue& telemetry_value
        );

        [[nodiscard]] const AlarmStateStore& state_store() const noexcept;

    private:
        AlarmStateStore& state_store_;
    };
}