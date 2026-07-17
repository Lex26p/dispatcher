#include <dispatcher/domain/device_catalog.hpp>

#include <dispatcher/domain/device_definition_validation.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace
{
    std::string make_area_local_name_key(
        const dispatcher::domain::OrganizationId& organization_id,
        const dispatcher::domain::SiteId& site_id,
        const dispatcher::domain::AreaId& area_id,
        std::string_view local_name
    )
    {
        return organization_id.value()
            + "/"
            + site_id.value()
            + "/"
            + area_id.value()
            + "/"
            + std::string(local_name);
    }
}

namespace dispatcher::domain
{
    dispatcher::common::ValidationResult DeviceCatalog::add(DeviceDefinition device_definition)
    {
        auto result = validate_device_definition(device_definition);

        const auto device_id = device_definition.device_id().value();

        const auto area_local_name_key = make_area_local_name_key(
            device_definition.organization_id(),
            device_definition.site_id(),
            device_definition.area_id(),
            device_definition.local_name()
        );

        if (by_id_.contains(device_id))
        {
            result.add_error(
                "device_id",
                "device_id must be unique"
            );
        }

        if (by_area_local_name_.contains(area_local_name_key))
        {
            result.add_error(
                "local_name",
                "local_name must be unique within area"
            );
        }

        if (result.has_errors())
        {
            return result;
        }

        const auto index = devices_.size();

        devices_.push_back(std::move(device_definition));
        by_id_.emplace(device_id, index);
        by_area_local_name_.emplace(area_local_name_key, index);

        return result;
    }

    std::optional<DeviceDefinition> DeviceCatalog::find_by_id(const DeviceId& device_id) const
    {
        const auto it = by_id_.find(device_id.value());

        if (it == by_id_.end())
        {
            return std::nullopt;
        }

        return devices_[it->second];
    }

    std::optional<DeviceDefinition> DeviceCatalog::find_by_area_local_name(
        const OrganizationId& organization_id,
        const SiteId& site_id,
        const AreaId& area_id,
        std::string_view local_name
    ) const
    {
        const auto key = make_area_local_name_key(
            organization_id,
            site_id,
            area_id,
            local_name
        );

        const auto it = by_area_local_name_.find(key);

        if (it == by_area_local_name_.end())
        {
            return std::nullopt;
        }

        return devices_[it->second];
    }

    bool DeviceCatalog::contains(const DeviceId& device_id) const
    {
        return by_id_.contains(device_id.value());
    }

    std::size_t DeviceCatalog::size() const noexcept
    {
        return devices_.size();
    }

    bool DeviceCatalog::empty() const noexcept
    {
        return devices_.empty();
    }

    const std::vector<DeviceDefinition>& DeviceCatalog::devices() const noexcept
    {
        return devices_;
    }
}