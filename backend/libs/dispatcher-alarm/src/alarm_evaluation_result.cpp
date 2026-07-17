#include <dispatcher/alarm/alarm_evaluation_result.hpp>

#include <utility>

namespace dispatcher::alarm
{
    AlarmEvaluationResult::AlarmEvaluationResult(
        AlarmEvaluationStatus status,
        AlarmTransitionType transition_type,
        AlarmState previous_state,
        AlarmState new_state,
        bool condition_active,
        std::optional<AlarmRuntimeEvent> event
    )
        : status_(status)
        , transition_type_(transition_type)
        , previous_state_(previous_state)
        , new_state_(new_state)
        , condition_active_(condition_active)
        , event_(std::move(event))
    {
    }

    AlarmEvaluationStatus AlarmEvaluationResult::status() const noexcept
    {
        return status_;
    }

    bool AlarmEvaluationResult::evaluated() const noexcept
    {
        return status_ == AlarmEvaluationStatus::Evaluated;
    }

    bool AlarmEvaluationResult::skipped() const noexcept
    {
        return !evaluated();
    }

    AlarmTransitionType AlarmEvaluationResult::transition_type() const noexcept
    {
        return transition_type_;
    }

    AlarmState AlarmEvaluationResult::previous_state() const noexcept
    {
        return previous_state_;
    }

    AlarmState AlarmEvaluationResult::new_state() const noexcept
    {
        return new_state_;
    }

    bool AlarmEvaluationResult::condition_active() const noexcept
    {
        return condition_active_;
    }

    bool AlarmEvaluationResult::transitioned() const noexcept
    {
        return transition_type_ != AlarmTransitionType::None;
    }

    bool AlarmEvaluationResult::activated() const noexcept
    {
        return transition_type_ == AlarmTransitionType::Activated;
    }

    bool AlarmEvaluationResult::acknowledged() const noexcept
    {
        return transition_type_ == AlarmTransitionType::Acknowledged;
    }

    bool AlarmEvaluationResult::cleared() const noexcept
    {
        return transition_type_ == AlarmTransitionType::Cleared;
    }

    const std::optional<AlarmRuntimeEvent>& AlarmEvaluationResult::event()
        const noexcept
    {
        return event_;
    }
}