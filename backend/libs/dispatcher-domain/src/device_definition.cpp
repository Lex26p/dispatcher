#include <dispatcher/domain/device_definition.hpp>

#include <utility>

namespace dispatcher::domain
{
    DeviceDefinition::DeviceDefinition(
        OrganizationId organization_id,
        SiteId site_id,
        AreaId area_id,
        DeviceId device_id,
        std::string local_name,
        std::string description,
        bool enabled,
        std::uint64_t config_version,
        std::string display_name
    )
        : organization_id_(std::move(organization_id))
        , site_id_(std::move(site_id))
        , area_id_(std::move(area_id))
        , device_id_(std::move(device_id))
        , local_name_(std::move(local_name))
        , display_name_(std::move(display_name))
        , description_(std::move(description))
        , enabled_(enabled)
        , config_version_(config_version)
    {
    }

    const OrganizationId& DeviceDefinition::organization_id() const noexcept
    {
        return organization_id_;
    }

    const SiteId& DeviceDefinition::site_id() const noexcept
    {
        return site_id_;
    }

    const AreaId& DeviceDefinition::area_id() const noexcept
    {
        return area_id_;
    }

    const DeviceId& DeviceDefinition::device_id() const noexcept
    {
        return device_id_;
    }

    const std::string& DeviceDefinition::local_name() const noexcept
    {
        return local_name_;
    }

    const std::string& DeviceDefinition::display_name() const noexcept
    {
        return display_name_;
    }

    const std::string& DeviceDefinition::name() const noexcept
    {
        return local_name_;
    }

    const std::string& DeviceDefinition::description() const noexcept
    {
        return description_;
    }

    bool DeviceDefinition::enabled() const noexcept
    {
        return enabled_;
    }

    std::uint64_t DeviceDefinition::config_version() const noexcept
    {
        return config_version_;
    }
}