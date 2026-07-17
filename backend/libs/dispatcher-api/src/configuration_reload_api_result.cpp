#include <dispatcher/api/configuration_reload_api_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::api
{
    ConfigurationReloadApiResult ConfigurationReloadApiResult::success(
        dispatcher::domain::ConfigurationSnapshot snapshot
    )
    {
        return ConfigurationReloadApiResult(
            ApiResult::success(),
            std::move(snapshot)
        );
    }

    ConfigurationReloadApiResult ConfigurationReloadApiResult::failure(
        ApiStatus status,
        std::string operation,
        std::string resource,
        std::string field,
        std::string message
    )
    {
        return ConfigurationReloadApiResult(
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

    bool ConfigurationReloadApiResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool ConfigurationReloadApiResult::failed() const noexcept
    {
        return result_.failed();
    }

    ApiStatus ConfigurationReloadApiResult::status() const noexcept
    {
        return result_.status();
    }

    const ApiResult& ConfigurationReloadApiResult::result() const noexcept
    {
        return result_;
    }

    const ApiError& ConfigurationReloadApiResult::error() const noexcept
    {
        return result_.error();
    }

    bool ConfigurationReloadApiResult::has_snapshot() const noexcept
    {
        return snapshot_.has_value();
    }

    const dispatcher::domain::ConfigurationSnapshot&
        ConfigurationReloadApiResult::snapshot() const
    {
        if (!snapshot_.has_value())
        {
            throw std::logic_error(
                "ConfigurationReloadApiResult does not contain a snapshot"
            );
        }

        return snapshot_.value();
    }

    ConfigurationReloadApiResult::ConfigurationReloadApiResult(
        ApiResult result,
        std::optional<dispatcher::domain::ConfigurationSnapshot> snapshot
    )
        : result_(std::move(result))
        , snapshot_(std::move(snapshot))
    {
    }
}