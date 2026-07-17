#include <dispatcher/config/configuration_exporter.hpp>

#include <dispatcher/config/configuration_json_export_serializer.hpp>
#include <dispatcher/config/configuration_snapshot_export_mapper.hpp>

#include <utility>

namespace dispatcher::config
{
    ConfigurationDocumentResult ConfigurationExporter::export_snapshot(
        const dispatcher::domain::ConfigurationSnapshot& snapshot,
        ConfigurationExportOptions options,
        std::string name
    )
    {
        const auto model_result =
            ConfigurationSnapshotExportMapper::map(snapshot, options);

        if (model_result.failed())
        {
            return ConfigurationDocumentResult::failure(
                model_result.status(),
                model_result.error().operation,
                model_result.error().resource,
                model_result.error().field,
                model_result.error().message
            );
        }

        switch (options.format)
        {
        case ConfigurationFormat::Json:
            return ConfigurationJsonExportSerializer::serialize(
                model_result.model(),
                std::move(name)
            );

        case ConfigurationFormat::Unknown:
            return ConfigurationDocumentResult::failure(
                ConfigurationIoStatus::UnsupportedFormat,
                "configuration.export",
                std::to_string(snapshot.config_version()),
                "format",
                "unsupported configuration export format"
            );
        }

        return ConfigurationDocumentResult::failure(
            ConfigurationIoStatus::UnsupportedFormat,
            "configuration.export",
            std::to_string(snapshot.config_version()),
            "format",
            "unsupported configuration export format"
        );
    }
}