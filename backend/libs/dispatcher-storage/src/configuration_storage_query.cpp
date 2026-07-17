#include <dispatcher/storage/configuration_storage_query.hpp>

namespace dispatcher::storage
{
    bool ConfigurationStorageQuery::has_config_version() const noexcept
    {
        return config_version.has_value();
    }

    bool ConfigurationStorageQuery::has_status() const noexcept
    {
        return status.has_value();
    }

    bool ConfigurationStorageQuery::has_name() const noexcept
    {
        return name.has_value() && !name->empty();
    }

    bool ConfigurationStorageQuery::has_limit() const noexcept
    {
        return limit > 0;
    }

    bool ConfigurationStorageQuery::requests_latest_only() const noexcept
    {
        return latest_only;
    }
}