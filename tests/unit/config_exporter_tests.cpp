#include <dispatcher/config/configuration_export_options.hpp>
#include <dispatcher/config/configuration_exporter.hpp>
#include <dispatcher/config/configuration_format.hpp>
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
    dispatcher::domain::DeviceDefinition make_exporter_device()
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

    dispatcher::domain::TagDefinition make_exporter_tag()
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

    dispatcher::domain::ConfigurationSnapshot make_exporter_snapshot()
    {
        dispatcher::domain::ConfigurationSnapshotBuilder builder;

        builder.config_version(7);
        builder.status(dispatcher::domain::ConfigurationStatus::Published);

        const auto device_result = builder.add_device(
            make_exporter_device()
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
            make_exporter_tag()
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
}

TEST(ConfigurationExporterTests, ExportsSnapshotToJsonDocument)
{
    const auto result = dispatcher::config::ConfigurationExporter::export_snapshot(
        make_exporter_snapshot(),
        dispatcher::config::ConfigurationExportOptions{
            .format = dispatcher::config::ConfigurationFormat::Json,
            .source = "unit-test",
            .exported_at = "2026-07-01T00:00:00Z"
        },
        "production.json"
    );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_document());

    const auto& document = result.document();

    EXPECT_EQ(
        document.format(),
        dispatcher::config::ConfigurationFormat::Json
    );

    EXPECT_EQ(document.name(), "production.json");
    EXPECT_FALSE(document.empty());

    const std::string& content = document.content();

    EXPECT_NE(content.find("\"schema_version\": \"dispatcher.config.v1\""), std::string::npos);
    EXPECT_NE(content.find("\"format\": \"json\""), std::string::npos);
    EXPECT_NE(content.find("\"config_version\": 7"), std::string::npos);
    EXPECT_NE(content.find("\"status\": \"published\""), std::string::npos);
    EXPECT_NE(content.find("\"source\": \"unit-test\""), std::string::npos);
    EXPECT_NE(content.find("\"exported_at\": \"2026-07-01T00:00:00Z\""), std::string::npos);

    EXPECT_NE(content.find("\"devices\": ["), std::string::npos);
    EXPECT_NE(content.find("\"tags\": ["), std::string::npos);

    EXPECT_NE(content.find("\"device_id\": \"device-1\""), std::string::npos);
    EXPECT_NE(content.find("\"tag_id\": \"tag-temperature\""), std::string::npos);
    EXPECT_NE(content.find("\"data_type\": \"float64\""), std::string::npos);
    EXPECT_NE(content.find("\"history_policy\": \"every_poll\""), std::string::npos);
}

TEST(ConfigurationExporterTests, CanExportMetadataOnly)
{
    const auto result = dispatcher::config::ConfigurationExporter::export_snapshot(
        make_exporter_snapshot(),
        dispatcher::config::ConfigurationExportOptions{
            .format = dispatcher::config::ConfigurationFormat::Json,
            .include_devices = false,
            .include_tags = false
        }
    );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_document());

    const std::string& content = result.document().content();

    EXPECT_NE(content.find("\"config_version\": 7"), std::string::npos);
    EXPECT_NE(content.find("\"devices\": []"), std::string::npos);
    EXPECT_NE(content.find("\"tags\": []"), std::string::npos);

    EXPECT_EQ(content.find("\"device_id\": \"device-1\""), std::string::npos);
    EXPECT_EQ(content.find("\"tag_id\": \"tag-temperature\""), std::string::npos);
}

TEST(ConfigurationExporterTests, PropagatesUnsupportedFormatFailure)
{
    const auto result = dispatcher::config::ConfigurationExporter::export_snapshot(
        make_exporter_snapshot(),
        dispatcher::config::ConfigurationExportOptions{
            .format = dispatcher::config::ConfigurationFormat::Unknown
        },
        "production.unknown"
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::UnsupportedFormat
    );

    EXPECT_FALSE(result.has_document());

    EXPECT_EQ(result.error().operation, "configuration.export");
    EXPECT_EQ(result.error().resource, "7");
    EXPECT_EQ(result.error().field, "format");
    EXPECT_EQ(
        result.error().message,
        "unsupported configuration export format"
    );
}