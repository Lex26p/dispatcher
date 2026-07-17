#include <dispatcher/config/configuration_export_device.hpp>
#include <dispatcher/config/configuration_export_metadata.hpp>
#include <dispatcher/config/configuration_export_model.hpp>
#include <dispatcher/config/configuration_export_tag.hpp>
#include <dispatcher/config/configuration_format.hpp>
#include <dispatcher/config/configuration_io_status.hpp>
#include <dispatcher/config/configuration_json_export_serializer.hpp>

#include <gtest/gtest.h>

#include <string>

namespace
{
    dispatcher::config::ConfigurationExportDevice make_json_export_device()
    {
        return dispatcher::config::ConfigurationExportDevice{
            .organization_id = "org-1",
            .site_id = "site-1",
            .area_id = "area-1",
            .device_id = "device-1",
            .local_name = "device-1",
            .display_name = "Device \"A\"",
            .enabled = true
        };
    }

    dispatcher::config::ConfigurationExportTag make_json_export_tag()
    {
        return dispatcher::config::ConfigurationExportTag{
            .organization_id = "org-1",
            .site_id = "site-1",
            .area_id = "area-1",
            .device_id = "device-1",
            .tag_id = "tag-temperature",
            .local_name = "temperature",
            .display_name = "Temperature\nSensor",
            .data_type = "float64",
            .history_policy = "every_poll",
            .enabled = false
        };
    }

    dispatcher::config::ConfigurationExportModel make_json_export_model()
    {
        dispatcher::config::ConfigurationExportMetadata metadata;

        metadata.config_version = 7;
        metadata.status = "published";
        metadata.description = "Production configuration";
        metadata.source = "unit-test";
        metadata.exported_at = "2026-07-01T00:00:00Z";

        dispatcher::config::ConfigurationExportModel model(metadata);

        model.add_device(make_json_export_device());
        model.add_tag(make_json_export_tag());

        return model;
    }
}

TEST(ConfigurationJsonExportSerializerTests, SerializesMetadata)
{
    const auto result =
        dispatcher::config::ConfigurationJsonExportSerializer::serialize(
            make_json_export_model(),
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

    EXPECT_NE(
        content.find("\"schema_version\": \"dispatcher.config.v1\""),
        std::string::npos
    );

    EXPECT_NE(content.find("\"format\": \"json\""), std::string::npos);
    EXPECT_NE(content.find("\"config_version\": 7"), std::string::npos);
    EXPECT_NE(content.find("\"status\": \"published\""), std::string::npos);

    EXPECT_NE(
        content.find("\"description\": \"Production configuration\""),
        std::string::npos
    );

    EXPECT_NE(content.find("\"source\": \"unit-test\""), std::string::npos);

    EXPECT_NE(
        content.find("\"exported_at\": \"2026-07-01T00:00:00Z\""),
        std::string::npos
    );
}

TEST(ConfigurationJsonExportSerializerTests, SerializesDevicesAndTags)
{
    const auto result =
        dispatcher::config::ConfigurationJsonExportSerializer::serialize(
            make_json_export_model()
        );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_document());

    const std::string& content = result.document().content();

    EXPECT_NE(content.find("\"devices\": ["), std::string::npos);
    EXPECT_NE(content.find("\"tags\": ["), std::string::npos);

    EXPECT_NE(content.find("\"organization_id\": \"org-1\""), std::string::npos);
    EXPECT_NE(content.find("\"site_id\": \"site-1\""), std::string::npos);
    EXPECT_NE(content.find("\"area_id\": \"area-1\""), std::string::npos);
    EXPECT_NE(content.find("\"device_id\": \"device-1\""), std::string::npos);
    EXPECT_NE(content.find("\"tag_id\": \"tag-temperature\""), std::string::npos);

    EXPECT_NE(content.find("\"data_type\": \"float64\""), std::string::npos);

    EXPECT_NE(
        content.find("\"history_policy\": \"every_poll\""),
        std::string::npos
    );
}

TEST(ConfigurationJsonExportSerializerTests, EscapesJsonStrings)
{
    const auto result =
        dispatcher::config::ConfigurationJsonExportSerializer::serialize(
            make_json_export_model()
        );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_document());

    const std::string& content = result.document().content();

    EXPECT_NE(
        content.find("\"display_name\": \"Device \\\"A\\\"\""),
        std::string::npos
    );

    EXPECT_NE(
        content.find("\"display_name\": \"Temperature\\nSensor\""),
        std::string::npos
    );
}

TEST(ConfigurationJsonExportSerializerTests, SerializesEmptyCollections)
{
    dispatcher::config::ConfigurationExportMetadata metadata;

    metadata.config_version = 11;
    metadata.status = "draft";

    const dispatcher::config::ConfigurationExportModel model(metadata);

    const auto result =
        dispatcher::config::ConfigurationJsonExportSerializer::serialize(model);

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_document());

    const std::string& content = result.document().content();

    EXPECT_NE(content.find("\"config_version\": 11"), std::string::npos);
    EXPECT_NE(content.find("\"status\": \"draft\""), std::string::npos);
    EXPECT_NE(content.find("\"devices\": []"), std::string::npos);
    EXPECT_NE(content.find("\"tags\": []"), std::string::npos);
}

TEST(ConfigurationJsonExportSerializerTests, RejectsNonJsonModel)
{
    dispatcher::config::ConfigurationExportMetadata metadata;

    metadata.format = dispatcher::config::ConfigurationFormat::Unknown;
    metadata.config_version = 7;

    const dispatcher::config::ConfigurationExportModel model(metadata);

    const auto result =
        dispatcher::config::ConfigurationJsonExportSerializer::serialize(
            model,
            "production.yaml"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::UnsupportedFormat
    );

    EXPECT_FALSE(result.has_document());

    EXPECT_EQ(result.error().operation, "configuration.export.serialize");
    EXPECT_EQ(result.error().resource, "7");
    EXPECT_EQ(result.error().field, "metadata.format");
    EXPECT_EQ(
        result.error().message,
        "configuration export model is not json"
    );
}