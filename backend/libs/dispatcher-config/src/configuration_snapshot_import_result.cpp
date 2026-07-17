#include <dispatcher/config/configuration_snapshot_import_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::config
{
    ConfigurationSnapshotImportResult
        ConfigurationSnapshotImportResult::success(
            dispatcher::domain::ConfigurationSnapshot snapshot
        )
    {
        return ConfigurationSnapshotImportResult(
            ConfigurationIoResult::success(),
            std::move(snapshot)
        );
    }

    ConfigurationSnapshotImportResult
        ConfigurationSnapshotImportResult::failure(
            ConfigurationIoStatus status,
            std::string operation,
            std::string resource,
            std::string field,
            std::string message
        )
    {
        return ConfigurationSnapshotImportResult(
            ConfigurationIoResult::failure(
                status,
                std::move(operation),
                std::move(resource),
                std::move(field),
                std::move(message)
            ),
            std::nullopt
        );
    }

    bool ConfigurationSnapshotImportResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool ConfigurationSnapshotImportResult::failed() const noexcept
    {
        return result_.failed();
    }

    ConfigurationIoStatus ConfigurationSnapshotImportResult::status()
        const noexcept
    {
        return result_.status();
    }

    const ConfigurationIoResult& ConfigurationSnapshotImportResult::result()
        const noexcept
    {
        return result_;
    }

    const ConfigurationIoError& ConfigurationSnapshotImportResult::error()
        const noexcept
    {
        return result_.error();
    }

    bool ConfigurationSnapshotImportResult::has_snapshot() const noexcept
    {
        return snapshot_.has_value();
    }

    const dispatcher::domain::ConfigurationSnapshot&
        ConfigurationSnapshotImportResult::snapshot() const
    {
        if (!snapshot_.has_value())
        {
            throw std::logic_error(
                "ConfigurationSnapshotImportResult does not contain a snapshot"
            );
        }

        return snapshot_.value();
    }

    ConfigurationSnapshotImportResult::ConfigurationSnapshotImportResult(
        ConfigurationIoResult result,
        std::optional<dispatcher::domain::ConfigurationSnapshot> snapshot
    )
        : result_(std::move(result))
        , snapshot_(std::move(snapshot))
    {
    }
}