#pragma once

#include <dispatcher/config/configuration_document.hpp>
#include <dispatcher/config/configuration_snapshot_import_result.hpp>

namespace dispatcher::config
{
    class ConfigurationImporter
    {
    public:
        [[nodiscard]] static ConfigurationSnapshotImportResult import_document(
            const ConfigurationDocument& document
        );
    };
}