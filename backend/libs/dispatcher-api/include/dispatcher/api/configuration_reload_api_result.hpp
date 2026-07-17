#pragma once

#include <dispatcher/api/api_result.hpp>
#include <dispatcher/api/api_status.hpp>
#include <dispatcher/domain/configuration_snapshot.hpp>

#include <optional>
#include <string>

namespace dispatcher::api
{
    class ConfigurationReloadApiResult
    {
    public:
        [[nodiscard]] static ConfigurationReloadApiResult success(
            dispatcher::domain::ConfigurationSnapshot snapshot
        );

        [[nodiscard]] static ConfigurationReloadApiResult failure(
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

        [[nodiscard]] bool has_snapshot() const noexcept;

        [[nodiscard]] const dispatcher::domain::ConfigurationSnapshot&
            snapshot() const;

    private:
        ConfigurationReloadApiResult(
            ApiResult result,
            std::optional<dispatcher::domain::ConfigurationSnapshot> snapshot
        );

        ApiResult result_;
        std::optional<dispatcher::domain::ConfigurationSnapshot> snapshot_;
    };
}