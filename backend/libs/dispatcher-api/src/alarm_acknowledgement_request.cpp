#include <dispatcher/api/alarm_acknowledgement_request.hpp>

namespace dispatcher::api
{
    bool AlarmAcknowledgementRequest::has_alarm_id() const noexcept
    {
        return !alarm_id.value().empty();
    }

    bool AlarmAcknowledgementRequest::has_operator_id() const noexcept
    {
        return !operator_id.empty();
    }

    bool AlarmAcknowledgementRequest::has_comment() const noexcept
    {
        return !comment.empty();
    }

    dispatcher::alarm::AlarmAcknowledgementCommand
        AlarmAcknowledgementRequest::to_command() const
    {
        return dispatcher::alarm::AlarmAcknowledgementCommand(
            alarm_id,
            operator_id,
            comment
        );
    }
}