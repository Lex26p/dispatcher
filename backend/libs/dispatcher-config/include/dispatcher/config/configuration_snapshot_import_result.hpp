#pragma once

#include <dispatcher/config/configuration_io_result.hpp>
#include <dispatcher/config/configuration_io_status.hpp>
#include <dispatcher/domain/configuration_snapshot.hpp>

#include <optional>
#include <string>

namespace dispatcher::config
{
    class ConfigurationSnapshotImportResult
    {
    public:
        [[nodiscard]] static ConfigurationSnapshotImportResult success(
            dispatcher::domain::ConfigurationSnapshot snapshot
        );

        [[nodiscard]] static ConfigurationSnapshotImportResult failure(
            ConfigurationIoStatus status,
            std::string operation = {},
            std::string resource = {},
            std::string field = {},
            std::string message = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] ConfigurationIoStatus status() const noexcept;

        [[nodiscard]] const ConfigurationIoResult& result() const noexcept;

        [[nodiscard]] const ConfigurationIoError& error() const noexcept;

        [[nodiscard]] bool has_snapshot() const noexcept;

        [[nodiscard]] const dispatcher::domain::ConfigurationSnapshot&
            snapshot() const;

    private:
        ConfigurationSnapshotImportResult(
            ConfigurationIoResult result,
            std::optional<dispatcher::domain::ConfigurationSnapshot> snapshot
        );

        ConfigurationIoResult result_;
        std::optional<dispatcher::domain::ConfigurationSnapshot> snapshot_;
    };
}