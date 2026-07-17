#pragma once

#include <dispatcher/api/api_page_request.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/storage/configuration_storage_query.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace dispatcher::api
{
    struct ConfigurationQueryRequest
    {
        std::optional<std::uint64_t> config_version;
        std::optional<dispatcher::domain::ConfigurationStatus> status;
        std::optional<std::string> name;

        ApiPageRequest page;
        bool latest_only{ false };

        [[nodiscard]] bool has_config_version() const noexcept;

        [[nodiscard]] bool has_status() const noexcept;

        [[nodiscard]] bool has_name() const noexcept;

        [[nodiscard]] bool requests_latest_only() const noexcept;

        [[nodiscard]] dispatcher::storage::ConfigurationStorageQuery
            to_storage_query() const;
    };
}