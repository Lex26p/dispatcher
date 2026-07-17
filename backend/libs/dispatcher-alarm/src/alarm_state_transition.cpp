#include <dispatcher/alarm/alarm_state_transition.hpp>

#include <optional>

namespace
{
    dispatcher::alarm::AlarmState next_state_for(
        dispatcher::alarm::AlarmState previous_state,
        bool condition_active
    )
    {
        using dispatcher::alarm::AlarmState;

        if (condition_active)
        {
            if (previous_state == AlarmState::Normal)
            {
                return AlarmState::Active;
            }

            return previous_state;
        }

        return AlarmState::Normal;
    }

    dispatcher::alarm::AlarmTransitionType transition_type_for(
        dispatcher::alarm::AlarmState previous_state,
        dispatcher::alarm::AlarmState new_state
    )
    {
        using dispatcher::alarm::AlarmState;
        using dispatcher::alarm::AlarmTransitionType;

        if (previous_state == AlarmState::Normal
            && new_state == AlarmState::Active)
        {
            return AlarmTransitionType::Activated;
        }

        if ((previous_state == AlarmState::Active
            || previous_state == AlarmState::Acknowledged)
            && new_state == AlarmState::Normal)
        {
            return AlarmTransitionType::Cleared;
        }

        return AlarmTransitionType::None;
    }
}

namespace dispatcher::alarm
{
    AlarmStateTransitionResult::AlarmStateTransitionResult(
        AlarmTransitionType transition_type,
        AlarmState previous_state,
        AlarmState new_state,
        std::optional<AlarmRuntimeEvent> event
    )
        : transition_type_(transition_type)
        , previous_state_(previous_state)
        , new_state_(new_state)
        , event_(std::move(event))
    {
    }

    AlarmTransitionType AlarmStateTransitionResult::transition_type() const noexcept
    {
        return transition_type_;
    }

    AlarmState AlarmStateTransitionResult::previous_state() const noexcept
    {
        return previous_state_;
    }

    AlarmState AlarmStateTransitionResult::new_state() const noexcept
    {
        return new_state_;
    }

    bool AlarmStateTransitionResult::transitioned() const noexcept
    {
        return transition_type_ != AlarmTransitionType::None;
    }

    bool AlarmStateTransitionResult::activated() const noexcept
    {
        return transition_type_ == AlarmTransitionType::Activated;
    }

    bool AlarmStateTransitionResult::cleared() const noexcept
    {
        return transition_type_ == AlarmTransitionType::Cleared;
    }

    const std::optional<AlarmRuntimeEvent>& AlarmStateTransitionResult::event()
        const noexcept
    {
        return event_;
    }

    AlarmStateTransitionResult evaluate_alarm_state_transition(
        const AlarmDefinition& alarm_definition,
        AlarmState previous_state,
        bool condition_active,
        const dispatcher::telemetry::TelemetryValue& telemetry_value
    )
    {
        const auto new_state = next_state_for(previous_state, condition_active);
        const auto transition_type = transition_type_for(previous_state, new_state);

        std::optional<AlarmRuntimeEvent> event;

        if (transition_type != AlarmTransitionType::None)
        {
            event = AlarmRuntimeEvent(
                alarm_definition.alarm_id(),
                alarm_definition.tag_id(),
                alarm_definition.severity(),
                transition_type,
                previous_state,
                new_state,
                telemetry_value.source_timestamp(),
                telemetry_value.ingest_timestamp(),
                telemetry_value.sequence()
            );
        }

        return AlarmStateTransitionResult(
            transition_type,
            previous_state,
            new_state,
            std::move(event)
        );
    }
}