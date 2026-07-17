#include <dispatcher/config/configuration_export_device.hpp>
#include <dispatcher/config/configuration_export_metadata.hpp>
#include <dispatcher/config/configuration_export_model.hpp>
#include <dispatcher/config/configuration_export_model_result.hpp>
#include <dispatcher/config/configuration_export_tag.hpp>
#include <dispatcher/config/configuration_format.hpp>
#include <dispatcher/config/configuration_io_status.hpp>

#include <gtest/gtest.h>

#include <stdexcept>

namespace
{
    dispatcher::config::ConfigurationExportDevice make_export_device()
    {
        return dispatcher::config::ConfigurationExportDevice{
            .organization_id = "org-1",
            .site_id = "site-1",
            .area_id = "area-1",
            .device_id = "device-1",
            .local_name = "device-1",
            .display_name = "Device 1",
            .enabled = true
        };
    }

    dispatcher::config::ConfigurationExportTag make_export_tag()
    {
        return dispatcher::config::ConfigurationExportTag{
            .organization_id = "org-1",
            .site_id = "site-1",
            .area_id = "area-1",
            .device_id = "device-1",
            .tag_id = "tag-temperature",
            .local_name = "temperature",
            .display_name = "Temperature",
            .data_type = "float64",
            .history_policy = "every_poll",
            .enabled = true
        };
    }
}

TEST(ConfigurationExportMetadataTests, DefaultMetadataIsUsable)
{
    const dispatcher::config::ConfigurationExportMetadata metadata;

    EXPECT_TRUE(metadata.has_schema_version());
    EXPECT_TRUE(metadata.has_known_format());
    EXPECT_FALSE(metadata.has_config_version());
    EXPECT_FALSE(metadata.has_status());
    EXPECT_FALSE(metadata.has_description());
    EXPECT_FALSE(metadata.has_source());
    EXPECT_FALSE(metadata.has_exported_at());

    EXPECT_EQ(
        metadata.format,
        dispatcher::config::ConfigurationFormat::Json
    );
}

TEST(ConfigurationExportMetadataTests, MetadataPredicatesReflectFields)
{
    const dispatcher::config::ConfigurationExportMetadata metadata{
        .schema_version = "dispatcher.config.v1",
        .format = dispatcher::config::ConfigurationFormat::Json,
        .config_version = 7,
        .status = "published",
        .description = "Production configuration",
        .source = "unit-test",
        .exported_at = "2026-07-01T00:00:00Z"
    };

    EXPECT_TRUE(metadata.has_schema_version());
    EXPECT_TRUE(metadata.has_known_format());
    EXPECT_TRUE(metadata.has_config_version());
    EXPECT_TRUE(metadata.has_status());
    EXPECT_TRUE(metadata.has_description());
    EXPECT_TRUE(metadata.has_source());
    EXPECT_TRUE(metadata.has_exported_at());
}

TEST(ConfigurationExportDeviceTests, DevicePredicatesReflectFields)
{
    const auto device = make_export_device();

    EXPECT_TRUE(device.has_organization_id());
    EXPECT_TRUE(device.has_site_id());
    EXPECT_TRUE(device.has_area_id());
    EXPECT_TRUE(device.has_device_id());
    EXPECT_TRUE(device.has_local_name());
    EXPECT_TRUE(device.has_display_name());
    EXPECT_TRUE(device.has_required_identity());
    EXPECT_TRUE(device.enabled);
}

TEST(ConfigurationExportDeviceTests, MissingDeviceIdentityIsInvalid)
{
    dispatcher::config::ConfigurationExportDevice device;

    device.organization_id = "org-1";
    device.site_id = "site-1";
    device.area_id = "area-1";

    EXPECT_FALSE(device.has_device_id());
    EXPECT_FALSE(device.has_required_identity());
}

TEST(ConfigurationExportTagTests, TagPredicatesReflectFields)
{
    const auto tag = make_export_tag();

    EXPECT_TRUE(tag.has_organization_id());
    EXPECT_TRUE(tag.has_site_id());
    EXPECT_TRUE(tag.has_area_id());
    EXPECT_TRUE(tag.has_device_id());
    EXPECT_TRUE(tag.has_tag_id());
    EXPECT_TRUE(tag.has_local_name());
    EXPECT_TRUE(tag.has_display_name());
    EXPECT_TRUE(tag.has_data_type());
    EXPECT_TRUE(tag.has_history_policy());
    EXPECT_TRUE(tag.has_required_identity());
    EXPECT_TRUE(tag.enabled);
}

TEST(ConfigurationExportTagTests, MissingTagIdentityIsInvalid)
{
    dispatcher::config::ConfigurationExportTag tag;

    tag.organization_id = "org-1";
    tag.site_id = "site-1";
    tag.area_id = "area-1";
    tag.device_id = "device-1";

    EXPECT_FALSE(tag.has_tag_id());
    EXPECT_FALSE(tag.has_required_identity());
}

TEST(ConfigurationExportModelTests, EmptyFactoryCreatesKnownFormatModel)
{
    const auto model = dispatcher::config::ConfigurationExportModel::create_empty();

    EXPECT_TRUE(model.metadata().has_known_format());
    EXPECT_EQ(
        model.metadata().format,
        dispatcher::config::ConfigurationFormat::Json
    );

    EXPECT_TRUE(model.empty());
    EXPECT_FALSE(model.has_devices());
    EXPECT_FALSE(model.has_tags());
    EXPECT_EQ(model.device_count(), 0);
    EXPECT_EQ(model.tag_count(), 0);
}

TEST(ConfigurationExportModelTests, ModelStoresDevicesAndTags)
{
    dispatcher::config::ConfigurationExportModel model =
        dispatcher::config::ConfigurationExportModel::create_empty();

    model.metadata().config_version = 7;
    model.metadata().status = "published";

    model.add_device(make_export_device());
    model.add_tag(make_export_tag());

    EXPECT_FALSE(model.empty());
    EXPECT_TRUE(model.has_devices());
    EXPECT_TRUE(model.has_tags());

    EXPECT_EQ(model.device_count(), 1);
    EXPECT_EQ(model.tag_count(), 1);

    ASSERT_NE(model.find_device_by_id("device-1"), nullptr);
    EXPECT_EQ(model.find_device_by_id("device-1")->display_name, "Device 1");

    ASSERT_NE(model.find_tag_by_id("tag-temperature"), nullptr);
    EXPECT_EQ(
        model.find_tag_by_id("tag-temperature")->data_type,
        "float64"
    );

    EXPECT_EQ(model.find_device_by_id("missing-device"), nullptr);
    EXPECT_EQ(model.find_tag_by_id("missing-tag"), nullptr);
}

TEST(ConfigurationExportModelTests, ClearRemovesDevicesAndTags)
{
    dispatcher::config::ConfigurationExportModel model =
        dispatcher::config::ConfigurationExportModel::create_empty();

    model.add_device(make_export_device());
    model.add_tag(make_export_tag());

    ASSERT_FALSE(model.empty());

    model.clear();

    EXPECT_TRUE(model.empty());
    EXPECT_EQ(model.device_count(), 0);
    EXPECT_EQ(model.tag_count(), 0);
}

TEST(ConfigurationExportModelResultTests, SuccessResultContainsModel)
{
    dispatcher::config::ConfigurationExportModel model =
        dispatcher::config::ConfigurationExportModel::create_empty();

    model.metadata().config_version = 7;
    model.add_device(make_export_device());

    const auto result =
        dispatcher::config::ConfigurationExportModelResult::success(model);

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::Success
    );

    EXPECT_TRUE(result.has_model());
    EXPECT_EQ(result.model().metadata().config_version, 7);
    EXPECT_EQ(result.model().device_count(), 1);
}

TEST(ConfigurationExportModelResultTests, FailureResultDoesNotContainModel)
{
    const auto result =
        dispatcher::config::ConfigurationExportModelResult::failure(
            dispatcher::config::ConfigurationIoStatus::ValidationError,
            "configuration.export",
            "config-version-7",
            "devices",
            "device identity is incomplete"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ValidationError
    );

    EXPECT_FALSE(result.has_model());

    EXPECT_EQ(result.error().operation, "configuration.export");
    EXPECT_EQ(result.error().resource, "config-version-7");
    EXPECT_EQ(result.error().field, "devices");
    EXPECT_EQ(result.error().message, "device identity is incomplete");

    EXPECT_THROW(
        (void)result.model(),
        std::logic_error
    );
}