#pragma once

#include <string>

namespace dispatcher::config
{
    struct ConfigurationExportDevice
    {
        std::string organization_id;
        std::string site_id;
        std::string area_id;
        std::string device_id;

        std::string local_name;
        std::string display_name;

        bool enabled{ true };

        [[nodiscard]] bool has_organization_id() const noexcept;

        [[nodiscard]] bool has_site_id() const noexcept;

        [[nodiscard]] bool has_area_id() const noexcept;

        [[nodiscard]] bool has_device_id() const noexcept;

        [[nodiscard]] bool has_local_name() const noexcept;

        [[nodiscard]] bool has_display_name() const noexcept;

        [[nodiscard]] bool has_required_identity() const noexcept;
    };
}