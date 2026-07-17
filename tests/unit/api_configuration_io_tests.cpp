#include <dispatcher/api/api_status.hpp>
#include <dispatcher/api/api_status_mapping.hpp>
#include <dispatcher/api/dispatcher_configuration_api.hpp>
#include <dispatcher/config/configuration_document.hpp>
#include <dispatcher/config/configuration_export_options.hpp>
#include <dispatcher/config/configuration_format.hpp>
#include <dispatcher/config/configuration_io_status.hpp>
#include <dispatcher/domain/configuration_snapshot_builder.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/device_definition_builder.hpp>
#include <dispatcher/domain/history_policy.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/tag_definition_builder.hpp>
#include <dispatcher/runtime/dispatcher_runtime.hpp>

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

namespace
{
    dispatcher::domain::DeviceDefinition make_api_config_io_device()
    {
        return dispatcher::domain::DeviceDefinitionBuilder{}
            .device_id(dispatcher::domain::DeviceId{ "device-1" })
            .organization_id(dispatcher::domain::OrganizationId{ "org-1" })
            .site_id(dispatcher::domain::SiteId{ "site-1" })
            .area_id(dispatcher::domain::AreaId{ "area-1" })
            .local_name("device-1")
            .display_name("Device 1")
            .enabled(true)
            .build();
    }

    dispatcher::domain::TagDefinition make_api_config_io_tag()
    {
        return dispatcher::domain::TagDefinitionBuilder{}
            .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
            .organization_id(dispatcher::domain::OrganizationId{ "org-1" })
            .site_id(dispatcher::domain::SiteId{ "site-1" })
            .area_id(dispatcher::domain::AreaId{ "area-1" })
            .device_id(dispatcher::domain::DeviceId{ "device-1" })
            .local_name("temperature")
            .display_name("Temperature")
            .data_type(dispatcher::domain::DataType::Float64)
            .history_policy(dispatcher::domain::HistoryPolicy::EveryPoll)
            .enabled(true)
            .build();
    }

    dispatcher::domain::ConfigurationSnapshot make_api_config_io_snapshot()
    {
        dispatcher::domain::ConfigurationSnapshotBuilder builder;

        builder.config_version(7);
        builder.status(dispatcher::domain::ConfigurationStatus::Published);

        const auto device_result = builder.add_device(
            make_api_config_io_device()
        );

        if (device_result.has_errors())
        {
            throw std::runtime_error(
                "failed to add device: "
                + device_result.errors().front().field
                + " - "
                + device_result.errors().front().message
            );
        }

        const auto tag_result = builder.add_tag(
            make_api_config_io_tag()
        );

        if (tag_result.has_errors())
        {
            throw std::runtime_error(
                "failed to add tag: "
                + tag_result.errors().front().field
                + " - "
                + tag_result.errors().front().message
            );
        }

        return builder.build();
    }

    dispatcher::config::ConfigurationDocument make_api_import_document()
    {
        return dispatcher::config::ConfigurationDocument::json(
            R"json({
  "schema_version": "dispatcher.config.v1",
  "format": "json",
  "config_version": 9,
  "status": "published",
  "devices": [
    {
      "organization_id": "org-1",
      "site_id": "site-1",
      "area_id": "area-1",
      "device_id": "device-2",
      "local_name": "device-2",
      "display_name": "Device 2",
      "enabled": true
    }
  ],
  "tags": [
    {
      "organization_id": "org-1",
      "site_id": "site-1",
      "area_id": "area-1",
      "device_id": "device-2",
      "tag_id": "tag-pressure",
      "local_name": "pressure",
      "display_name": "Pressure",
      "data_type": "float64",
      "history_policy": "every_poll",
      "enabled": true
    }
  ]
}
)json",
"import.json"
);
    }
}

TEST(ApiStatusMappingTests, MapsConfigurationIoStatuses)
{
    EXPECT_EQ(
        dispatcher::api::map_configuration_io_status_to_api_status(
            dispatcher::config::ConfigurationIoStatus::Success
        ),
        dispatcher::api::ApiStatus::Success
    );

    EXPECT_EQ(
        dispatcher::api::map_configuration_io_status_to_api_status(
            dispatcher::config::ConfigurationIoStatus::UnsupportedFormat
        ),
        dispatcher::api::ApiStatus::UnsupportedOperation
    );

    EXPECT_EQ(
        dispatcher::api::map_configuration_io_status_to_api_status(
            dispatcher::config::ConfigurationIoStatus::ParseError
        ),
        dispatcher::api::ApiStatus::BadRequest
    );

    EXPECT_EQ(
        dispatcher::api::map_configuration_io_status_to_api_status(
            dispatcher::config::ConfigurationIoStatus::ValidationError
        ),
        dispatcher::api::ApiStatus::ValidationError
    );

    EXPECT_EQ(
        dispatcher::api::map_configuration_io_status_to_api_status(
            dispatcher::config::ConfigurationIoStatus::UnknownError
        ),
        dispatcher::api::ApiStatus::InternalError
    );
}

TEST(DispatcherConfigurationApiIoTests, ExportsCurrentConfiguration)
{
    dispatcher::runtime::DispatcherRuntime runtime(
        make_api_config_io_snapshot()
    );

    dispatcher::api::DispatcherConfigurationApi api(runtime);

    dispatcher::config::ConfigurationExportOptions options;

    options.format = dispatcher::config::ConfigurationFormat::Json;
    options.source = "api-test";
    options.exported_at = "2026-07-01T00:00:00Z";

    const auto result = api.export_current(
        options,
        "current.json"
    );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_document());

    const auto& document = result.document();

    EXPECT_EQ(
        document.format(),
        dispatcher::config::ConfigurationFormat::Json
    );

    EXPECT_EQ(document.name(), "current.json");

    const std::string& content = document.content();

    EXPECT_NE(content.find("\"config_version\": 7"), std::string::npos);
    EXPECT_NE(content.find("\"status\": \"published\""), std::string::npos);
    EXPECT_NE(content.find("\"source\": \"api-test\""), std::string::npos);
    EXPECT_NE(content.find("\"device_id\": \"device-1\""), std::string::npos);

    EXPECT_NE(
        content.find("\"tag_id\": \"tag-temperature\""),
        std::string::npos
    );
}

TEST(DispatcherConfigurationApiIoTests, ImportDocumentReloadsRuntimeConfiguration)
{
    dispatcher::runtime::DispatcherRuntime runtime(
        make_api_config_io_snapshot()
    );

    dispatcher::api::DispatcherConfigurationApi api(runtime);

    const auto result = api.import_document(make_api_import_document());

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_snapshot());

    EXPECT_EQ(result.snapshot().config_version(), 9);
    EXPECT_EQ(result.snapshot().device_count(), 1);
    EXPECT_EQ(result.snapshot().tag_count(), 1);

    const auto imported_tag =
        runtime.telemetry_ingestor()
        .configuration_snapshot()
        .find_tag_by_id(
            dispatcher::domain::TagId{ "tag-pressure" }
        );

    EXPECT_TRUE(imported_tag.has_value());
}

TEST(DispatcherConfigurationApiIoTests, ImportPropagatesParseFailure)
{
    dispatcher::runtime::DispatcherRuntime runtime(
        make_api_config_io_snapshot()
    );

    dispatcher::api::DispatcherConfigurationApi api(runtime);

    const auto result = api.import_document(
        dispatcher::config::ConfigurationDocument::json(
            "{",
            "broken.json"
        )
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::ApiStatus::BadRequest
    );

    EXPECT_EQ(result.error().operation, "configuration.import.parse");
    EXPECT_EQ(result.error().resource, "broken.json");
    EXPECT_EQ(result.error().field, "document.content");
}

TEST(DispatcherConfigurationApiIoTests, ExportRejectsUnsupportedFormat)
{
    dispatcher::runtime::DispatcherRuntime runtime(
        make_api_config_io_snapshot()
    );

    dispatcher::api::DispatcherConfigurationApi api(runtime);

    dispatcher::config::ConfigurationExportOptions options;

    options.format = dispatcher::config::ConfigurationFormat::Unknown;

    const auto result = api.export_current(
        options,
        "current.unknown"
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::ApiStatus::UnsupportedOperation
    );

    EXPECT_FALSE(result.has_document());

    EXPECT_EQ(result.error().operation, "configuration.export");
    EXPECT_EQ(result.error().field, "format");
}