#pragma once

#include <dispatcher/common/validation_result.hpp>
#include <dispatcher/domain/device_definition.hpp>

namespace dispatcher::domain
{
    [[nodiscard]] dispatcher::common::ValidationResult validate_device_definition(
        const DeviceDefinition& device_definition
    );
}