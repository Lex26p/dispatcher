#include <dispatcher/alarm/alarm_suppression_command.hpp>

#include <utility>

namespace dispatcher::alarm
{
    AlarmSuppressionCommand::AlarmSuppressionCommand(
        dispatcher::domain::AlarmId alarm_id,
        std::string operator_id,
        AlarmSuppressionMode mode,
        AlarmSuppressionReason reason,
        std::string comment,
        std::optional<TimePoint> expires_at
    )
        : alarm_id_(std::move(alarm_id))
        , operator_id_(std::move(operator_id))
        , mode_(mode)
        , reason_(reason)
        , comment_(std::move(comment))
        , expires_at_(std::move(expires_at))
    {
    }

    const dispatcher::domain::AlarmId& AlarmSuppressionCommand::alarm_id()
        const noexcept
    {
        return alarm_id_;
    }

    const std::string& AlarmSuppressionCommand::operator_id() const noexcept
    {
        return operator_id_;
    }

    AlarmSuppressionMode AlarmSuppressionCommand::mode() const noexcept
    {
        return mode_;
    }

    AlarmSuppressionReason AlarmSuppressionCommand::reason() const noexcept
    {
        return reason_;
    }

    const std::string& AlarmSuppressionCommand::comment() const noexcept
    {
        return comment_;
    }

    const std::optional<AlarmSuppressionCommand::TimePoint>&
        AlarmSuppressionCommand::expires_at() const noexcept
    {
        return expires_at_;
    }

    bool AlarmSuppressionCommand::has_alarm_id() const noexcept
    {
        return !alarm_id_.value().empty();
    }

    bool AlarmSuppressionCommand::has_operator_id() const noexcept
    {
        return !operator_id_.empty();
    }

    bool AlarmSuppressionCommand::has_comment() const noexcept
    {
        return !comment_.empty();
    }

    bool AlarmSuppressionCommand::has_expiration() const noexcept
    {
        return expires_at_.has_value();
    }

    bool AlarmSuppressionCommand::expired_at(TimePoint now) const noexcept
    {
        return expires_at_.has_value() && expires_at_.value() <= now;
    }

    bool AlarmSuppressionCommand::valid() const noexcept
    {
        return has_alarm_id()
            && has_operator_id()
            && is_known_reason(reason_)
            && !expired_at(Clock::now());
    }
}