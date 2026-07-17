#pragma once

#include <dispatcher/common/validation_result.hpp>
#include <dispatcher/domain/configuration_snapshot.hpp>

namespace dispatcher::domain
{
    [[nodiscard]] dispatcher::common::ValidationResult validate_configuration_snapshot(
        const ConfigurationSnapshot& snapshot
    );
}