#include <dispatcher/alarm/alarm_acknowledgement_record.hpp>

#include <utility>

namespace dispatcher::alarm
{
    AlarmAcknowledgementRecord::AlarmAcknowledgementRecord(
        dispatcher::domain::AlarmId alarm_id,
        std::string operator_id,
        std::string comment,
        AlarmAcknowledgementStatus status,
        AlarmState previous_state,
        AlarmState new_state,
        TimePoint timestamp,
        std::optional<std::uint64_t> event_sequence
    )
        : alarm_id_(std::move(alarm_id))
        , operator_id_(std::move(operator_id))
        , comment_(std::move(comment))
        , status_(status)
        , previous_state_(previous_state)
        , new_state_(new_state)
        , timestamp_(timestamp)
        , event_sequence_(event_sequence)
    {
    }

    const dispatcher::domain::AlarmId& AlarmAcknowledgementRecord::alarm_id()
        const noexcept
    {
        return alarm_id_;
    }

    const std::string& AlarmAcknowledgementRecord::operator_id() const noexcept
    {
        return operator_id_;
    }

    const std::string& AlarmAcknowledgementRecord::comment() const noexcept
    {
        return comment_;
    }

    AlarmAcknowledgementStatus AlarmAcknowledgementRecord::status()
        const noexcept
    {
        return status_;
    }

    bool AlarmAcknowledgementRecord::acknowledged() const noexcept
    {
        return status_ == AlarmAcknowledgementStatus::Acknowledged;
    }

    bool AlarmAcknowledgementRecord::skipped() const noexcept
    {
        return !acknowledged();
    }

    AlarmState AlarmAcknowledgementRecord::previous_state() const noexcept
    {
        return previous_state_;
    }

    AlarmState AlarmAcknowledgementRecord::new_state() const noexcept
    {
        return new_state_;
    }

    AlarmAcknowledgementRecord::TimePoint AlarmAcknowledgementRecord::timestamp()
        const noexcept
    {
        return timestamp_;
    }

    const std::optional<std::uint64_t>&
        AlarmAcknowledgementRecord::event_sequence() const noexcept
    {
        return event_sequence_;
    }
}