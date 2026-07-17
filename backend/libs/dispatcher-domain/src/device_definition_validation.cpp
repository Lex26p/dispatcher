#include <dispatcher/domain/device_definition_validation.hpp>

#include <string>

namespace dispatcher::domain
{
    dispatcher::common::ValidationResult validate_device_definition(
        const DeviceDefinition& device_definition
    )
    {
        dispatcher::common::ValidationResult result;

        if (device_definition.organization_id().empty())
        {
            result.add_error(
                "organization_id",
                "organization_id must not be empty"
            );
        }

        if (device_definition.site_id().empty())
        {
            result.add_error(
                "site_id",
                "site_id must not be empty"
            );
        }

        if (device_definition.area_id().empty())
        {
            result.add_error(
                "area_id",
                "area_id must not be empty"
            );
        }

        if (device_definition.device_id().empty())
        {
            result.add_error(
                "device_id",
                "device_id must not be empty"
            );
        }

        if (device_definition.local_name().empty())
        {
            result.add_error(
                "local_name",
                "local_name must not be empty"
            );
        }

        if (device_definition.local_name().find('/') != std::string::npos)
        {
            result.add_error(
                "local_name",
                "local_name must not contain '/'"
            );
        }

        if (device_definition.config_version() == 0)
        {
            result.add_error(
                "config_version",
                "config_version must be greater than zero"
            );
        }

        return result;
    }
}