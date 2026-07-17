#pragma once

#include <dispatcher/domain/configuration_snapshot.hpp>
#include <dispatcher/storage/storage_result.hpp>
#include <dispatcher/storage/storage_status.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace dispatcher::storage
{
    class ConfigurationSnapshotQueryResult
    {
    public:
        [[nodiscard]] static ConfigurationSnapshotQueryResult success(
            std::vector<dispatcher::domain::ConfigurationSnapshot> snapshots
        );

        [[nodiscard]] static ConfigurationSnapshotQueryResult failure(
            StorageStatus status,
            std::string operation = {},
            std::string key = {},
            std::string message = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] StorageStatus status() const noexcept;

        [[nodiscard]] const StorageResult& result() const noexcept;

        [[nodiscard]] const StorageError& error() const noexcept;

        [[nodiscard]] const std::vector<dispatcher::domain::ConfigurationSnapshot>&
            snapshots() const noexcept;

        [[nodiscard]] std::size_t snapshot_count() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

    private:
        ConfigurationSnapshotQueryResult(
            StorageResult result,
            std::vector<dispatcher::domain::ConfigurationSnapshot> snapshots
        );

        StorageResult result_;
        std::vector<dispatcher::domain::ConfigurationSnapshot> snapshots_;
    };
}