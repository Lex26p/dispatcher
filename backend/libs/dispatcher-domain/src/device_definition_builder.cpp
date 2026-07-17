#include <dispatcher/domain/device_definition_builder.hpp>

#include <utility>

namespace dispatcher::domain
{
    DeviceDefinitionBuilder& DeviceDefinitionBuilder::organization_id(OrganizationId value)
    {
        organization_id_ = std::move(value);
        return *this;
    }

    DeviceDefinitionBuilder& DeviceDefinitionBuilder::site_id(SiteId value)
    {
        site_id_ = std::move(value);
        return *this;
    }

    DeviceDefinitionBuilder& DeviceDefinitionBuilder::area_id(AreaId value)
    {
        area_id_ = std::move(value);
        return *this;
    }

    DeviceDefinitionBuilder& DeviceDefinitionBuilder::device_id(DeviceId value)
    {
        device_id_ = std::move(value);
        return *this;
    }

    DeviceDefinitionBuilder& DeviceDefinitionBuilder::local_name(std::string value)
    {
        local_name_ = std::move(value);
        return *this;
    }

    DeviceDefinitionBuilder& DeviceDefinitionBuilder::display_name(std::string value)
    {
        display_name_ = std::move(value);
        return *this;
    }

    DeviceDefinitionBuilder& DeviceDefinitionBuilder::name(std::string value)
    {
        return local_name(std::move(value));
    }

    DeviceDefinitionBuilder& DeviceDefinitionBuilder::description(std::string value)
    {
        description_ = std::move(value);
        return *this;
    }

    DeviceDefinitionBuilder& DeviceDefinitionBuilder::enabled(bool value)
    {
        enabled_ = value;
        return *this;
    }

    DeviceDefinitionBuilder& DeviceDefinitionBuilder::config_version(std::uint64_t value)
    {
        config_version_ = value;
        return *this;
    }

    DeviceDefinition DeviceDefinitionBuilder::build() const
    {
        return DeviceDefinition(
            organization_id_,
            site_id_,
            area_id_,
            device_id_,
            local_name_,
            description_,
            enabled_,
            config_version_,
            display_name_
        );
    }
}