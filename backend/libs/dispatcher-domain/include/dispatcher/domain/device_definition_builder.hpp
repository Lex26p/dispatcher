#pragma once

#include <dispatcher/domain/device_definition.hpp>

#include <cstdint>
#include <string>

namespace dispatcher::domain
{
    class DeviceDefinitionBuilder
    {
    public:
        DeviceDefinitionBuilder& organization_id(OrganizationId value);
        DeviceDefinitionBuilder& site_id(SiteId value);
        DeviceDefinitionBuilder& area_id(AreaId value);
        DeviceDefinitionBuilder& device_id(DeviceId value);

        DeviceDefinitionBuilder& local_name(std::string value);
        DeviceDefinitionBuilder& display_name(std::string value);
        DeviceDefinitionBuilder& name(std::string value);
        DeviceDefinitionBuilder& description(std::string value);

        DeviceDefinitionBuilder& enabled(bool value);
        DeviceDefinitionBuilder& config_version(std::uint64_t value);

        [[nodiscard]] DeviceDefinition build() const;

    private:
        OrganizationId organization_id_;
        SiteId site_id_;
        AreaId area_id_;
        DeviceId device_id_;

        std::string local_name_;
        std::string display_name_;
        std::string description_;

        bool enabled_{ true };
        std::uint64_t config_version_{ 1 };
    };
}