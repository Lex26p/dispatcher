#include <dispatcher/alarm/alarm_catalog_validation.hpp>

#include <dispatcher/alarm/alarm_definition_validation.hpp>

#include <string>
#include <unordered_set>

namespace dispatcher::alarm
{
    dispatcher::common::ValidationResult validate_alarm_catalog(
        const AlarmCatalog& catalog
    )
    {
        dispatcher::common::ValidationResult result;

        std::unordered_set<std::string> alarm_ids;
        std::unordered_set<std::string> tag_name_keys;

        for (const auto& definition : catalog.definitions())
        {
            const auto definition_validation = validate_alarm_definition(definition);

            for (const auto& error : definition_validation.errors())
            {
                result.add_error(
                    "alarm_definition." + error.field,
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

            const auto tag_name_key =
                definition.tag_id().value() + "/" + definition.name();

            if (!definition.tag_id().empty()
                && !definition.name().empty()
                && tag_name_keys.contains(tag_name_key))
            {
                result.add_error(
                    "tag_id.name",
                    "alarm name must be unique within tag"
                );
            }

            tag_name_keys.insert(tag_name_key);
        }

        return result;
    }
}