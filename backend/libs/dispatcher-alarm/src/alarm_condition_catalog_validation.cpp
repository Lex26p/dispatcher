#include <dispatcher/alarm/alarm_condition_catalog_validation.hpp>

#include <dispatcher/alarm/alarm_condition_definition_validation.hpp>

#include <string>
#include <unordered_set>

namespace dispatcher::alarm
{
    dispatcher::common::ValidationResult validate_alarm_condition_catalog(
        const AlarmConditionCatalog& catalog
    )
    {
        dispatcher::common::ValidationResult result;

        std::unordered_set<std::string> alarm_ids;

        for (const auto& definition : catalog.definitions())
        {
            const auto definition_validation =
                validate_alarm_condition_definition(definition);

            for (const auto& error : definition_validation.errors())
            {
                result.add_error(
                    "alarm_condition_definition." + error.field,
                    error.message
                );
            }

            const auto alarm_id = definition.alarm_id().value();

            if (!alarm_id.empty() && alarm_ids.contains(alarm_id))
            {
                result.add_error(
                    "alarm_id",
                    "alarm_id must be unique"
                );
            }

            alarm_ids.insert(alarm_id);
        }

        return result;
    }
}