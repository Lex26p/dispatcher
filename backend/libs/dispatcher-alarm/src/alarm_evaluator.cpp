#include <dispatcher/alarm/alarm_evaluator.hpp>

#include <dispatcher/alarm/alarm_state_transition.hpp>

#include <optional>
#include <utility>

namespace dispatcher::alarm
{
    AlarmEvaluator::AlarmEvaluator(AlarmStateStore& state_store)
        : state_store_(state_store)
    {
    }

    AlarmEvaluationResult AlarmEvaluator::evaluate(
        const AlarmDefinition& alarm_definition,
        const ThresholdAlarmCondition& condition,
        const dispatcher::telemetry::TelemetryValue& telemetry_value
    )
    {
        const auto previous_state = state_store_.state_of(
            alarm_definition.alarm_id()
        );

        if (!alarm_definition.enabled())
        {
            return AlarmEvaluationResult(
                AlarmEvaluationStatus::DisabledAlarm,
                AlarmTransitionType::None,
                previous_state,
                previous_state,
                false,
                std::nullopt
            );
        }

        if (alarm_definition.tag_id() != telemetry_value.tag_id())
        {
            return AlarmEvaluationResult(
                AlarmEvaluationStatus::TagMismatch,
                AlarmTransitionType::None,
                previous_state,
                previous_state,
                false,
                std::nullopt
            );
        }

        const auto condition_result = condition.evaluate(telemetry_value);

        if (!condition_result.evaluated())
        {
            return AlarmEvaluationResult(
                AlarmEvaluationStatus::UnsupportedValueType,
                AlarmTransitionType::None,
                previous_state,
                previous_state,
                false,
                std::nullopt
            );
        }

        const auto transition_result = evaluate_alarm_state_transition(
            alarm_definition,
            previous_state,
            condition_result.active(),
            telemetry_value
        );

        state_store_.apply_transition(
            alarm_definition.alarm_id(),
            transition_result
        );

        return AlarmEvaluationResult(
            AlarmEvaluationStatus::Evaluated,
            transition_result.transition_type(),
            transition_result.previous_state(),
            transition_result.new_state(),
            condition_result.active(),
            transition_result.event()
        );
    }

    const AlarmStateStore& AlarmEvaluator::state_store() const noexcept
    {
        return state_store_;
    }
}