#include <dispatcher/api/configuration_query_api_result.hpp>

#include <utility>

namespace dispatcher::api
{
    ConfigurationQueryApiResult ConfigurationQueryApiResult::success(
        std::vector<dispatcher::domain::ConfigurationSnapshot> snapshots,
        ApiPage page
    )
    {
        return ConfigurationQueryApiResult(
            ApiResult::success(),
            std::move(snapshots),
            page
        );
    }

    ConfigurationQueryApiResult ConfigurationQueryApiResult::failure(
        ApiStatus status,
        std::string operation,
        std::string resource,
        std::string field,
        std::string message
    )
    {
        return ConfigurationQueryApiResult(
            ApiResult::failure(
                status,
                std::move(operation),
                std::move(resource),
                std::move(field),
                std::move(message)
            ),
            {},
            ApiPage{}
        );
    }

    bool ConfigurationQueryApiResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool ConfigurationQueryApiResult::failed() const noexcept
    {
        return result_.failed();
    }

    ApiStatus ConfigurationQueryApiResult::status() const noexcept
    {
        return result_.status();
    }

    const ApiResult& ConfigurationQueryApiResult::result() const noexcept
    {
        return result_;
    }

    const ApiError& ConfigurationQueryApiResult::error() const noexcept
    {
        return result_.error();
    }

    const std::vector<dispatcher::domain::ConfigurationSnapshot>&
        ConfigurationQueryApiResult::snapshots() const noexcept
    {
        return snapshots_;
    }

    std::size_t ConfigurationQueryApiResult::snapshot_count() const noexcept
    {
        return snapshots_.size();
    }

    bool ConfigurationQueryApiResult::empty() const noexcept
    {
        return snapshots_.empty();
    }

    const ApiPage& ConfigurationQueryApiResult::page() const noexcept
    {
        return page_;
    }

    ConfigurationQueryApiResult::ConfigurationQueryApiResult(
        ApiResult result,
        std::vector<dispatcher::domain::ConfigurationSnapshot> snapshots,
        ApiPage page
    )
        : result_(std::move(result))
        , snapshots_(std::move(snapshots))
        , page_(page)
    {
    }
}