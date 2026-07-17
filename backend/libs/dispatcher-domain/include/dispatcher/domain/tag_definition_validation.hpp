#pragma once

#include <dispatcher/common/validation_result.hpp>
#include <dispatcher/domain/tag_definition.hpp>

namespace dispatcher::domain
{
    [[nodiscard]] dispatcher::common::ValidationResult validate_tag_definition(
        const TagDefinition& tag_definition
    );
}