#include <dispatcher/storage/configuration_snapshot_query_result.hpp>

#include <utility>

namespace dispatcher::storage
{
    ConfigurationSnapshotQueryResult ConfigurationSnapshotQueryResult::success(
        std::vector<dispatcher::domain::ConfigurationSnapshot> snapshots
    )
    {
        return ConfigurationSnapshotQueryResult(
            StorageResult::success(),
            std::move(snapshots)
        );
    }

    ConfigurationSnapshotQueryResult ConfigurationSnapshotQueryResult::failure(
        StorageStatus status,
        std::string operation,
        std::string key,
        std::string message
    )
    {
        return ConfigurationSnapshotQueryResult(
            StorageResult::failure(
                status,
                std::move(operation),
                std::move(key),
                std::move(message)
            ),
            {}
        );
    }

    bool ConfigurationSnapshotQueryResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool ConfigurationSnapshotQueryResult::failed() const noexcept
    {
        return result_.failed();
    }

    StorageStatus ConfigurationSnapshotQueryResult::status() const noexcept
    {
        return result_.status();
    }

    const StorageResult& ConfigurationSnapshotQueryResult::result() const noexcept
    {
        return result_;
    }

    const StorageError& ConfigurationSnapshotQueryResult::error() const noexcept
    {
        return result_.error();
    }

    const std::vector<dispatcher::domain::ConfigurationSnapshot>&
        ConfigurationSnapshotQueryResult::snapshots() const noexcept
    {
        return snapshots_;
    }

    std::size_t ConfigurationSnapshotQueryResult::snapshot_count() const noexcept
    {
        return snapshots_.size();
    }

    bool ConfigurationSnapshotQueryResult::empty() const noexcept
    {
        return snapshots_.empty();
    }

    ConfigurationSnapshotQueryResult::ConfigurationSnapshotQueryResult(
        StorageResult result,
        std::vector<dispatcher::domain::ConfigurationSnapshot> snapshots
    )
        : result_(std::move(result))
        , snapshots_(std::move(snapshots))
    {
    }
}