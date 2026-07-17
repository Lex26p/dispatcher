#include <dispatcher/alarm/alarm_condition_definition.hpp>

#include <utility>

namespace dispatcher::alarm
{
    AlarmConditionDefinition::AlarmConditionDefinition(
        dispatcher::domain::AlarmId alarm_id,
        ThresholdAlarmCondition condition
    )
        : alarm_id_(std::move(alarm_id))
        , condition_(condition)
    {
    }

    const dispatcher::domain::AlarmId& AlarmConditionDefinition::alarm_id()
        const noexcept
    {
        return alarm_id_;
    }

    const ThresholdAlarmCondition& AlarmConditionDefinition::condition()
        const noexcept
    {
        return condition_;
    }
}