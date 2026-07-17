#pragma once

#include <dispatcher/alarm/alarm_condition_catalog.hpp>
#include <dispatcher/common/validation_result.hpp>

namespace dispatcher::alarm
{
    [[nodiscard]] dispatcher::common::ValidationResult
        validate_alarm_condition_catalog(
            const AlarmConditionCatalog& catalog
        );
}