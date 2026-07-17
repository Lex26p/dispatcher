#include <dispatcher/domain/configuration_snapshot.hpp>
#include <dispatcher/domain/configuration_snapshot_validation.hpp>
#include <dispatcher/domain/device_catalog.hpp>
#include <dispatcher/domain/device_definition_builder.hpp>
#include <dispatcher/domain/tag_catalog.hpp>
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

    dispatcher::domain::DeviceCatalog make_device_catalog()
    {
        dispatcher::domain::DeviceCatalog devices;

        const auto result = devices.add(make_device("device-1", "plc-1"));
        EXPECT_TRUE(result.valid());

        return devices;
    }

    dispatcher::domain::TagCatalog make_tag_catalog()
    {
        dispatcher::domain::TagCatalog tags;

        const auto result = tags.add(
            make_tag(
                "device-1",
                "tag-temperature",
                "temperature"
            )
        );

        EXPECT_TRUE(result.valid());

        return tags;
    }
}

TEST(ConfigurationSnapshotTests, StoresCatalogsAndConfigVersion)
{
    const auto snapshot = dispatcher::domain::ConfigurationSnapshot(
        7,
        make_device_catalog(),
        make_tag_catalog()
    );

    EXPECT_EQ(snapshot.config_version(), 7);
    EXPECT_EQ(snapshot.device_count(), 1);
    EXPECT_EQ(snapshot.tag_count(), 1);
    EXPECT_FALSE(snapshot.empty());
}

TEST(ConfigurationSnapshotTests, FindsDeviceById)
{
    using namespace dispatcher::domain;

    const auto snapshot = ConfigurationSnapshot(
        1,
        make_device_catalog(),
        make_tag_catalog()
    );

    const auto found = snapshot.find_device_by_id(DeviceId{ "device-1" });

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->device_id().value(), "device-1");
    EXPECT_EQ(found->local_name(), "plc-1");
}

TEST(ConfigurationSnapshotTests, FindsTagById)
{
    using namespace dispatcher::domain;

    const auto snapshot = ConfigurationSnapshot(
        1,
        make_device_catalog(),
        make_tag_catalog()
    );

    const auto found = snapshot.find_tag_by_id(TagId{ "tag-temperature" });

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->tag_id().value(), "tag-temperature");
    EXPECT_EQ(found->local_name(), "temperature");
}

TEST(ConfigurationSnapshotValidationTests, ValidSnapshotPassesValidation)
{
    const auto snapshot = dispatcher::domain::ConfigurationSnapshot(
        1,
        make_device_catalog(),
        make_tag_catalog()
    );

    const auto result = dispatcher::domain::validate_configuration_snapshot(snapshot);

    EXPECT_TRUE(result.valid());
    EXPECT_FALSE(result.has_errors());
}

TEST(ConfigurationSnapshotValidationTests, ZeroConfigVersionFailsValidation)
{
    const auto snapshot = dispatcher::domain::ConfigurationSnapshot(
        0,
        make_device_catalog(),
        make_tag_catalog()
    );

    const auto result = dispatcher::domain::validate_configuration_snapshot(snapshot);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "config_version");
}

TEST(ConfigurationSnapshotValidationTests, TagReferencingUnknownDeviceFailsValidation)
{
    dispatcher::domain::DeviceCatalog devices;
    dispatcher::domain::TagCatalog tags;

    ASSERT_TRUE(
        devices.add(
            make_device(
                "device-1",
                "plc-1"
            )
        ).valid()
    );

    ASSERT_TRUE(
        tags.add(
            make_tag(
                "unknown-device",
                "tag-temperature",
                "temperature"
            )
        ).valid()
    );

    const auto snapshot = dispatcher::domain::ConfigurationSnapshot(
        1,
        std::move(devices),
        std::move(tags)
    );

    const auto result = dispatcher::domain::validate_configuration_snapshot(snapshot);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "tag.device_id");
}

TEST(ConfigurationSnapshotValidationTests, EmptySnapshotWithPositiveVersionIsValid)
{
    const auto snapshot = dispatcher::domain::ConfigurationSnapshot(
        1,
        dispatcher::domain::DeviceCatalog{},
        dispatcher::domain::TagCatalog{}
    );

    const auto result = dispatcher::domain::validate_configuration_snapshot(snapshot);

    EXPECT_TRUE(result.valid());
    EXPECT_TRUE(snapshot.empty());
}

TEST(ConfigurationSnapshotTests, StoresMetadata)
{
    using namespace dispatcher::domain;

    const auto metadata = ConfigurationMetadata::draft(
        11,
        "Draft snapshot"
    );

    const auto snapshot = ConfigurationSnapshot(
        metadata,
        make_device_catalog(),
        make_tag_catalog()
    );

    EXPECT_EQ(snapshot.config_version(), 11);
    EXPECT_EQ(snapshot.status(), ConfigurationStatus::Draft);
    EXPECT_TRUE(snapshot.is_draft());
    EXPECT_FALSE(snapshot.is_published());
    EXPECT_EQ(snapshot.metadata().description(), "Draft snapshot");
}

TEST(ConfigurationSnapshotTests, OldConstructorCreatesPublishedSnapshot)
{
    const auto snapshot = dispatcher::domain::ConfigurationSnapshot(
        12,
        make_device_catalog(),
        make_tag_catalog()
    );

    EXPECT_EQ(snapshot.config_version(), 12);
    EXPECT_TRUE(snapshot.is_published());
    EXPECT_FALSE(snapshot.is_draft());
}