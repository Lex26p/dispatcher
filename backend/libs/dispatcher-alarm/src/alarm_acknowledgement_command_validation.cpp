#include <dispatcher/alarm/alarm_acknowledgement_command_validation.hpp>

namespace dispatcher::alarm
{
    dispatcher::common::ValidationResult
        validate_alarm_acknowledgement_command(
            const AlarmAcknowledgementCommand& command
        )
    {
        dispatcher::common::ValidationResult result;

        if (command.alarm_id().empty())
        {
            result.add_error(
                "alarm_id",
                "alarm acknowledgement command alarm_id must not be empty"
            );
        }

        if (command.operator_id().empty())
        {
            result.add_error(
                "operator_id",
                "alarm acknowledgement command operator_id must not be empty"
            );
        }

        return result;
    }
}