#pragma once

#include <dispatcher/alarm/alarm_configuration_snapshot.hpp>
#include <dispatcher/common/validation_result.hpp>

namespace dispatcher::alarm
{
    [[nodiscard]] dispatcher::common::ValidationResult
        validate_alarm_configuration_snapshot(
            const AlarmConfigurationSnapshot& snapshot
        );
}