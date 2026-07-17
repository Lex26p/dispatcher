#pragma once

#include <dispatcher/config/configuration_export_model.hpp>
#include <dispatcher/config/configuration_io_result.hpp>
#include <dispatcher/config/configuration_io_status.hpp>

#include <optional>
#include <string>

namespace dispatcher::config
{
    class ConfigurationExportModelResult
    {
    public:
        [[nodiscard]] static ConfigurationExportModelResult success(
            ConfigurationExportModel model
        );

        [[nodiscard]] static ConfigurationExportModelResult failure(
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

        [[nodiscard]] bool has_model() const noexcept;

        [[nodiscard]] const ConfigurationExportModel& model() const;

    private:
        ConfigurationExportModelResult(
            ConfigurationIoResult result,
            std::optional<ConfigurationExportModel> model
        );

        ConfigurationIoResult result_;
        std::optional<ConfigurationExportModel> model_;
    };
}