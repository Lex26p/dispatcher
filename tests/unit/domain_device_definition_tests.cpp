#include <dispatcher/domain/device_definition_builder.hpp>
#include <dispatcher/domain/device_definition_validation.hpp>

#include <gtest/gtest.h>

TEST(DeviceDefinitionTests, BuilderStoresDeviceMetadata)
{
    using namespace dispatcher::domain;

    const auto device = DeviceDefinitionBuilder{}
        .organization_id(OrganizationId{ "org-1" })
        .site_id(SiteId{ "site-1" })
        .area_id(AreaId{ "area-1" })
        .device_id(DeviceId{ "device-1" })
        .local_name("plc-1")
        .display_name("Main PLC")
        .description("Main production line PLC")
        .enabled(true)
        .config_version(5)
        .build();

    EXPECT_EQ(device.organization_id().value(), "org-1");
    EXPECT_EQ(device.site_id().value(), "site-1");
    EXPECT_EQ(device.area_id().value(), "area-1");
    EXPECT_EQ(device.device_id().value(), "device-1");

    EXPECT_EQ(device.local_name(), "plc-1");
    EXPECT_EQ(device.name(), "plc-1");
    EXPECT_EQ(device.display_name(), "Main PLC");
    EXPECT_EQ(device.description(), "Main production line PLC");
    EXPECT_TRUE(device.enabled());
    EXPECT_EQ(device.config_version(), 5);
}

TEST(DeviceDefinitionTests, OldNameBuilderMethodMapsToLocalName)
{
    using namespace dispatcher::domain;

    const auto device = DeviceDefinitionBuilder{}
        .organization_id(OrganizationId{ "org-1" })
        .site_id(SiteId{ "site-1" })
        .area_id(AreaId{ "area-1" })
        .device_id(DeviceId{ "device-1" })
        .name("plc-1")
        .build();

    EXPECT_EQ(device.local_name(), "plc-1");
    EXPECT_EQ(device.name(), "plc-1");
}

TEST(DeviceDefinitionValidationTests, ValidDevicePassesValidation)
{
    using namespace dispatcher::domain;

    const auto device = DeviceDefinitionBuilder{}
        .organization_id(OrganizationId{ "org-1" })
        .site_id(SiteId{ "site-1" })
        .area_id(AreaId{ "area-1" })
        .device_id(DeviceId{ "device-1" })
        .local_name("plc-1")
        .build();

    const auto result = validate_device_definition(device);

    EXPECT_TRUE(result.valid());
    EXPECT_FALSE(result.has_errors());
}

TEST(DeviceDefinitionValidationTests, EmptyDeviceIdFailsValidation)
{
    using namespace dispatcher::domain;

    const auto device = DeviceDefinitionBuilder{}
        .organization_id(OrganizationId{ "org-1" })
        .site_id(SiteId{ "site-1" })
        .area_id(AreaId{ "area-1" })
        .device_id(DeviceId{ "" })
        .local_name("plc-1")
        .build();

    const auto result = validate_device_definition(device);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());
    EXPECT_EQ(result.errors()[0].field, "device_id");
}

TEST(DeviceDefinitionValidationTests, EmptyLocalNameFailsValidation)
{
    using namespace dispatcher::domain;

    const auto device = DeviceDefinitionBuilder{}
        .organization_id(OrganizationId{ "org-1" })
        .site_id(SiteId{ "site-1" })
        .area_id(AreaId{ "area-1" })
        .device_id(DeviceId{ "device-1" })
        .local_name("")
        .build();

    const auto result = validate_device_definition(device);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());
    EXPECT_EQ(result.errors()[0].field, "local_name");
}

TEST(DeviceDefinitionValidationTests, LocalNameWithSlashFailsValidation)
{
    using namespace dispatcher::domain;

    const auto device = DeviceDefinitionBuilder{}
        .organization_id(OrganizationId{ "org-1" })
        .site_id(SiteId{ "site-1" })
        .area_id(AreaId{ "area-1" })
        .device_id(DeviceId{ "device-1" })
        .local_name("line/plc")
        .build();

    const auto result = validate_device_definition(device);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());
    EXPECT_EQ(result.errors()[0].field, "local_name");
}

TEST(DeviceDefinitionValidationTests, ZeroConfigVersionFailsValidation)
{
    using namespace dispatcher::domain;

    const auto device = DeviceDefinitionBuilder{}
        .organization_id(OrganizationId{ "org-1" })
        .site_id(SiteId{ "site-1" })
        .area_id(AreaId{ "area-1" })
        .device_id(DeviceId{ "device-1" })
        .local_name("plc-1")
        .config_version(0)
        .build();

    const auto result = validate_device_definition(device);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());
    EXPECT_EQ(result.errors()[0].field, "config_version");
}