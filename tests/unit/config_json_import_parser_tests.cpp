#include <dispatcher/config/configuration_document.hpp>
#include <dispatcher/config/configuration_format.hpp>
#include <dispatcher/config/configuration_io_status.hpp>
#include <dispatcher/config/configuration_json_import_parser.hpp>

#include <gtest/gtest.h>

#include <string>

namespace
{
    dispatcher::config::ConfigurationDocument make_import_json_document()
    {
        return dispatcher::config::ConfigurationDocument::json(
            R"json({
  "schema_version": "dispatcher.config.v1",
  "format": "json",
  "config_version": 7,
  "status": "published",
  "description": "Production configuration",
  "source": "unit-test",
  "exported_at": "2026-07-01T00:00:00Z",
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

TEST(ConfigurationJsonImportParserTests, ParsesMetadata)
{
    const auto result =
        dispatcher::config::ConfigurationJsonImportParser::parse(
            make_import_json_document()
        );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_model());

    const auto& model = result.model();

    EXPECT_EQ(model.metadata().schema_version, "dispatcher.config.v1");

    EXPECT_EQ(
        model.metadata().format,
        dispatcher::config::ConfigurationFormat::Json
    );

    EXPECT_EQ(model.metadata().config_version, 7);
    EXPECT_EQ(model.metadata().status, "published");
    EXPECT_EQ(model.metadata().description, "Production configuration");
    EXPECT_EQ(model.metadata().source, "unit-test");

    EXPECT_EQ(
        model.metadata().imported_at,
        "2026-07-01T00:00:00Z"
    );
}

TEST(ConfigurationJsonImportParserTests, ParsesDevicesAndTags)
{
    const auto result =
        dispatcher::config::ConfigurationJsonImportParser::parse(
            make_import_json_document()
        );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_model());

    const auto& model = result.model();

    ASSERT_EQ(model.device_count(), 1);
    ASSERT_EQ(model.tag_count(), 1);

    const auto& device = model.devices().front();

    EXPECT_EQ(device.organization_id, "org-1");
    EXPECT_EQ(device.site_id, "site-1");
    EXPECT_EQ(device.area_id, "area-1");
    EXPECT_EQ(device.device_id, "device-1");
    EXPECT_EQ(device.local_name, "device-1");
    EXPECT_EQ(device.display_name, "Device 1");
    EXPECT_TRUE(device.enabled);

    const auto& tag = model.tags().front();

    EXPECT_EQ(tag.organization_id, "org-1");
    EXPECT_EQ(tag.site_id, "site-1");
    EXPECT_EQ(tag.area_id, "area-1");
    EXPECT_EQ(tag.device_id, "device-1");
    EXPECT_EQ(tag.tag_id, "tag-temperature");
    EXPECT_EQ(tag.local_name, "temperature");
    EXPECT_EQ(tag.display_name, "Temperature");
    EXPECT_EQ(tag.data_type, "float64");
    EXPECT_EQ(tag.history_policy, "every_poll");
    EXPECT_FALSE(tag.enabled);
}

TEST(ConfigurationJsonImportParserTests, UsesDocumentNameAsSourceFallback)
{
    const auto document = dispatcher::config::ConfigurationDocument::json(
        R"json({
  "schema_version": "dispatcher.config.v1",
  "format": "json",
  "config_version": 7,
  "status": "published",
  "devices": [],
  "tags": []
}
)json",
"production.json"
);

    const auto result =
        dispatcher::config::ConfigurationJsonImportParser::parse(document);

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_model());

    EXPECT_EQ(result.model().metadata().source, "production.json");
}

TEST(ConfigurationJsonImportParserTests, MissingCollectionsBecomeEmpty)
{
    const auto document = dispatcher::config::ConfigurationDocument::json(
        R"json({
  "schema_version": "dispatcher.config.v1",
  "format": "json",
  "config_version": 7,
  "status": "published"
}
)json",
"production.json"
);

    const auto result =
        dispatcher::config::ConfigurationJsonImportParser::parse(document);

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_model());

    EXPECT_EQ(result.model().device_count(), 0);
    EXPECT_EQ(result.model().tag_count(), 0);
}

TEST(ConfigurationJsonImportParserTests, RejectsNonJsonDocument)
{
    const dispatcher::config::ConfigurationDocument document{
        dispatcher::config::ConfigurationFormat::Unknown,
        "{}",
        "production.yaml"
    };

    const auto result =
        dispatcher::config::ConfigurationJsonImportParser::parse(document);

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::UnsupportedFormat
    );

    EXPECT_EQ(result.error().operation, "configuration.import.parse");
    EXPECT_EQ(result.error().resource, "production.yaml");
    EXPECT_EQ(result.error().field, "document.format");
    EXPECT_EQ(result.error().message, "configuration document is not json");
}

TEST(ConfigurationJsonImportParserTests, RejectsEmptyDocument)
{
    const auto document =
        dispatcher::config::ConfigurationDocument::json(
            "",
            "production.json"
        );

    const auto result =
        dispatcher::config::ConfigurationJsonImportParser::parse(document);

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ParseError
    );

    EXPECT_EQ(result.error().field, "document.content");
    EXPECT_EQ(result.error().message, "configuration document is empty");
}

TEST(ConfigurationJsonImportParserTests, RejectsMalformedJson)
{
    const auto document =
        dispatcher::config::ConfigurationDocument::json(
            "{",
            "production.json"
        );

    const auto result =
        dispatcher::config::ConfigurationJsonImportParser::parse(document);

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ParseError
    );

    EXPECT_EQ(result.error().field, "document.content");
    EXPECT_EQ(
        result.error().message,
        "configuration document is not valid json"
    );
}

TEST(ConfigurationJsonImportParserTests, RejectsNonObjectRoot)
{
    const auto document =
        dispatcher::config::ConfigurationDocument::json(
            "[]",
            "production.json"
        );

    const auto result =
        dispatcher::config::ConfigurationJsonImportParser::parse(document);

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ParseError
    );

    EXPECT_EQ(result.error().field, "document");
    EXPECT_EQ(
        result.error().message,
        "configuration document root must be an object"
    );
}

TEST(ConfigurationJsonImportParserTests, RejectsUnsupportedFormatField)
{
    const auto document =
        dispatcher::config::ConfigurationDocument::json(
            R"json({
  "format": "yaml",
  "config_version": 7,
  "status": "published"
}
)json",
"production.json"
);

    const auto result =
        dispatcher::config::ConfigurationJsonImportParser::parse(document);

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::UnsupportedFormat
    );

    EXPECT_EQ(result.error().field, "format");
    EXPECT_EQ(
        result.error().message,
        "unsupported configuration import format"
    );
}

TEST(ConfigurationJsonImportParserTests, RejectsWrongConfigVersionType)
{
    const auto document =
        dispatcher::config::ConfigurationDocument::json(
            R"json({
  "format": "json",
  "config_version": "7",
  "status": "published"
}
)json",
"production.json"
);

    const auto result =
        dispatcher::config::ConfigurationJsonImportParser::parse(document);

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ParseError
    );

    EXPECT_EQ(result.error().field, "config_version");
    EXPECT_EQ(result.error().message, "expected unsigned integer");
}

TEST(ConfigurationJsonImportParserTests, RejectsDevicesWhenNotArray)
{
    const auto document =
        dispatcher::config::ConfigurationDocument::json(
            R"json({
  "format": "json",
  "config_version": 7,
  "status": "published",
  "devices": {}
}
)json",
"production.json"
);

    const auto result =
        dispatcher::config::ConfigurationJsonImportParser::parse(document);

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ParseError
    );

    EXPECT_EQ(result.error().field, "devices");
    EXPECT_EQ(result.error().message, "expected array");
}

TEST(ConfigurationJsonImportParserTests, RejectsDeviceFieldWithWrongType)
{
    const auto document =
        dispatcher::config::ConfigurationDocument::json(
            R"json({
  "format": "json",
  "config_version": 7,
  "status": "published",
  "devices": [
    {
      "device_id": 42
    }
  ]
}
)json",
"production.json"
);

    const auto result =
        dispatcher::config::ConfigurationJsonImportParser::parse(document);

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ParseError
    );

    EXPECT_EQ(result.error().field, "devices[0].device_id");
    EXPECT_EQ(result.error().message, "expected string");
}

TEST(ConfigurationJsonImportParserTests, RejectsTagsWhenNotArray)
{
    const auto document =
        dispatcher::config::ConfigurationDocument::json(
            R"json({
  "format": "json",
  "config_version": 7,
  "status": "published",
  "tags": {}
}
)json",
"production.json"
);

    const auto result =
        dispatcher::config::ConfigurationJsonImportParser::parse(document);

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ParseError
    );

    EXPECT_EQ(result.error().field, "tags");
    EXPECT_EQ(result.error().message, "expected array");
}

TEST(ConfigurationJsonImportParserTests, RejectsTagFieldWithWrongType)
{
    const auto document =
        dispatcher::config::ConfigurationDocument::json(
            R"json({
  "format": "json",
  "config_version": 7,
  "status": "published",
  "tags": [
    {
      "tag_id": 42
    }
  ]
}
)json",
"production.json"
);

    const auto result =
        dispatcher::config::ConfigurationJsonImportParser::parse(document);

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ParseError
    );

    EXPECT_EQ(result.error().field, "tags[0].tag_id");
    EXPECT_EQ(result.error().message, "expected string");
}