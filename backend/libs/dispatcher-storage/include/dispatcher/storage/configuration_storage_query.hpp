#pragma once

#include <dispatcher/domain/configuration_status.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace dispatcher::storage
{
    struct ConfigurationStorageQuery
    {
        std::optional<std::uint64_t> config_version;
        std::optional<dispatcher::domain::ConfigurationStatus> status;
        std::optional<std::string> name;

        std::size_t limit{ 0 };
        bool latest_only{ false };

        [[nodiscard]] bool has_config_version() const noexcept;

        [[nodiscard]] bool has_status() const noexcept;

        [[nodiscard]] bool has_name() const noexcept;

        [[nodiscard]] bool has_limit() const noexcept;

        [[nodiscard]] bool requests_latest_only() const noexcept;
    };
}