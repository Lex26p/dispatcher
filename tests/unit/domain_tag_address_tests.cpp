#include <dispatcher/domain/tag_address.hpp>
#include <dispatcher/domain/tag_definition_builder.hpp>

#include <gtest/gtest.h>

TEST(TagAddressTests, StoresAddressParts)
{
    using namespace dispatcher::domain;

    const TagAddress address(
        OrganizationId{ "org-1" },
        SiteId{ "site-1" },
        AreaId{ "area-1" },
        DeviceId{ "device-1" },
        TagId{ "tag-1" }
    );

    EXPECT_EQ(address.organization_id().value(), "org-1");
    EXPECT_EQ(address.site_id().value(), "site-1");
    EXPECT_EQ(address.area_id().value(), "area-1");
    EXPECT_EQ(address.device_id().value(), "device-1");
    EXPECT_EQ(address.tag_id().value(), "tag-1");
}

TEST(TagAddressTests, FullNameReturnsHierarchicalPath)
{
    using namespace dispatcher::domain;

    const TagAddress address(
        OrganizationId{ "org-1" },
        SiteId{ "site-1" },
        AreaId{ "area-1" },
        DeviceId{ "device-1" },
        TagId{ "tag-temperature" }
    );

    EXPECT_EQ(
        address.full_name(),
        "org-1/site-1/area-1/device-1/tag-temperature"
    );
}

TEST(TagAddressTests, EmptyReturnsFalseWhenAllPartsArePresent)
{
    using namespace dispatcher::domain;

    const TagAddress address(
        OrganizationId{ "org-1" },
        SiteId{ "site-1" },
        AreaId{ "area-1" },
        DeviceId{ "device-1" },
        TagId{ "tag-1" }
    );

    EXPECT_FALSE(address.empty());
}

TEST(TagAddressTests, EmptyReturnsTrueWhenAnyPartIsEmpty)
{
    using namespace dispatcher::domain;

    const TagAddress address(
        OrganizationId{ "org-1" },
        SiteId{ "" },
        AreaId{ "area-1" },
        DeviceId{ "device-1" },
        TagId{ "tag-1" }
    );

    EXPECT_TRUE(address.empty());
}

TEST(TagAddressTests, CanBeCreatedFromTagDefinition)
{
    using namespace dispatcher::domain;

    const auto tag = TagDefinitionBuilder{}
        .organization_id(OrganizationId{ "org-1" })
        .site_id(SiteId{ "site-1" })
        .area_id(AreaId{ "area-1" })
        .device_id(DeviceId{ "device-1" })
        .tag_id(TagId{ "tag-pressure" })
        .name("Pressure")
        .data_type(DataType::Float64)
        .build();

    const auto address = make_tag_address(tag);

    EXPECT_EQ(address.full_name(), "org-1/site-1/area-1/device-1/tag-pressure");
}