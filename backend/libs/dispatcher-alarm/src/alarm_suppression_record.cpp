#include <dispatcher/alarm/alarm_suppression_record.hpp>

#include <utility>

namespace dispatcher::alarm
{
    AlarmSuppressionRecord::AlarmSuppressionRecord(
        dispatcher::domain::AlarmId alarm_id,
        std::string operator_id,
        AlarmSuppressionMode mode,
        AlarmSuppressionReason reason,
        TimePoint applied_at,
        std::string comment,
        std::optional<TimePoint> expires_at
    )
        : alarm_id_(std::move(alarm_id))
        , operator_id_(std::move(operator_id))
        , mode_(mode)
        , reason_(reason)
        , applied_at_(applied_at)
        , comment_(std::move(comment))
        , expires_at_(std::move(expires_at))
    {
    }

    AlarmSuppressionRecord AlarmSuppressionRecord::from_command(
        const AlarmSuppressionCommand& command,
        TimePoint applied_at
    )
    {
        return AlarmSuppressionRecord(
            command.alarm_id(),
            command.operator_id(),
            command.mode(),
            command.reason(),
            applied_at,
            command.comment(),
            command.expires_at()
        );
    }

    const dispatcher::domain::AlarmId& AlarmSuppressionRecord::alarm_id()
        const noexcept
    {
        return alarm_id_;
    }

    const std::string& AlarmSuppressionRecord::operator_id() const noexcept
    {
        return operator_id_;
    }

    AlarmSuppressionMode AlarmSuppressionRecord::mode() const noexcept
    {
        return mode_;
    }

    AlarmSuppressionReason AlarmSuppressionRecord::reason() const noexcept
    {
        return reason_;
    }

    AlarmSuppressionRecord::TimePoint AlarmSuppressionRecord::applied_at()
        const noexcept
    {
        return applied_at_;
    }

    const std::string& AlarmSuppressionRecord::comment() const noexcept
    {
        return comment_;
    }

    const std::optional<AlarmSuppressionRecord::TimePoint>&
        AlarmSuppressionRecord::expires_at() const noexcept
    {
        return expires_at_;
    }

    bool AlarmSuppressionRecord::has_comment() const noexcept
    {
        return !comment_.empty();
    }

    bool AlarmSuppressionRecord::has_expiration() const noexcept
    {
        return expires_at_.has_value();
    }

    bool AlarmSuppressionRecord::expired_at(TimePoint now) const noexcept
    {
        return expires_at_.has_value() && expires_at_.value() <= now;
    }

    bool AlarmSuppressionRecord::active_at(TimePoint now) const noexcept
    {
        return !expired_at(now);
    }
}