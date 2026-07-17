#pragma once

#include <dispatcher/alarm/alarm_definition.hpp>
#include <dispatcher/alarm/threshold_alarm_condition.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

namespace dispatcher::alarm
{
    struct AlarmEvaluationCandidate
    {
        AlarmDefinition alarm_definition;
        ThresholdAlarmCondition condition;
        dispatcher::telemetry::TelemetryValue telemetry_value;
    };
}