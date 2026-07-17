#pragma once

#include <dispatcher/domain/configuration_snapshot.hpp>
#include <dispatcher/storage/configuration_snapshot_query_result.hpp>
#include <dispatcher/storage/configuration_storage_query.hpp>
#include <dispatcher/storage/storage_result.hpp>

#include <cstdint>

namespace dispatcher::storage
{
    class ConfigurationStorage
    {
    public:
        virtual ~ConfigurationStorage() = default;

        [[nodiscard]] virtual StorageResult save(
            const dispatcher::domain::ConfigurationSnapshot& snapshot
        ) = 0;

        [[nodiscard]] virtual ConfigurationSnapshotQueryResult query(
            const ConfigurationStorageQuery& query
        ) const = 0;

        [[nodiscard]] virtual StorageResult remove_by_version(
            std::uint64_t config_version
        ) = 0;

        [[nodiscard]] virtual StorageResult clear() = 0;
    };
}