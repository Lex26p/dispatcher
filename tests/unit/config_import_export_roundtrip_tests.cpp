#include <dispatcher/config/configuration_import_export.hpp>
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
    dispatcher::domain::DeviceDefinition make_roundtrip_device_1()
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

    dispatcher::domain::DeviceDefinition make_roundtrip_device_2()
    {
        return dispatcher::domain::DeviceDefinitionBuilder{}
            .device_id(dispatcher::domain::DeviceId{ "device-2" })
            .organization_id(dispatcher::domain::OrganizationId{ "org-1" })
            .site_id(dispatcher::domain::SiteId{ "site-1" })
            .area_id(dispatcher::domain::AreaId{ "area-1" })
            .local_name("device-2")
            .display_name("Device 2")
            .enabled(false)
            .build();
    }

    dispatcher::domain::TagDefinition make_roundtrip_temperature_tag()
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

    dispatcher::domain::TagDefinition make_roundtrip_running_tag()
    {
        return dispatcher::domain::TagDefinitionBuilder{}
            .tag_id(dispatcher::domain::TagId{ "tag-running" })
            .organization_id(dispatcher::domain::OrganizationId{ "org-1" })
            .site_id(dispatcher::domain::SiteId{ "site-1" })
            .area_id(dispatcher::domain::AreaId{ "area-1" })
            .device_id(dispatcher::domain::DeviceId{ "device-2" })
            .local_name("running")
            .display_name("Running")
            .data_type(dispatcher::domain::DataType::Boolean)
            .history_policy(dispatcher::domain::HistoryPolicy::OnChange)
            .enabled(false)
            .build();
    }

    void require_valid(
        const dispatcher::common::ValidationResult& validation,
        const std::string& operation
    )
    {
        if (validation.has_errors())
        {
            throw std::runtime_error(
                operation
                + " failed: "
                + validation.errors().front().field
                + " - "
                + validation.errors().front().message
            );
        }
    }

    dispatcher::domain::ConfigurationSnapshot make_roundtrip_snapshot()
    {
        dispatcher::domain::ConfigurationSnapshotBuilder builder;

        builder.config_version(77);
        builder.status(dispatcher::domain::ConfigurationStatus::Published);

        require_valid(
            builder.add_device(make_roundtrip_device_1()),
            "add device 1"
        );

        require_valid(
            builder.add_device(make_roundtrip_device_2()),
            "add device 2"
        );

        require_valid(
            builder.add_tag(make_roundtrip_temperature_tag()),
            "add temperature tag"
        );

        require_valid(
            builder.add_tag(make_roundtrip_running_tag()),
            "add running tag"
        );

        return builder.build();
    }
}

TEST(ConfigurationImportExportRoundtripTests, UmbrellaHeaderExposesPublicApi)
{
    const auto snapshot = make_roundtrip_snapshot();

    dispatcher::config::ConfigurationExportOptions options;

    options.format = dispatcher::config::ConfigurationFormat::Json;
    options.source = "roundtrip-test";
    options.exported_at = "2026-07-01T00:00:00Z";

    const auto export_result =
        dispatcher::config::ConfigurationExporter::export_snapshot(
            snapshot,
            options,
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
}

TEST(ConfigurationImportExportRoundtripTests, RoundTripsSnapshotContent)
{
    const auto original_snapshot = make_roundtrip_snapshot();

    dispatcher::config::ConfigurationExportOptions options;

    options.format = dispatcher::config::ConfigurationFormat::Json;
    options.source = "roundtrip-test";
    options.exported_at = "2026-07-01T00:00:00Z";

    const auto export_result =
        dispatcher::config::ConfigurationExporter::export_snapshot(
            original_snapshot,
            options,
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

    const auto& imported_snapshot = import_result.snapshot();

    EXPECT_EQ(imported_snapshot.config_version(), 77);

    EXPECT_EQ(
        imported_snapshot.status(),
        dispatcher::domain::ConfigurationStatus::Published
    );

    ASSERT_EQ(imported_snapshot.device_count(), 2);
    ASSERT_EQ(imported_snapshot.tag_count(), 2);

    const auto device_1 = imported_snapshot.find_device_by_id(
        dispatcher::domain::DeviceId{ "device-1" }
    );

    const auto device_2 = imported_snapshot.find_device_by_id(
        dispatcher::domain::DeviceId{ "device-2" }
    );

    ASSERT_TRUE(device_1.has_value());
    ASSERT_TRUE(device_2.has_value());

    EXPECT_EQ(device_1->display_name(), "Device 1");
    EXPECT_TRUE(device_1->enabled());

    EXPECT_EQ(device_2->display_name(), "Device 2");
    EXPECT_FALSE(device_2->enabled());

    const auto temperature = imported_snapshot.find_tag_by_id(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    const auto running = imported_snapshot.find_tag_by_id(
        dispatcher::domain::TagId{ "tag-running" }
    );

    ASSERT_TRUE(temperature.has_value());
    ASSERT_TRUE(running.has_value());

    EXPECT_EQ(temperature->display_name(), "Temperature");

    EXPECT_EQ(
        temperature->data_type(),
        dispatcher::domain::DataType::Float64
    );

    EXPECT_EQ(
        temperature->history_policy(),
        dispatcher::domain::HistoryPolicy::EveryPoll
    );

    EXPECT_TRUE(temperature->enabled());

    EXPECT_EQ(running->display_name(), "Running");

    EXPECT_EQ(
        running->data_type(),
        dispatcher::domain::DataType::Boolean
    );

    EXPECT_EQ(
        running->history_policy(),
        dispatcher::domain::HistoryPolicy::OnChange
    );

    EXPECT_FALSE(running->enabled());
}

TEST(ConfigurationImportExportRoundtripTests, ExportDocumentContainsExpectedJson)
{
    const auto snapshot = make_roundtrip_snapshot();

    dispatcher::config::ConfigurationExportOptions options;

    options.format = dispatcher::config::ConfigurationFormat::Json;
    options.source = "roundtrip-test";
    options.exported_at = "2026-07-01T00:00:00Z";

    const auto export_result =
        dispatcher::config::ConfigurationExporter::export_snapshot(
            snapshot,
            options,
            "roundtrip.json"
        );

    ASSERT_TRUE(export_result.ok());
    ASSERT_TRUE(export_result.has_document());

    const auto& document = export_result.document();

    EXPECT_EQ(
        document.format(),
        dispatcher::config::ConfigurationFormat::Json
    );

    EXPECT_EQ(document.name(), "roundtrip.json");

    const std::string& content = document.content();

    EXPECT_NE(content.find("\"config_version\": 77"), std::string::npos);
    EXPECT_NE(content.find("\"source\": \"roundtrip-test\""), std::string::npos);
    EXPECT_NE(content.find("\"device_id\": \"device-1\""), std::string::npos);
    EXPECT_NE(content.find("\"device_id\": \"device-2\""), std::string::npos);

    EXPECT_NE(
        content.find("\"tag_id\": \"tag-temperature\""),
        std::string::npos
    );

    EXPECT_NE(
        content.find("\"tag_id\": \"tag-running\""),
        std::string::npos
    );

    EXPECT_NE(content.find("\"data_type\": \"boolean\""), std::string::npos);
    EXPECT_NE(content.find("\"history_policy\": \"on_change\""), std::string::npos);
}