#pragma once

#include <dispatcher/common/validation_result.hpp>
#include <dispatcher/domain/device_definition.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dispatcher::domain
{
    class DeviceCatalog
    {
    public:
        [[nodiscard]] dispatcher::common::ValidationResult add(DeviceDefinition device_definition);

        [[nodiscard]] std::optional<DeviceDefinition> find_by_id(const DeviceId& device_id) const;

        [[nodiscard]] std::optional<DeviceDefinition> find_by_area_local_name(
            const OrganizationId& organization_id,
            const SiteId& site_id,
            const AreaId& area_id,
            std::string_view local_name
        ) const;

        [[nodiscard]] bool contains(const DeviceId& device_id) const;

        [[nodiscard]] std::size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] const std::vector<DeviceDefinition>& devices() const noexcept;

    private:
        std::vector<DeviceDefinition> devices_;
        std::unordered_map<std::string, std::size_t> by_id_;
        std::unordered_map<std::string, std::size_t> by_area_local_name_;
    };
}