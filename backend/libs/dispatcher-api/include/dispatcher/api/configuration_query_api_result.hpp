#pragma once

#include <dispatcher/api/api_page.hpp>
#include <dispatcher/api/api_result.hpp>
#include <dispatcher/api/api_status.hpp>
#include <dispatcher/domain/configuration_snapshot.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace dispatcher::api
{
    class ConfigurationQueryApiResult
    {
    public:
        [[nodiscard]] static ConfigurationQueryApiResult success(
            std::vector<dispatcher::domain::ConfigurationSnapshot> snapshots,
            ApiPage page
        );

        [[nodiscard]] static ConfigurationQueryApiResult failure(
            ApiStatus status,
            std::string operation = {},
            std::string resource = {},
            std::string field = {},
            std::string message = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] ApiStatus status() const noexcept;

        [[nodiscard]] const ApiResult& result() const noexcept;

        [[nodiscard]] const ApiError& error() const noexcept;

        [[nodiscard]] const std::vector<dispatcher::domain::ConfigurationSnapshot>&
            snapshots() const noexcept;

        [[nodiscard]] std::size_t snapshot_count() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] const ApiPage& page() const noexcept;

    private:
        ConfigurationQueryApiResult(
            ApiResult result,
            std::vector<dispatcher::domain::ConfigurationSnapshot> snapshots,
            ApiPage page
        );

        ApiResult result_;
        std::vector<dispatcher::domain::ConfigurationSnapshot> snapshots_;
        ApiPage page_;
    };
}