#pragma once

#include <dispatcher/api/api_result.hpp>
#include <dispatcher/api/api_status.hpp>
#include <dispatcher/config/configuration_document.hpp>

#include <optional>
#include <string>

namespace dispatcher::api
{
    class ConfigurationExportApiResult
    {
    public:
        [[nodiscard]] static ConfigurationExportApiResult success(
            dispatcher::config::ConfigurationDocument document
        );

        [[nodiscard]] static ConfigurationExportApiResult failure(
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

        [[nodiscard]] bool has_document() const noexcept;

        [[nodiscard]] const dispatcher::config::ConfigurationDocument&
            document() const;

    private:
        ConfigurationExportApiResult(
            ApiResult result,
            std::optional<dispatcher::config::ConfigurationDocument> document
        );

        ApiResult result_;
        std::optional<dispatcher::config::ConfigurationDocument> document_;
    };
}