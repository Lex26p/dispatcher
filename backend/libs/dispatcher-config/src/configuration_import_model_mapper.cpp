#include <dispatcher/config/configuration_import_model_mapper.hpp>

#include <dispatcher/config/configuration_format.hpp>
#include <dispatcher/domain/configuration_snapshot_builder.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/device_definition_builder.hpp>
#include <dispatcher/domain/history_policy.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/tag_definition_builder.hpp>

#include <optional>
#include <string>

namespace
{
    std::string import_resource(
        const dispatcher::config::ConfigurationImportModel& model
    )
    {
        if (model.metadata().has_source())
        {
            return model.metadata().source;
        }

        if (model.metadata().has_config_version())
        {
            return std::to_string(model.metadata().config_version);
        }

        return {};
    }

    std::optional<dispatcher::domain::ConfigurationStatus>
        parse_configuration_status(const std::string& value)
    {
        if (value == "draft")
        {
            return dispatcher::domain::ConfigurationStatus::Draft;
        }

        if (value == "published")
        {
            return dispatcher::domain::ConfigurationStatus::Published;
        }

        return std::nullopt;
    }

    std::optional<dispatcher::domain::DataType> parse_data_type(
        const std::string& value
    )
    {
        if (value == "boolean" || value == "bool")
        {
            return dispatcher::domain::DataType::Boolean;
        }

        if (value == "int32")
        {
            return dispatcher::domain::DataType::Int32;
        }

        if (value == "int64")
        {
            return dispatcher::domain::DataType::Int64;
        }

        if (value == "float32")
        {
            return dispatcher::domain::DataType::Float32;
        }

        if (value == "float64")
        {
            return dispatcher::domain::DataType::Float64;
        }

        if (value == "string")
        {
            return dispatcher::domain::DataType::String;
        }

        return std::nullopt;
    }

    std::optional<dispatcher::domain::HistoryPolicy> parse_history_policy(
        const std::string& value
    )
    {
        if (value == "disabled")
        {
            return dispatcher::domain::HistoryPolicy::Disabled;
        }

        if (value == "on_change")
        {
            return dispatcher::domain::HistoryPolicy::OnChange;
        }

        if (value == "on_change_with_forced_sample")
        {
            return dispatcher::domain::HistoryPolicy::OnChangeWithForcedSample;
        }

        if (value == "every_poll")
        {
            return dispatcher::domain::HistoryPolicy::EveryPoll;
        }

        if (value == "critical_lossless")
        {
            return dispatcher::domain::HistoryPolicy::CriticalLossless;
        }

        if (value == "diagnostic_best_effort")
        {
            return dispatcher::domain::HistoryPolicy::DiagnosticBestEffort;
        }

        if (value == "live_only")
        {
            return dispatcher::domain::HistoryPolicy::LiveOnly;
        }

        return std::nullopt;
    }

    dispatcher::domain::DeviceDefinition make_device_definition(
        const dispatcher::config::ConfigurationImportDevice& device
    )
    {
        return dispatcher::domain::DeviceDefinitionBuilder{}
            .device_id(dispatcher::domain::DeviceId{ device.device_id })
            .organization_id(
                dispatcher::domain::OrganizationId{ device.organization_id }
            )
            .site_id(dispatcher::domain::SiteId{ device.site_id })
            .area_id(dispatcher::domain::AreaId{ device.area_id })
            .local_name(device.local_name)
            .display_name(device.display_name)
            .enabled(device.enabled)
            .build();
    }

    dispatcher::domain::TagDefinition make_tag_definition(
        const dispatcher::config::ConfigurationImportTag& tag,
        dispatcher::domain::DataType data_type,
        dispatcher::domain::HistoryPolicy history_policy
    )
    {
        return dispatcher::domain::TagDefinitionBuilder{}
            .tag_id(dispatcher::domain::TagId{ tag.tag_id })
            .organization_id(
                dispatcher::domain::OrganizationId{ tag.organization_id }
            )
            .site_id(dispatcher::domain::SiteId{ tag.site_id })
            .area_id(dispatcher::domain::AreaId{ tag.area_id })
            .device_id(dispatcher::domain::DeviceId{ tag.device_id })
            .local_name(tag.local_name)
            .display_name(tag.display_name)
            .data_type(data_type)
            .history_policy(history_policy)
            .enabled(tag.enabled)
            .build();
    }
}

namespace dispatcher::config
{
    ConfigurationSnapshotImportResult ConfigurationImportModelMapper::map(
        const ConfigurationImportModel& model
    )
    {
        const auto resource = import_resource(model);

        if (!model.metadata().has_known_format())
        {
            return ConfigurationSnapshotImportResult::failure(
                ConfigurationIoStatus::UnsupportedFormat,
                "configuration.import.map",
                resource,
                "metadata.format",
                "unsupported configuration import format"
            );
        }

        if (model.metadata().format != ConfigurationFormat::Json)
        {
            return ConfigurationSnapshotImportResult::failure(
                ConfigurationIoStatus::UnsupportedFormat,
                "configuration.import.map",
                resource,
                "metadata.format",
                "unsupported configuration import format"
            );
        }

        if (!model.metadata().has_config_version())
        {
            return ConfigurationSnapshotImportResult::failure(
                ConfigurationIoStatus::ValidationError,
                "configuration.import.map",
                resource,
                "metadata.config_version",
                "configuration version is required"
            );
        }

        const auto status = parse_configuration_status(
            model.metadata().status
        );

        if (!status.has_value())
        {
            return ConfigurationSnapshotImportResult::failure(
                ConfigurationIoStatus::ValidationError,
                "configuration.import.map",
                resource,
                "metadata.status",
                "configuration status is invalid"
            );
        }

        dispatcher::domain::ConfigurationSnapshotBuilder builder;

        builder.config_version(model.metadata().config_version);
        builder.status(status.value());
        builder.description(model.metadata().description);

        for (std::size_t index = 0; index < model.devices().size(); ++index)
        {
            const auto& device = model.devices()[index];

            if (!device.has_required_identity())
            {
                return ConfigurationSnapshotImportResult::failure(
                    ConfigurationIoStatus::ValidationError,
                    "configuration.import.map",
                    resource,
                    "devices[" + std::to_string(index) + "]",
                    "device identity is incomplete"
                );
            }

            const auto validation = builder.add_device(
                make_device_definition(device)
            );

            if (validation.has_errors())
            {
                const auto& error = validation.errors().front();

                return ConfigurationSnapshotImportResult::failure(
                    ConfigurationIoStatus::ValidationError,
                    "configuration.import.map",
                    resource,
                    "devices[" + std::to_string(index) + "]." + error.field,
                    error.message
                );
            }
        }

        for (std::size_t index = 0; index < model.tags().size(); ++index)
        {
            const auto& tag = model.tags()[index];

            if (!tag.has_required_identity())
            {
                return ConfigurationSnapshotImportResult::failure(
                    ConfigurationIoStatus::ValidationError,
                    "configuration.import.map",
                    resource,
                    "tags[" + std::to_string(index) + "]",
                    "tag identity is incomplete"
                );
            }

            const auto data_type = parse_data_type(tag.data_type);

            if (!data_type.has_value())
            {
                return ConfigurationSnapshotImportResult::failure(
                    ConfigurationIoStatus::ValidationError,
                    "configuration.import.map",
                    resource,
                    "tags[" + std::to_string(index) + "].data_type",
                    "tag data type is invalid"
                );
            }

            const auto history_policy = parse_history_policy(
                tag.history_policy
            );

            if (!history_policy.has_value())
            {
                return ConfigurationSnapshotImportResult::failure(
                    ConfigurationIoStatus::ValidationError,
                    "configuration.import.map",
                    resource,
                    "tags[" + std::to_string(index) + "].history_policy",
                    "tag history policy is invalid"
                );
            }

            const auto validation = builder.add_tag(
                make_tag_definition(
                    tag,
                    data_type.value(),
                    history_policy.value()
                )
            );

            if (validation.has_errors())
            {
                const auto& error = validation.errors().front();

                return ConfigurationSnapshotImportResult::failure(
                    ConfigurationIoStatus::ValidationError,
                    "configuration.import.map",
                    resource,
                    "tags[" + std::to_string(index) + "]." + error.field,
                    error.message
                );
            }
        }

        return ConfigurationSnapshotImportResult::success(
            builder.build()
        );
    }
}