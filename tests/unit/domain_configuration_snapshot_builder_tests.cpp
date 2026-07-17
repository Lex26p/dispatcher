#include <dispatcher/domain/configuration_snapshot_builder.hpp>
#include <dispatcher/domain/device_definition_builder.hpp>
#include <dispatcher/domain/tag_definition_builder.hpp>

#include <gtest/gtest.h>

namespace
{
    dispatcher::domain::DeviceDefinition make_device(
        std::string device_id,
        std::string local_name
    )
    {
        using namespace dispatcher::domain;

        return DeviceDefinitionBuilder{}
            .organization_id(OrganizationId{ "org-1" })
            .site_id(SiteId{ "site-1" })
            .area_id(AreaId{ "area-1" })
            .device_id(DeviceId{ std::move(device_id) })
            .local_name(std::move(local_name))
            .display_name("Device")
            .build();
    }

    dispatcher::domain::TagDefinition make_tag(
        std::string device_id,
        std::string tag_id,
        std::string local_name
    )
    {
        using namespace dispatcher::domain;

        return TagDefinitionBuilder{}
            .organization_id(OrganizationId{ "org-1" })
            .site_id(SiteId{ "site-1" })
            .area_id(AreaId{ "area-1" })
            .device_id(DeviceId{ std::move(device_id) })
            .tag_id(TagId{ std::move(tag_id) })
            .local_name(std::move(local_name))
            .display_name("Tag")
            .data_type(DataType::Float64)
            .build();
    }
}

TEST(ConfigurationSnapshotBuilderTests, BuildsSnapshotWithDevicesAndTags)
{
    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    ASSERT_TRUE(builder.add_device(make_device("device-1", "plc-1")).valid());
    ASSERT_TRUE(builder.add_tag(make_tag("device-1", "tag-temperature", "temperature")).valid());

    const auto snapshot = builder
        .config_version(5)
        .build();

    EXPECT_EQ(snapshot.config_version(), 5);
    EXPECT_EQ(snapshot.device_count(), 1);
    EXPECT_EQ(snapshot.tag_count(), 1);
}

TEST(ConfigurationSnapshotBuilderTests, ValidatePassesForConsistentSnapshot)
{
    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    ASSERT_TRUE(builder.add_device(make_device("device-1", "plc-1")).valid());
    ASSERT_TRUE(builder.add_tag(make_tag("device-1", "tag-temperature", "temperature")).valid());

    const auto result = builder.validate();

    EXPECT_TRUE(result.valid());
    EXPECT_FALSE(result.has_errors());
}

TEST(ConfigurationSnapshotBuilderTests, ValidateFailsWhenTagReferencesUnknownDevice)
{
    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    ASSERT_TRUE(builder.add_device(make_device("device-1", "plc-1")).valid());
    ASSERT_TRUE(builder.add_tag(make_tag("unknown-device", "tag-temperature", "temperature")).valid());

    const auto result = builder.validate();

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "tag.device_id");
}

TEST(ConfigurationSnapshotBuilderTests, ValidateFailsWhenConfigVersionIsZero)
{
    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    builder.config_version(0);

    const auto result = builder.validate();

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "config_version");
}

TEST(ConfigurationSnapshotBuilderTests, AddDeviceRejectsDuplicateDeviceId)
{
    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    ASSERT_TRUE(builder.add_device(make_device("device-1", "plc-1")).valid());

    const auto result = builder.add_device(make_device("device-1", "plc-2"));

    EXPECT_FALSE(result.valid());
    EXPECT_TRUE(result.has_errors());
    EXPECT_EQ(builder.device_catalog().size(), 1);
}

TEST(ConfigurationSnapshotBuilderTests, AddTagRejectsDuplicateTagId)
{
    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    ASSERT_TRUE(builder.add_tag(make_tag("device-1", "tag-1", "temperature")).valid());

    const auto result = builder.add_tag(make_tag("device-1", "tag-1", "pressure"));

    EXPECT_FALSE(result.valid());
    EXPECT_TRUE(result.has_errors());
    EXPECT_EQ(builder.tag_catalog().size(), 1);
}

TEST(ConfigurationSnapshotBuilderTests, BuildsDraftSnapshotByDefault)
{
    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    const auto snapshot = builder
        .config_version(3)
        .description("Draft snapshot")
        .build();

    EXPECT_EQ(snapshot.config_version(), 3);
    EXPECT_TRUE(snapshot.is_draft());
    EXPECT_FALSE(snapshot.is_published());
    EXPECT_EQ(snapshot.metadata().description(), "Draft snapshot");
}

TEST(ConfigurationSnapshotBuilderTests, CanBuildPublishedSnapshot)
{
    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    const auto snapshot = builder
        .config_version(4)
        .published()
        .description("Published snapshot")
        .build();

    EXPECT_EQ(snapshot.config_version(), 4);
    EXPECT_FALSE(snapshot.is_draft());
    EXPECT_TRUE(snapshot.is_published());
    EXPECT_EQ(snapshot.metadata().description(), "Published snapshot");
}