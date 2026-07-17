#pragma once

#include <dispatcher/config/configuration_export_model_result.hpp>
#include <dispatcher/config/configuration_export_options.hpp>
#include <dispatcher/domain/configuration_snapshot.hpp>

namespace dispatcher::config
{
    class ConfigurationSnapshotExportMapper
    {
    public:
        [[nodiscard]] static ConfigurationExportModelResult map(
            const dispatcher::domain::ConfigurationSnapshot& snapshot,
            ConfigurationExportOptions options = {}
        );
    };
}