#pragma once

#include <dispatcher/alarm/threshold_alarm_condition.hpp>
#include <dispatcher/domain/id_types.hpp>

namespace dispatcher::alarm
{
    class AlarmConditionDefinition
    {
    public:
        AlarmConditionDefinition(
            dispatcher::domain::AlarmId alarm_id,
            ThresholdAlarmCondition condition
        );

        [[nodiscard]] const dispatcher::domain::AlarmId& alarm_id() const noexcept;

        [[nodiscard]] const ThresholdAlarmCondition& condition() const noexcept;

    private:
        dispatcher::domain::AlarmId alarm_id_;
        ThresholdAlarmCondition condition_;
    };
}