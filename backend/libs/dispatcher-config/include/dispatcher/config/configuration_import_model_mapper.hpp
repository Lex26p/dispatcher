#pragma once

#include <dispatcher/config/configuration_import_model.hpp>
#include <dispatcher/config/configuration_snapshot_import_result.hpp>

namespace dispatcher::config
{
    class ConfigurationImportModelMapper
    {
    public:
        [[nodiscard]] static ConfigurationSnapshotImportResult map(
            const ConfigurationImportModel& model
        );
    };
}