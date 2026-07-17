#pragma once

#include <string>

namespace dispatcher::config
{
    struct ConfigurationExportTag
    {
        std::string organization_id;
        std::string site_id;
        std::string area_id;
        std::string device_id;
        std::string tag_id;

        std::string local_name;
        std::string display_name;

        std::string data_type;
        std::string history_policy;

        bool enabled{ true };

        [[nodiscard]] bool has_organization_id() const noexcept;

        [[nodiscard]] bool has_site_id() const noexcept;

        [[nodiscard]] bool has_area_id() const noexcept;

        [[nodiscard]] bool has_device_id() const noexcept;

        [[nodiscard]] bool has_tag_id() const noexcept;

        [[nodiscard]] bool has_local_name() const noexcept;

        [[nodiscard]] bool has_display_name() const noexcept;

        [[nodiscard]] bool has_data_type() const noexcept;

        [[nodiscard]] bool has_history_policy() const noexcept;

        [[nodiscard]] bool has_required_identity() const noexcept;
    };
}