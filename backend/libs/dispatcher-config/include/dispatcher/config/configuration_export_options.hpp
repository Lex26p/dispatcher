#pragma once

#include <dispatcher/config/configuration_format.hpp>

#include <string>

namespace dispatcher::config
{
    struct ConfigurationExportOptions
    {
        ConfigurationFormat format{ ConfigurationFormat::Json };

        std::string source;
        std::string exported_at;

        bool include_devices{ true };
        bool include_tags{ true };

        [[nodiscard]] bool has_known_format() const noexcept;

        [[nodiscard]] bool has_source() const noexcept;

        [[nodiscard]] bool has_exported_at() const noexcept;

        [[nodiscard]] bool requests_devices() const noexcept;

        [[nodiscard]] bool requests_tags() const noexcept;
    };
}