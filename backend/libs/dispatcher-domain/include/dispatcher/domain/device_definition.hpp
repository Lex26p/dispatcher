#pragma once

#include <dispatcher/domain/id_types.hpp>

#include <cstdint>
#include <string>

namespace dispatcher::domain
{
    class DeviceDefinition
    {
    public:
        DeviceDefinition(
            OrganizationId organization_id,
            SiteId site_id,
            AreaId area_id,
            DeviceId device_id,
            std::string local_name,
            std::string description,
            bool enabled,
            std::uint64_t config_version,
            std::string display_name = {}
        );

        [[nodiscard]] const OrganizationId& organization_id() const noexcept;
        [[nodiscard]] const SiteId& site_id() const noexcept;
        [[nodiscard]] const AreaId& area_id() const noexcept;
        [[nodiscard]] const DeviceId& device_id() const noexcept;

        [[nodiscard]] const std::string& local_name() const noexcept;
        [[nodiscard]] const std::string& display_name() const noexcept;
        [[nodiscard]] const std::string& name() const noexcept;
        [[nodiscard]] const std::string& description() const noexcept;

        [[nodiscard]] bool enabled() const noexcept;
        [[nodiscard]] std::uint64_t config_version() const noexcept;

    private:
        OrganizationId organization_id_;
        SiteId site_id_;
        AreaId area_id_;
        DeviceId device_id_;

        std::string local_name_;
        std::string display_name_;
        std::string description_;

        bool enabled_;
        std::uint64_t config_version_;
    };
}