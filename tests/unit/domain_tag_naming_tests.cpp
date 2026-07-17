#include <dispatcher/domain/tag_catalog.hpp>
#include <dispatcher/domain/tag_definition_builder.hpp>
#include <dispatcher/domain/tag_definition_validation.hpp>

#include <gtest/gtest.h>

#include <string>

namespace
{
    dispatcher::domain::TagDefinition make_named_tag(
        std::string device_id,
        std::string tag_id,
        std::string local_name,
        std::string display_name = {}
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
            .display_name(std::move(display_name))
            .data_type(DataType::Float64)
            .build();
    }
}

TEST(TagNamingTests, BuilderStoresLocalNameAndDisplayName)
{
    using namespace dispatcher::domain;

    const auto tag = TagDefinitionBuilder{}
        .organization_id(OrganizationId{ "org-1" })
        .site_id(SiteId{ "site-1" })
        .area_id(AreaId{ "area-1" })
        .device_id(DeviceId{ "device-1" })
        .tag_id(TagId{ "tag-temperature" })
        .local_name("temperature")
        .display_name("Motor Temperature")
        .description("Main motor temperature")
        .data_type(DataType::Float64)
        .build();

    EXPECT_EQ(tag.local_name(), "temperature");
    EXPECT_EQ(tag.name(), "temperature");
    EXPECT_EQ(tag.display_name(), "Motor Temperature");
    EXPECT_EQ(tag.description(), "Main motor temperature");
}

TEST(TagNamingTests, OldNameBuilderMethodMapsToLocalName)
{
    using namespace dispatcher::domain;

    const auto tag = TagDefinitionBuilder{}
        .organization_id(OrganizationId{ "org-1" })
        .site_id(SiteId{ "site-1" })
        .area_id(AreaId{ "area-1" })
        .device_id(DeviceId{ "device-1" })
        .tag_id(TagId{ "tag-pressure" })
        .name("pressure")
        .data_type(DataType::Float64)
        .build();

    EXPECT_EQ(tag.local_name(), "pressure");
    EXPECT_EQ(tag.name(), "pressure");
}

TEST(TagNamingTests, DisplayNameCanBeEmpty)
{
    const auto tag = make_named_tag(
        "device-1",
        "tag-temperature",
        "temperature"
    );

    const auto result = dispatcher::domain::validate_tag_definition(tag);

    EXPECT_TRUE(result.valid());
    EXPECT_EQ(tag.display_name(), "");
}

TEST(TagNamingTests, CatalogRejectsDuplicateLocalNameWithinSameDevice)
{
    dispatcher::domain::TagCatalog catalog;

    ASSERT_TRUE(
        catalog.add(
            make_named_tag(
                "device-1",
                "tag-temperature-1",
                "temperature",
                "Motor Temperature 1"
            )
        ).valid()
    );

    const auto result = catalog.add(
        make_named_tag(
            "device-1",
            "tag-temperature-2",
            "temperature",
            "Motor Temperature 2"
        )
    );

    EXPECT_FALSE(result.valid());
    EXPECT_TRUE(result.has_errors());
    EXPECT_EQ(catalog.size(), 1);
}

TEST(TagNamingTests, CatalogAllowsSameLocalNameOnDifferentDevices)
{
    dispatcher::domain::TagCatalog catalog;

    const auto first_result = catalog.add(
        make_named_tag(
            "device-1",
            "tag-device-1-temperature",
            "temperature"
        )
    );

    const auto second_result = catalog.add(
        make_named_tag(
            "device-2",
            "tag-device-2-temperature",
            "temperature"
        )
    );

    EXPECT_TRUE(first_result.valid());
    EXPECT_TRUE(second_result.valid());
    EXPECT_EQ(catalog.size(), 2);
}

TEST(TagNamingTests, CatalogFindsTagByDeviceLocalName)
{
    using namespace dispatcher::domain;

    TagCatalog catalog;

    ASSERT_TRUE(
        catalog.add(
            make_named_tag(
                "device-1",
                "tag-temperature",
                "temperature",
                "Motor Temperature"
            )
        ).valid()
    );

    const auto found = catalog.find_by_device_local_name(
        OrganizationId{ "org-1" },
        SiteId{ "site-1" },
        AreaId{ "area-1" },
        DeviceId{ "device-1" },
        "temperature"
    );

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->tag_id().value(), "tag-temperature");
    EXPECT_EQ(found->local_name(), "temperature");
    EXPECT_EQ(found->display_name(), "Motor Temperature");
}