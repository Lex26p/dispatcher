#include <dispatcher/api/configuration_query_request.hpp>

namespace dispatcher::api
{
    bool ConfigurationQueryRequest::has_config_version() const noexcept
    {
        return config_version.has_value();
    }

    bool ConfigurationQueryRequest::has_status() const noexcept
    {
        return status.has_value();
    }

    bool ConfigurationQueryRequest::has_name() const noexcept
    {
        return name.has_value() && !name->empty();
    }

    bool ConfigurationQueryRequest::requests_latest_only() const noexcept
    {
        return latest_only;
    }

    dispatcher::storage::ConfigurationStorageQuery
        ConfigurationQueryRequest::to_storage_query() const
    {
        return dispatcher::storage::ConfigurationStorageQuery{
            .config_version = config_version,
            .status = status,
            .name = name,
            .limit = page.limit,
            .latest_only = latest_only
        };
    }
}