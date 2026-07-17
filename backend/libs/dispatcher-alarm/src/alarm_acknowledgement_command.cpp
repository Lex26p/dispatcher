#include <dispatcher/alarm/alarm_acknowledgement_command.hpp>

#include <utility>

namespace dispatcher::alarm
{
    AlarmAcknowledgementCommand::AlarmAcknowledgementCommand(
        dispatcher::domain::AlarmId alarm_id,
        std::string operator_id,
        std::string comment
    )
        : alarm_id_(std::move(alarm_id))
        , operator_id_(std::move(operator_id))
        , comment_(std::move(comment))
    {
    }

    const dispatcher::domain::AlarmId& AlarmAcknowledgementCommand::alarm_id()
        const noexcept
    {
        return alarm_id_;
    }

    const std::string& AlarmAcknowledgementCommand::operator_id() const noexcept
    {
        return operator_id_;
    }

    const std::string& AlarmAcknowledgementCommand::comment() const noexcept
    {
        return comment_;
    }
}