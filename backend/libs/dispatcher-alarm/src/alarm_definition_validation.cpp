#include <dispatcher/alarm/alarm_definition_validation.hpp>

#include <string>

namespace dispatcher::alarm
{
    dispatcher::common::ValidationResult validate_alarm_definition(
        const AlarmDefinition& alarm_definition
    )
    {
        dispatcher::common::ValidationResult result;

        if (alarm_definition.alarm_id().empty())
        {
            result.add_error("alarm_id", "alarm_id must not be empty");
        }

        if (alarm_definition.tag_id().empty())
        {
            result.add_error("tag_id", "tag_id must not be empty");
        }

        if (alarm_definition.name().empty())
        {
            result.add_error("name", "name must not be empty");
        }

        if (alarm_definition.name().find('/') != std::string::npos)
        {
            result.add_error("name", "name must not contain '/'");
        }

        if (alarm_definition.config_version() == 0)
        {
            result.add_error(
                "config_version",
                "config_version must be greater than zero"
            );
        }

        return result;
    }
}