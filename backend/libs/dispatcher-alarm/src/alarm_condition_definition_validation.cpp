#include <dispatcher/alarm/alarm_condition_definition_validation.hpp>

#include <cmath>

namespace dispatcher::alarm
{
    dispatcher::common::ValidationResult validate_alarm_condition_definition(
        const AlarmConditionDefinition& definition
    )
    {
        dispatcher::common::ValidationResult result;

        if (definition.alarm_id().empty())
        {
            result.add_error(
                "alarm_id",
                "alarm_id must not be empty"
            );
        }

        if (!std::isfinite(definition.condition().threshold()))
        {
            result.add_error(
                "threshold",
                "threshold must be finite"
            );
        }

        return result;
    }
}