#include <dispatcher/api/configuration_export_api_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::api
{
    ConfigurationExportApiResult ConfigurationExportApiResult::success(
        dispatcher::config::ConfigurationDocument document
    )
    {
        return ConfigurationExportApiResult(
            ApiResult::success(),
            std::move(document)
        );
    }

    ConfigurationExportApiResult ConfigurationExportApiResult::failure(
        ApiStatus status,
        std::string operation,
        std::string resource,
        std::string field,
        std::string message
    )
    {
        return ConfigurationExportApiResult(
            ApiResult::failure(
                status,
                std::move(operation),
                std::move(resource),
                std::move(field),
                std::move(message)
            ),
            std::nullopt
        );
    }

    bool ConfigurationExportApiResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool ConfigurationExportApiResult::failed() const noexcept
    {
        return result_.failed();
    }

    ApiStatus ConfigurationExportApiResult::status() const noexcept
    {
        return result_.status();
    }

    const ApiResult& ConfigurationExportApiResult::result() const noexcept
    {
        return result_;
    }

    const ApiError& ConfigurationExportApiResult::error() const noexcept
    {
        return result_.error();
    }

    bool ConfigurationExportApiResult::has_document() const noexcept
    {
        return document_.has_value();
    }

    const dispatcher::config::ConfigurationDocument&
        ConfigurationExportApiResult::document() const
    {
        if (!document_.has_value())
        {
            throw std::logic_error(
                "ConfigurationExportApiResult does not contain a document"
            );
        }

        return document_.value();
    }

    ConfigurationExportApiResult::ConfigurationExportApiResult(
        ApiResult result,
        std::optional<dispatcher::config::ConfigurationDocument> document
    )
        : result_(std::move(result))
        , document_(std::move(document))
    {
    }
}