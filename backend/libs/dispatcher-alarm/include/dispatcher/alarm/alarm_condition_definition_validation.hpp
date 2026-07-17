#pragma once

#include <dispatcher/alarm/alarm_condition_definition.hpp>
#include <dispatcher/common/validation_result.hpp>

namespace dispatcher::alarm
{
    [[nodiscard]] dispatcher::common::ValidationResult
        validate_alarm_condition_definition(
            const AlarmConditionDefinition& definition
        );
}