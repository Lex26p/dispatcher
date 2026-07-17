#pragma once

#include <dispatcher/config/configuration_document_result.hpp>
#include <dispatcher/config/configuration_export_model.hpp>

#include <string>

namespace dispatcher::config
{
    class ConfigurationJsonExportSerializer
    {
    public:
        [[nodiscard]] static ConfigurationDocumentResult serialize(
            const ConfigurationExportModel& model,
            std::string name = {}
        );
    };
}