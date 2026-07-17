#include <dispatcher/alarm/alarm_configuration_snapshot_validation.hpp>

#include <dispatcher/alarm/alarm_catalog_validation.hpp>
#include <dispatcher/alarm/alarm_condition_catalog_validation.hpp>

#include <string>

namespace dispatcher::alarm
{
    dispatcher::common::ValidationResult validate_alarm_configuration_snapshot(
        const AlarmConfigurationSnapshot& snapshot
    )
    {
        dispatcher::common::ValidationResult result;

        if (snapshot.config_version() == 0)
        {
            result.add_error(
                "config_version",
                "config_version must be greater than zero"
            );
        }

        if (snapshot.metadata().name.empty())
        {
            result.add_error(
                "metadata.name",
                "metadata name must not be empty"
            );
        }

        if (snapshot.metadata().name.find('/') != std::string::npos)
        {
            result.add_error(
                "metadata.name",
                "metadata name must not contain '/'"
            );
        }

        const auto alarm_catalog_validation = validate_alarm_catalog(
            snapshot.alarm_catalog()
        );

        for (const auto& error : alarm_catalog_validation.errors())
        {
            result.add_error(
                "alarm_catalog." + error.field,
                error.message
            );
        }

        const auto condition_catalog_validation =
            validate_alarm_condition_catalog(
                snapshot.condition_catalog()
            );

        for (const auto& error : condition_catalog_validation.errors())
        {
            result.add_error(
                "condition_catalog." + error.field,
                error.message
            );
        }

        for (const auto& alarm_definition : snapshot.alarm_catalog().definitions())
        {
            const auto condition_definition =
                snapshot.condition_catalog().find_by_alarm_id(
                    alarm_definition.alarm_id()
                );

            if (!condition_definition.has_value())
            {
                result.add_error(
                    "condition_catalog",
                    "alarm definition must have condition"
                );
            }
        }

        for (const auto& condition_definition :
            snapshot.condition_catalog().definitions())
        {
            const auto alarm_definition = snapshot.alarm_catalog().find_by_alarm_id(
                condition_definition.alarm_id()
            );

            if (!alarm_definition.has_value())
            {
                result.add_error(
                    "condition_catalog.alarm_id",
                    "condition must reference existing alarm definition"
                );
            }
        }

        return result;
    }
}