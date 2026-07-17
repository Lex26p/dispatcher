#include <dispatcher/domain/configuration_snapshot_builder.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/device_definition_builder.hpp>
#include <dispatcher/domain/history_policy.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/tag_definition_builder.hpp>

#include <gtest/gtest.h>

#include <stdexcept>

namespace
{
    dispatcher::domain::DeviceDefinition make_read_access_device()
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

    dispatcher::domain::TagDefinition make_read_access_tag()
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

    dispatcher::domain::ConfigurationSnapshot make_read_access_snapshot()
    {
        dispatcher::domain::ConfigurationSnapshotBuilder builder;

        builder.config_version(7);
        builder.status(dispatcher::domain::ConfigurationStatus::Published);

        const auto device_result = builder.add_device(
            make_read_access_device()
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
            make_read_access_tag()
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

TEST(ConfigurationSnapshotReadAccessTests, ExposesDevicesAndTags)
{
    const auto snapshot = make_read_access_snapshot();

    ASSERT_EQ(snapshot.device_count(), 1);
    ASSERT_EQ(snapshot.tag_count(), 1);

    ASSERT_EQ(snapshot.devices().size(), 1);
    ASSERT_EQ(snapshot.tags().size(), 1);

    EXPECT_EQ(
        snapshot.devices().front().device_id(),
        dispatcher::domain::DeviceId{ "device-1" }
    );

    EXPECT_EQ(
        snapshot.tags().front().tag_id(),
        dispatcher::domain::TagId{ "tag-temperature" }
    );
}

TEST(ConfigurationSnapshotReadAccessTests, ReadCollectionsMatchFindById)
{
    const auto snapshot = make_read_access_snapshot();

    const auto device = snapshot.find_device_by_id(
        dispatcher::domain::DeviceId{ "device-1" }
    );

    const auto tag = snapshot.find_tag_by_id(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_TRUE(device.has_value());
    ASSERT_TRUE(tag.has_value());

    ASSERT_FALSE(snapshot.devices().empty());
    ASSERT_FALSE(snapshot.tags().empty());

    EXPECT_EQ(
        snapshot.devices().front().device_id(),
        device->device_id()
    );

    EXPECT_EQ(
        snapshot.tags().front().tag_id(),
        tag->tag_id()
    );
}