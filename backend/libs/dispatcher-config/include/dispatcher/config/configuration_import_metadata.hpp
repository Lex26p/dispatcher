#pragma once

#include <dispatcher/config/configuration_format.hpp>

#include <cstdint>
#include <string>

namespace dispatcher::config
{
    struct ConfigurationImportMetadata
    {
        std::string schema_version{ "dispatcher.config.v1" };
        ConfigurationFormat format{ ConfigurationFormat::Json };

        std::uint64_t config_version{ 0 };
        std::string status;
        std::string description;

        std::string source;
        std::string imported_at;

        [[nodiscard]] bool has_schema_version() const noexcept;

        [[nodiscard]] bool has_known_format() const noexcept;

        [[nodiscard]] bool has_config_version() const noexcept;

        [[nodiscard]] bool has_status() const noexcept;

        [[nodiscard]] bool has_description() const noexcept;

        [[nodiscard]] bool has_source() const noexcept;

        [[nodiscard]] bool has_imported_at() const noexcept;
    };
}