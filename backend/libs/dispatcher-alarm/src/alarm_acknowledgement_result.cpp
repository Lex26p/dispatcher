#include <dispatcher/alarm/alarm_acknowledgement_result.hpp>

#include <utility>

namespace dispatcher::alarm
{
    AlarmAcknowledgementResult::AlarmAcknowledgementResult(
        AlarmAcknowledgementStatus status,
        AlarmState previous_state,
        AlarmState new_state,
        std::optional<AlarmRuntimeEvent> event
    )
        : status_(status)
        , previous_state_(previous_state)
        , new_state_(new_state)
        , event_(std::move(event))
    {
    }

    AlarmAcknowledgementStatus AlarmAcknowledgementResult::status()
        const noexcept
    {
        return status_;
    }

    bool AlarmAcknowledgementResult::acknowledged() const noexcept
    {
        return status_ == AlarmAcknowledgementStatus::Acknowledged;
    }

    bool AlarmAcknowledgementResult::skipped() const noexcept
    {
        return !acknowledged();
    }

    bool AlarmAcknowledgementResult::unknown_alarm() const noexcept
    {
        return status_ == AlarmAcknowledgementStatus::UnknownAlarm;
    }

    bool AlarmAcknowledgementResult::not_active() const noexcept
    {
        return status_ == AlarmAcknowledgementStatus::NotActive;
    }

    bool AlarmAcknowledgementResult::already_acknowledged() const noexcept
    {
        return status_ == AlarmAcknowledgementStatus::AlreadyAcknowledged;
    }

    bool AlarmAcknowledgementResult::invalid_command() const noexcept
    {
        return status_ == AlarmAcknowledgementStatus::InvalidCommand;
    }

    AlarmState AlarmAcknowledgementResult::previous_state() const noexcept
    {
        return previous_state_;
    }

    AlarmState AlarmAcknowledgementResult::new_state() const noexcept
    {
        return new_state_;
    }

    const std::optional<AlarmRuntimeEvent>& AlarmAcknowledgementResult::event()
        const noexcept
    {
        return event_;
    }
}