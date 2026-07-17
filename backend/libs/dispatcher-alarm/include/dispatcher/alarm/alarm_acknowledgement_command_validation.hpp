#pragma once

#include <dispatcher/alarm/alarm_acknowledgement_command.hpp>
#include <dispatcher/common/validation_result.hpp>

namespace dispatcher::alarm
{
    [[nodiscard]] dispatcher::common::ValidationResult
        validate_alarm_acknowledgement_command(
            const AlarmAcknowledgementCommand& command
        );
}