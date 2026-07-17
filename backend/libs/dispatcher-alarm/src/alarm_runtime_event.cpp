#include <dispatcher/alarm/alarm_runtime_event.hpp>

#include <utility>

namespace dispatcher::alarm
{
    AlarmRuntimeEvent::AlarmRuntimeEvent(
        dispatcher::domain::AlarmId alarm_id,
        dispatcher::domain::TagId tag_id,
        AlarmSeverity severity,
        AlarmTransitionType transition_type,
        AlarmState previous_state,
        AlarmState new_state,
        TimePoint source_timestamp,
        TimePoint event_timestamp,
        std::uint64_t sequence
    )
        : alarm_id_(std::move(alarm_id))
        , tag_id_(std::move(tag_id))
        , severity_(severity)
        , transition_type_(transition_type)
        , previous_state_(previous_state)
        , new_state_(new_state)
        , source_timestamp_(source_timestamp)
        , event_timestamp_(event_timestamp)
        , sequence_(sequence)
    {
    }

    const dispatcher::domain::AlarmId& AlarmRuntimeEvent::alarm_id() const noexcept
    {
        return alarm_id_;
    }

    const dispatcher::domain::TagId& AlarmRuntimeEvent::tag_id() const noexcept
    {
        return tag_id_;
    }

    AlarmSeverity AlarmRuntimeEvent::severity() const noexcept
    {
        return severity_;
    }

    AlarmTransitionType AlarmRuntimeEvent::transition_type() const noexcept
    {
        return transition_type_;
    }

    AlarmState AlarmRuntimeEvent::previous_state() const noexcept
    {
        return previous_state_;
    }

    AlarmState AlarmRuntimeEvent::new_state() const noexcept
    {
        return new_state_;
    }

    AlarmRuntimeEvent::TimePoint AlarmRuntimeEvent::source_timestamp() const noexcept
    {
        return source_timestamp_;
    }

    AlarmRuntimeEvent::TimePoint AlarmRuntimeEvent::event_timestamp() const noexcept
    {
        return event_timestamp_;
    }

    std::uint64_t AlarmRuntimeEvent::sequence() const noexcept
    {
        return sequence_;
    }
}