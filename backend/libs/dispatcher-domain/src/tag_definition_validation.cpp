#include <dispatcher/domain/tag_definition_validation.hpp>

#include <string>

namespace dispatcher::domain
{
    dispatcher::common::ValidationResult validate_tag_definition(
        const TagDefinition& tag_definition
    )
    {
        dispatcher::common::ValidationResult result;

        if (tag_definition.organization_id().empty())
        {
            result.add_error(
                "organization_id",
                "organization_id must not be empty"
            );
        }

        if (tag_definition.site_id().empty())
        {
            result.add_error(
                "site_id",
                "site_id must not be empty"
            );
        }

        if (tag_definition.area_id().empty())
        {
            result.add_error(
                "area_id",
                "area_id must not be empty"
            );
        }

        if (tag_definition.device_id().empty())
        {
            result.add_error(
                "device_id",
                "device_id must not be empty"
            );
        }

        if (tag_definition.tag_id().empty())
        {
            result.add_error(
                "tag_id",
                "tag_id must not be empty"
            );
        }

        if (tag_definition.local_name().empty())
        {
            result.add_error(
                "local_name",
                "local_name must not be empty"
            );
        }

        if (tag_definition.local_name().find('/') != std::string::npos)
        {
            result.add_error(
                "local_name",
                "local_name must not contain '/'"
            );
        }

        if (tag_definition.deadband().value() < 0.0)
        {
            result.add_error(
                "deadband",
                "deadband must be greater than or equal to zero"
            );
        }

        if (tag_definition.config_version() == 0)
        {
            result.add_error(
                "config_version",
                "config_version must be greater than zero"
            );
        }

        return result;
    }
}