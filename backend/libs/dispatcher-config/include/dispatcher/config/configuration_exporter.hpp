#pragma once

#include <dispatcher/config/configuration_document_result.hpp>
#include <dispatcher/config/configuration_export_options.hpp>
#include <dispatcher/domain/configuration_snapshot.hpp>

#include <string>

namespace dispatcher::config
{
    class ConfigurationExporter
    {
    public:
        [[nodiscard]] static ConfigurationDocumentResult export_snapshot(
            const dispatcher::domain::ConfigurationSnapshot& snapshot,
            ConfigurationExportOptions options = {},
            std::string name = {}
        );
    };
}