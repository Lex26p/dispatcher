#pragma once

#include <dispatcher/storage/configuration_storage.hpp>
#include <dispatcher/storage/storage_result.hpp>
#include <dispatcher/storage/storage_status.hpp>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace dispatcher::storage
{
    class InMemoryConfigurationStorage final : public ConfigurationStorage
    {
    public:
        [[nodiscard]] StorageResult save(
            const dispatcher::domain::ConfigurationSnapshot& snapshot
        ) override
        {
            for (auto& stored_snapshot : snapshots_)
            {
                if (
                    stored_snapshot.config_version()
                    == snapshot.config_version()
                    )
                {
                    stored_snapshot = snapshot;

                    return StorageResult::success();
                }
            }

            snapshots_.push_back(snapshot);

            return StorageResult::success();
        }

        [[nodiscard]] ConfigurationSnapshotQueryResult query(
            const ConfigurationStorageQuery& query
        ) const override
        {
            if (query.has_name())
            {
                return ConfigurationSnapshotQueryResult::failure(
                    StorageStatus::UnsupportedOperation,
                    "configuration.query",
                    query.name.value(),
                    "name filtering is not implemented by baseline in-memory configuration storage"
                );
            }

            std::vector<dispatcher::domain::ConfigurationSnapshot>
                selected_snapshots;

            for (const auto& snapshot : snapshots_)
            {
                if (
                    query.has_config_version()
                    && snapshot.config_version()
                    != query.config_version.value()
                    )
                {
                    continue;
                }

                if (
                    query.has_status()
                    && snapshot.status() != query.status.value()
                    )
                {
                    continue;
                }

                selected_snapshots.push_back(snapshot);
            }

            if (query.requests_latest_only())
            {
                if (selected_snapshots.empty())
                {
                    return ConfigurationSnapshotQueryResult::success({});
                }

                return ConfigurationSnapshotQueryResult::success(
                    std::vector<dispatcher::domain::ConfigurationSnapshot>{
                    selected_snapshots.back()
                }
                );
            }

            if (
                query.has_limit()
                && selected_snapshots.size() > query.limit
                )
            {
                selected_snapshots.erase(
                    selected_snapshots.begin(),
                    selected_snapshots.end()
                    - static_cast<std::vector<
                    dispatcher::domain::ConfigurationSnapshot
                    >::difference_type>(query.limit)
                );
            }

            return ConfigurationSnapshotQueryResult::success(
                std::move(selected_snapshots)
            );
        }

        [[nodiscard]] StorageResult remove_by_version(
            std::uint64_t config_version
        ) override
        {
            const auto old_size = snapshots_.size();

            snapshots_.erase(
                std::remove_if(
                    snapshots_.begin(),
                    snapshots_.end(),
                    [config_version](
                        const dispatcher::domain::ConfigurationSnapshot&
                        snapshot
                        )
                    {
                        return snapshot.config_version() == config_version;
                    }
                ),
                snapshots_.end()
            );

            if (snapshots_.size() == old_size)
            {
                return StorageResult::failure(
                    StorageStatus::NotFound,
                    "configuration.remove_by_version",
                    std::to_string(config_version),
                    "configuration version was not found"
                );
            }

            return StorageResult::success();
        }

        [[nodiscard]] StorageResult clear() override
        {
            snapshots_.clear();

            return StorageResult::success();
        }

        [[nodiscard]] const std::vector<
            dispatcher::domain::ConfigurationSnapshot
        >& snapshots() const noexcept
        {
            return snapshots_;
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return snapshots_.size();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return snapshots_.empty();
        }

    private:
        std::vector<dispatcher::domain::ConfigurationSnapshot> snapshots_;
    };
}