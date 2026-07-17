#include <dispatcher/config/configuration_snapshot_export_mapper.hpp>

#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/device_definition.hpp>
#include <dispatcher/domain/history_policy.hpp>
#include <dispatcher/domain/tag_definition.hpp>

#include <string>
#include <utility>

namespace
{
    const char* configuration_status_to_export_string(
        dispatcher::domain::ConfigurationStatus status
    ) noexcept
    {
        switch (status)
        {
        case dispatcher::domain::ConfigurationStatus::Draft:
            return "draft";

        case dispatcher::domain::ConfigurationStatus::Published:
            return "published";
        }

        return "unknown";
    }

    const char* data_type_to_export_string(
        dispatcher::domain::DataType data_type
    ) noexcept
    {
        switch (data_type)
        {
        case dispatcher::domain::DataType::Boolean:
            return "boolean";

        case dispatcher::domain::DataType::Int32:
            return "int32";

        case dispatcher::domain::DataType::Int64:
            return "int64";

        case dispatcher::domain::DataType::Float32:
            return "float32";

        case dispatcher::domain::DataType::Float64:
            return "float64";

        case dispatcher::domain::DataType::String:
            return "string";
        }

        return "unknown";
    }

    const char* history_policy_to_export_string(
        dispatcher::domain::HistoryPolicy history_policy
    ) noexcept
    {
        switch (history_policy)
        {
        case dispatcher::domain::HistoryPolicy::Disabled:
            return "disabled";

        case dispatcher::domain::HistoryPolicy::OnChange:
            return "on_change";

        case dispatcher::domain::HistoryPolicy::OnChangeWithForcedSample:
            return "on_change_with_forced_sample";

        case dispatcher::domain::HistoryPolicy::EveryPoll:
            return "every_poll";

        case dispatcher::domain::HistoryPolicy::CriticalLossless:
            return "critical_lossless";

        case dispatcher::domain::HistoryPolicy::DiagnosticBestEffort:
            return "diagnostic_best_effort";

        case dispatcher::domain::HistoryPolicy::LiveOnly:
            return "live_only";
        }

        return "unknown";
    }

    dispatcher::config::ConfigurationExportDevice map_device(
        const dispatcher::domain::DeviceDefinition& device
    )
    {
        return dispatcher::config::ConfigurationExportDevice{
            .organization_id = device.organization_id().value(),
            .site_id = device.site_id().value(),
            .area_id = device.area_id().value(),
            .device_id = device.device_id().value(),
            .local_name = device.local_name(),
            .display_name = device.display_name(),
            .enabled = device.enabled()
        };
    }

    dispatcher::config::ConfigurationExportTag map_tag(
        const dispatcher::domain::TagDefinition& tag
    )
    {
        return dispatcher::config::ConfigurationExportTag{
            .organization_id = tag.organization_id().value(),
            .site_id = tag.site_id().value(),
            .area_id = tag.area_id().value(),
            .device_id = tag.device_id().value(),
            .tag_id = tag.tag_id().value(),
            .local_name = tag.local_name(),
            .display_name = tag.display_name(),
            .data_type = data_type_to_export_string(tag.data_type()),
            .history_policy = history_policy_to_export_string(
                tag.history_policy()
            ),
            .enabled = tag.enabled()
        };
    }
}

namespace dispatcher::config
{
    ConfigurationExportModelResult ConfigurationSnapshotExportMapper::map(
        const dispatcher::domain::ConfigurationSnapshot& snapshot,
        ConfigurationExportOptions options
    )
    {
        if (!options.has_known_format())
        {
            return ConfigurationExportModelResult::failure(
                ConfigurationIoStatus::UnsupportedFormat,
                "configuration.export",
                std::to_string(snapshot.config_version()),
                "format",
                "unsupported configuration export format"
            );
        }

        ConfigurationExportMetadata metadata;

        metadata.format = options.format;
        metadata.config_version = snapshot.config_version();
        metadata.status = configuration_status_to_export_string(
            snapshot.status()
        );
        metadata.source = std::move(options.source);
        metadata.exported_at = std::move(options.exported_at);

        ConfigurationExportModel model(std::move(metadata));

        if (options.requests_devices())
        {
            for (const auto& device : snapshot.devices())
            {
                model.add_device(map_device(device));
            }
        }

        if (options.requests_tags())
        {
            for (const auto& tag : snapshot.tags())
            {
                model.add_tag(map_tag(tag));
            }
        }

        return ConfigurationExportModelResult::success(
            std::move(model)
        );
    }
}