#pragma once

#include <dispatcher/alarm/alarm_definition.hpp>
#include <dispatcher/common/validation_result.hpp>

namespace dispatcher::alarm
{
    [[nodiscard]] dispatcher::common::ValidationResult validate_alarm_definition(
        const AlarmDefinition& alarm_definition
    );
}