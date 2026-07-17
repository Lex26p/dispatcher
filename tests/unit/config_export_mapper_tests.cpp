#include <dispatcher/config/configuration_export_options.hpp>
#include <dispatcher/config/configuration_format.hpp>
#include <dispatcher/config/configuration_io_status.hpp>
#include <dispatcher/config/configuration_snapshot_export_mapper.hpp>
#include <dispatcher/domain/configuration_snapshot_builder.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/device_definition_builder.hpp>
#include <dispatcher/domain/history_policy.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/tag_definition_builder.hpp>

#include <gtest/gtest.h>

#include <stdexcept>
#include <cstdint>

namespace
{
    dispatcher::domain::ConfigurationSnapshot make_export_mapper_snapshot(
        std::uint64_t config_version,
        dispatcher::domain::ConfigurationStatus status =
        dispatcher::domain::ConfigurationStatus::Published
    )
    {
        return dispatcher::domain::ConfigurationSnapshotBuilder{}
            .config_version(config_version)
            .status(status)
            .build();
    }

    dispatcher::domain::DeviceDefinition make_export_mapper_device()
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

    dispatcher::domain::TagDefinition make_export_mapper_tag()
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

    dispatcher::domain::ConfigurationSnapshot
        make_export_mapper_snapshot_with_device_and_tag()
    {
        dispatcher::domain::ConfigurationSnapshotBuilder builder;

        builder.config_version(7);
        builder.status(dispatcher::domain::ConfigurationStatus::Published);
        builder.description("Production configuration");

        const auto device_result = builder.add_device(
            make_export_mapper_device()
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
            make_export_mapper_tag()
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

TEST(ConfigurationExportOptionsTests, DefaultOptionsRequestJsonFullExport)
{
    const dispatcher::config::ConfigurationExportOptions options;

    EXPECT_TRUE(options.has_known_format());
    EXPECT_FALSE(options.has_source());
    EXPECT_FALSE(options.has_exported_at());
    EXPECT_TRUE(options.requests_devices());
    EXPECT_TRUE(options.requests_tags());

    EXPECT_EQ(
        options.format,
        dispatcher::config::ConfigurationFormat::Json
    );
}

TEST(ConfigurationExportOptionsTests, PredicatesReflectConfiguredFields)
{
    const dispatcher::config::ConfigurationExportOptions options{
        .format = dispatcher::config::ConfigurationFormat::Json,
        .source = "unit-test",
        .exported_at = "2026-07-01T00:00:00Z",
        .include_devices = false,
        .include_tags = false
    };

    EXPECT_TRUE(options.has_known_format());
    EXPECT_TRUE(options.has_source());
    EXPECT_TRUE(options.has_exported_at());
    EXPECT_FALSE(options.requests_devices());
    EXPECT_FALSE(options.requests_tags());
}

TEST(ConfigurationSnapshotExportMapperTests, MapsPublishedSnapshotMetadata)
{
    const auto snapshot = make_export_mapper_snapshot(
        7,
        dispatcher::domain::ConfigurationStatus::Published
    );

    const auto result = dispatcher::config::ConfigurationSnapshotExportMapper::map(
        snapshot
    );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_model());

    const auto& model = result.model();

    EXPECT_EQ(
        model.metadata().schema_version,
        "dispatcher.config.v1"
    );

    EXPECT_EQ(
        model.metadata().format,
        dispatcher::config::ConfigurationFormat::Json
    );

    EXPECT_EQ(model.metadata().config_version, 7);
    EXPECT_EQ(model.metadata().status, "published");

    EXPECT_TRUE(model.metadata().has_schema_version());
    EXPECT_TRUE(model.metadata().has_known_format());
    EXPECT_TRUE(model.metadata().has_config_version());
    EXPECT_TRUE(model.metadata().has_status());

    EXPECT_TRUE(model.empty());
    EXPECT_EQ(model.device_count(), 0);
    EXPECT_EQ(model.tag_count(), 0);
}

TEST(ConfigurationSnapshotExportMapperTests, MapsDraftSnapshotMetadata)
{
    const auto snapshot = make_export_mapper_snapshot(
        11,
        dispatcher::domain::ConfigurationStatus::Draft
    );

    const auto result = dispatcher::config::ConfigurationSnapshotExportMapper::map(
        snapshot
    );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_model());

    EXPECT_EQ(result.model().metadata().config_version, 11);
    EXPECT_EQ(result.model().metadata().status, "draft");
}

TEST(ConfigurationSnapshotExportMapperTests, AppliesExportOptions)
{
    const auto snapshot = make_export_mapper_snapshot(7);

    const auto result = dispatcher::config::ConfigurationSnapshotExportMapper::map(
        snapshot,
        dispatcher::config::ConfigurationExportOptions{
            .format = dispatcher::config::ConfigurationFormat::Json,
            .source = "unit-test",
            .exported_at = "2026-07-01T00:00:00Z",
            .include_devices = false,
            .include_tags = false
        }
    );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_model());

    EXPECT_EQ(result.model().metadata().source, "unit-test");
    EXPECT_EQ(result.model().metadata().exported_at, "2026-07-01T00:00:00Z");

    EXPECT_TRUE(result.model().metadata().has_source());
    EXPECT_TRUE(result.model().metadata().has_exported_at());

    EXPECT_EQ(result.model().device_count(), 0);
    EXPECT_EQ(result.model().tag_count(), 0);
}

TEST(ConfigurationSnapshotExportMapperTests, RejectsUnknownFormat)
{
    const auto snapshot = make_export_mapper_snapshot(7);

    const auto result = dispatcher::config::ConfigurationSnapshotExportMapper::map(
        snapshot,
        dispatcher::config::ConfigurationExportOptions{
            .format = dispatcher::config::ConfigurationFormat::Unknown
        }
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::UnsupportedFormat
    );

    EXPECT_FALSE(result.has_model());

    EXPECT_EQ(result.error().operation, "configuration.export");
    EXPECT_EQ(result.error().resource, "7");
    EXPECT_EQ(result.error().field, "format");
    EXPECT_EQ(
        result.error().message,
        "unsupported configuration export format"
    );
}

TEST(ConfigurationSnapshotExportMapperTests, MapsDevicesAndTags)
{
    const auto snapshot =
        make_export_mapper_snapshot_with_device_and_tag();

    const auto result =
        dispatcher::config::ConfigurationSnapshotExportMapper::map(snapshot);

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_model());

    const auto& model = result.model();

    EXPECT_EQ(model.metadata().config_version, 7);
    EXPECT_EQ(model.metadata().status, "published");

    ASSERT_EQ(model.device_count(), 1);
    ASSERT_EQ(model.tag_count(), 1);

    const auto* device = model.find_device_by_id("device-1");

    ASSERT_NE(device, nullptr);

    EXPECT_EQ(device->organization_id, "org-1");
    EXPECT_EQ(device->site_id, "site-1");
    EXPECT_EQ(device->area_id, "area-1");
    EXPECT_EQ(device->device_id, "device-1");
    EXPECT_EQ(device->local_name, "device-1");
    EXPECT_EQ(device->display_name, "Device 1");
    EXPECT_TRUE(device->enabled);

    const auto* tag = model.find_tag_by_id("tag-temperature");

    ASSERT_NE(tag, nullptr);

    EXPECT_EQ(tag->organization_id, "org-1");
    EXPECT_EQ(tag->site_id, "site-1");
    EXPECT_EQ(tag->area_id, "area-1");
    EXPECT_EQ(tag->device_id, "device-1");
    EXPECT_EQ(tag->tag_id, "tag-temperature");
    EXPECT_EQ(tag->local_name, "temperature");
    EXPECT_EQ(tag->display_name, "Temperature");
    EXPECT_EQ(tag->data_type, "float64");
    EXPECT_EQ(tag->history_policy, "every_poll");
    EXPECT_TRUE(tag->enabled);
}

TEST(ConfigurationSnapshotExportMapperTests, CanSkipDevicesAndTags)
{
    const auto snapshot =
        make_export_mapper_snapshot_with_device_and_tag();

    const auto result =
        dispatcher::config::ConfigurationSnapshotExportMapper::map(
            snapshot,
            dispatcher::config::ConfigurationExportOptions{
                .include_devices = false,
                .include_tags = false
            }
        );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_model());

    EXPECT_EQ(result.model().device_count(), 0);
    EXPECT_EQ(result.model().tag_count(), 0);
    EXPECT_TRUE(result.model().empty());
}

TEST(ConfigurationSnapshotExportMapperTests, CanSkipOnlyTags)
{
    const auto snapshot =
        make_export_mapper_snapshot_with_device_and_tag();

    const auto result =
        dispatcher::config::ConfigurationSnapshotExportMapper::map(
            snapshot,
            dispatcher::config::ConfigurationExportOptions{
                .include_devices = true,
                .include_tags = false
            }
        );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_model());

    EXPECT_EQ(result.model().device_count(), 1);
    EXPECT_EQ(result.model().tag_count(), 0);
    EXPECT_TRUE(result.model().has_devices());
    EXPECT_FALSE(result.model().has_tags());
}