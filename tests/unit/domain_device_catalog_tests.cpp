#include <dispatcher/domain/device_catalog.hpp>
#include <dispatcher/domain/device_definition_builder.hpp>

#include <gtest/gtest.h>

#include <string>

namespace
{
    dispatcher::domain::DeviceDefinition make_device(
        std::string device_id,
        std::string local_name,
        std::string area_id = "area-1"
    )
    {
        using namespace dispatcher::domain;

        return DeviceDefinitionBuilder{}
            .organization_id(OrganizationId{ "org-1" })
            .site_id(SiteId{ "site-1" })
            .area_id(AreaId{ std::move(area_id) })
            .device_id(DeviceId{ std::move(device_id) })
            .local_name(std::move(local_name))
            .display_name("Device")
            .build();
    }
}

TEST(DeviceCatalogTests, CatalogIsInitiallyEmpty)
{
    const dispatcher::domain::DeviceCatalog catalog;

    EXPECT_TRUE(catalog.empty());
    EXPECT_EQ(catalog.size(), 0);
}

TEST(DeviceCatalogTests, AddsValidDevice)
{
    dispatcher::domain::DeviceCatalog catalog;

    const auto result = catalog.add(make_device("device-1", "plc-1"));

    EXPECT_TRUE(result.valid());
    EXPECT_FALSE(result.has_errors());
    EXPECT_EQ(catalog.size(), 1);
}

TEST(DeviceCatalogTests, FindsDeviceById)
{
    using namespace dispatcher::domain;

    DeviceCatalog catalog;

    ASSERT_TRUE(catalog.add(make_device("device-1", "plc-1")).valid());

    const auto found = catalog.find_by_id(DeviceId{ "device-1" });

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->device_id().value(), "device-1");
    EXPECT_EQ(found->local_name(), "plc-1");
}

TEST(DeviceCatalogTests, ReturnsEmptyOptionalForUnknownDeviceId)
{
    using namespace dispatcher::domain;

    const DeviceCatalog catalog;

    const auto found = catalog.find_by_id(DeviceId{ "unknown-device" });

    EXPECT_FALSE(found.has_value());
}

TEST(DeviceCatalogTests, RejectsDuplicateDeviceId)
{
    dispatcher::domain::DeviceCatalog catalog;

    ASSERT_TRUE(catalog.add(make_device("device-1", "plc-1")).valid());

    const auto result = catalog.add(make_device("device-1", "plc-2"));

    EXPECT_FALSE(result.valid());
    EXPECT_TRUE(result.has_errors());
    EXPECT_EQ(catalog.size(), 1);
}

TEST(DeviceCatalogTests, RejectsDuplicateLocalNameWithinSameArea)
{
    dispatcher::domain::DeviceCatalog catalog;

    ASSERT_TRUE(catalog.add(make_device("device-1", "plc-1", "area-1")).valid());

    const auto result = catalog.add(make_device("device-2", "plc-1", "area-1"));

    EXPECT_FALSE(result.valid());
    EXPECT_TRUE(result.has_errors());
    EXPECT_EQ(catalog.size(), 1);
}

TEST(DeviceCatalogTests, AllowsSameLocalNameInDifferentAreas)
{
    dispatcher::domain::DeviceCatalog catalog;

    const auto first_result = catalog.add(make_device("device-1", "plc-1", "area-1"));
    const auto second_result = catalog.add(make_device("device-2", "plc-1", "area-2"));

    EXPECT_TRUE(first_result.valid());
    EXPECT_TRUE(second_result.valid());
    EXPECT_EQ(catalog.size(), 2);
}

TEST(DeviceCatalogTests, FindsDeviceByAreaLocalName)
{
    using namespace dispatcher::domain;

    DeviceCatalog catalog;

    ASSERT_TRUE(catalog.add(make_device("device-1", "plc-1", "area-1")).valid());

    const auto found = catalog.find_by_area_local_name(
        OrganizationId{ "org-1" },
        SiteId{ "site-1" },
        AreaId{ "area-1" },
        "plc-1"
    );

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->device_id().value(), "device-1");
    EXPECT_EQ(found->local_name(), "plc-1");
}

TEST(DeviceCatalogTests, RejectsInvalidDeviceDefinition)
{
    using namespace dispatcher::domain;

    DeviceCatalog catalog;

    const auto invalid_device = DeviceDefinitionBuilder{}
        .device_id(DeviceId{ "" })
        .local_name("")
        .build();

    const auto result = catalog.add(invalid_device);

    EXPECT_FALSE(result.valid());
    EXPECT_TRUE(result.has_errors());
    EXPECT_EQ(catalog.size(), 0);
}

TEST(DeviceCatalogTests, ReturnsAllDevices)
{
    dispatcher::domain::DeviceCatalog catalog;

    ASSERT_TRUE(catalog.add(make_device("device-1", "plc-1")).valid());
    ASSERT_TRUE(catalog.add(make_device("device-2", "plc-2")).valid());

    ASSERT_EQ(catalog.devices().size(), 2);
    EXPECT_EQ(catalog.devices()[0].local_name(), "plc-1");
    EXPECT_EQ(catalog.devices()[1].local_name(), "plc-2");
}