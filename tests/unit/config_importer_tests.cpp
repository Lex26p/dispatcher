#include <dispatcher/config/configuration_document.hpp>
#include <dispatcher/config/configuration_export_options.hpp>
#include <dispatcher/config/configuration_exporter.hpp>
#include <dispatcher/config/configuration_format.hpp>
#include <dispatcher/config/configuration_importer.hpp>
#include <dispatcher/config/configuration_io_status.hpp>
#include <dispatcher/domain/configuration_snapshot_builder.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/device_definition_builder.hpp>
#include <dispatcher/domain/history_policy.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/tag_definition_builder.hpp>

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

namespace
{
    dispatcher::domain::DeviceDefinition make_importer_device()
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

    dispatcher::domain::TagDefinition make_importer_tag()
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
            .enabled(false)
            .build();
    }

    dispatcher::domain::ConfigurationSnapshot make_importer_snapshot()
    {
        dispatcher::domain::ConfigurationSnapshotBuilder builder;

        builder.config_version(7);
        builder.status(dispatcher::domain::ConfigurationStatus::Published);

        const auto device_result = builder.add_device(
            make_importer_device()
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
            make_importer_tag()
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

    dispatcher::config::ConfigurationDocument make_importer_json_document()
    {
        return dispatcher::config::ConfigurationDocument::json(
            R"json({
  "schema_version": "dispatcher.config.v1",
  "format": "json",
  "config_version": 7,
  "status": "published",
  "source": "production.json",
  "devices": [
    {
      "organization_id": "org-1",
      "site_id": "site-1",
      "area_id": "area-1",
      "device_id": "device-1",
      "local_name": "device-1",
      "display_name": "Device 1",
      "enabled": true
    }
  ],
  "tags": [
    {
      "organization_id": "org-1",
      "site_id": "site-1",
      "area_id": "area-1",
      "device_id": "device-1",
      "tag_id": "tag-temperature",
      "local_name": "temperature",
      "display_name": "Temperature",
      "data_type": "float64",
      "history_policy": "every_poll",
      "enabled": false
    }
  ]
}
)json",
"production.json"
);
    }
}

TEST(ConfigurationImporterTests, ImportsJsonDocumentToSnapshot)
{
    const auto result =
        dispatcher::config::ConfigurationImporter::import_document(
            make_importer_json_document()
        );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_snapshot());

    const auto& snapshot = result.snapshot();

    EXPECT_EQ(snapshot.config_version(), 7);

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::domain::ConfigurationStatus::Published
    );

    ASSERT_EQ(snapshot.device_count(), 1);
    ASSERT_EQ(snapshot.tag_count(), 1);

    const auto& device = snapshot.devices().front();

    EXPECT_EQ(
        device.organization_id(),
        dispatcher::domain::OrganizationId{ "org-1" }
    );

    EXPECT_EQ(device.site_id(), dispatcher::domain::SiteId{ "site-1" });
    EXPECT_EQ(device.area_id(), dispatcher::domain::AreaId{ "area-1" });
    EXPECT_EQ(device.device_id(), dispatcher::domain::DeviceId{ "device-1" });
    EXPECT_EQ(device.local_name(), "device-1");
    EXPECT_EQ(device.display_name(), "Device 1");
    EXPECT_TRUE(device.enabled());

    const auto& tag = snapshot.tags().front();

    EXPECT_EQ(tag.organization_id(), dispatcher::domain::OrganizationId{ "org-1" });
    EXPECT_EQ(tag.site_id(), dispatcher::domain::SiteId{ "site-1" });
    EXPECT_EQ(tag.area_id(), dispatcher::domain::AreaId{ "area-1" });
    EXPECT_EQ(tag.device_id(), dispatcher::domain::DeviceId{ "device-1" });
    EXPECT_EQ(tag.tag_id(), dispatcher::domain::TagId{ "tag-temperature" });
    EXPECT_EQ(tag.local_name(), "temperature");
    EXPECT_EQ(tag.display_name(), "Temperature");

    EXPECT_EQ(
        tag.data_type(),
        dispatcher::domain::DataType::Float64
    );

    EXPECT_EQ(
        tag.history_policy(),
        dispatcher::domain::HistoryPolicy::EveryPoll
    );

    EXPECT_FALSE(tag.enabled());
}

TEST(ConfigurationImporterTests, RoundTripsExporterDocument)
{
    const auto export_result =
        dispatcher::config::ConfigurationExporter::export_snapshot(
            make_importer_snapshot(),
            dispatcher::config::ConfigurationExportOptions{
                .format = dispatcher::config::ConfigurationFormat::Json,
                .source = "unit-test",
                .exported_at = "2026-07-01T00:00:00Z"
            },
            "roundtrip.json"
        );

    ASSERT_TRUE(export_result.ok());
    ASSERT_TRUE(export_result.has_document());

    const auto import_result =
        dispatcher::config::ConfigurationImporter::import_document(
            export_result.document()
        );

    ASSERT_TRUE(import_result.ok());
    ASSERT_TRUE(import_result.has_snapshot());

    const auto& snapshot = import_result.snapshot();

    EXPECT_EQ(snapshot.config_version(), 7);

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::domain::ConfigurationStatus::Published
    );

    ASSERT_EQ(snapshot.device_count(), 1);
    ASSERT_EQ(snapshot.tag_count(), 1);

    EXPECT_EQ(
        snapshot.devices().front().device_id(),
        dispatcher::domain::DeviceId{ "device-1" }
    );

    EXPECT_EQ(
        snapshot.tags().front().tag_id(),
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    EXPECT_EQ(
        snapshot.tags().front().data_type(),
        dispatcher::domain::DataType::Float64
    );

    EXPECT_EQ(
        snapshot.tags().front().history_policy(),
        dispatcher::domain::HistoryPolicy::EveryPoll
    );
}

TEST(ConfigurationImporterTests, PropagatesParserFailure)
{
    const auto document = dispatcher::config::ConfigurationDocument::json(
        "{",
        "broken.json"
    );

    const auto result =
        dispatcher::config::ConfigurationImporter::import_document(document);

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());
    EXPECT_FALSE(result.has_snapshot());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ParseError
    );

    EXPECT_EQ(result.error().operation, "configuration.import.parse");
    EXPECT_EQ(result.error().resource, "broken.json");
    EXPECT_EQ(result.error().field, "document.content");
    EXPECT_EQ(
        result.error().message,
        "configuration document is not valid json"
    );
}

TEST(ConfigurationImporterTests, PropagatesMapperFailure)
{
    const auto document = dispatcher::config::ConfigurationDocument::json(
        R"json({
  "schema_version": "dispatcher.config.v1",
  "format": "json",
  "config_version": 7,
  "status": "archived",
  "devices": [],
  "tags": []
}
)json",
"invalid-status.json"
);

    const auto result =
        dispatcher::config::ConfigurationImporter::import_document(document);

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());
    EXPECT_FALSE(result.has_snapshot());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ValidationError
    );

    EXPECT_EQ(result.error().operation, "configuration.import.map");
    EXPECT_EQ(result.error().resource, "invalid-status.json");
    EXPECT_EQ(result.error().field, "metadata.status");
    EXPECT_EQ(result.error().message, "configuration status is invalid");
}

TEST(ConfigurationImporterTests, PropagatesDomainValidationFailure)
{
    const auto document = dispatcher::config::ConfigurationDocument::json(
        R"json({
  "schema_version": "dispatcher.config.v1",
  "format": "json",
  "config_version": 7,
  "status": "published",
  "devices": [
    {
      "organization_id": "org-1",
      "site_id": "site-1",
      "area_id": "area-1",
      "device_id": "device-1",
      "local_name": "device-1",
      "display_name": "Device 1",
      "enabled": true
    },
    {
      "organization_id": "org-1",
      "site_id": "site-1",
      "area_id": "area-1",
      "device_id": "device-1",
      "local_name": "device-1-duplicate",
      "display_name": "Device 1 Duplicate",
      "enabled": true
    }
  ],
  "tags": []
}
)json",
"duplicate-device.json"
);

    const auto result =
        dispatcher::config::ConfigurationImporter::import_document(document);

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());
    EXPECT_FALSE(result.has_snapshot());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ValidationError
    );

    EXPECT_EQ(result.error().operation, "configuration.import.map");
    EXPECT_EQ(result.error().resource, "duplicate-device.json");

    EXPECT_NE(
        result.error().field.find("devices[1]."),
        std::string::npos
    );

    EXPECT_FALSE(result.error().message.empty());
}