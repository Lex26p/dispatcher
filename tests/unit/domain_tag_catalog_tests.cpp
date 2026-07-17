#include <dispatcher/domain/tag_catalog.hpp>
#include <dispatcher/domain/tag_definition_builder.hpp>

#include <gtest/gtest.h>

namespace
{
    dispatcher::domain::TagDefinition make_tag(
        std::string tag_id,
        std::string tag_name
    )
    {
        using namespace dispatcher::domain;

        return TagDefinitionBuilder{}
            .organization_id(OrganizationId{ "org-1" })
            .site_id(SiteId{ "site-1" })
            .area_id(AreaId{ "area-1" })
            .device_id(DeviceId{ "device-1" })
            .tag_id(TagId{ std::move(tag_id) })
            .name(std::move(tag_name))
            .data_type(DataType::Float64)
            .build();
    }

    dispatcher::domain::TagDefinition make_tag_with_address(
        std::string organization_id,
        std::string site_id,
        std::string area_id,
        std::string device_id,
        std::string tag_id,
        std::string tag_name
    )
    {
        using namespace dispatcher::domain;

        return TagDefinitionBuilder{}
            .organization_id(OrganizationId{ std::move(organization_id) })
            .site_id(SiteId{ std::move(site_id) })
            .area_id(AreaId{ std::move(area_id) })
            .device_id(DeviceId{ std::move(device_id) })
            .tag_id(TagId{ std::move(tag_id) })
            .name(std::move(tag_name))
            .data_type(DataType::Float64)
            .build();
    }
}

TEST(TagCatalogTests, CatalogIsInitiallyEmpty)
{
    const dispatcher::domain::TagCatalog catalog;

    EXPECT_TRUE(catalog.empty());
    EXPECT_EQ(catalog.size(), 0);
}

TEST(TagCatalogTests, AddsValidTag)
{
    dispatcher::domain::TagCatalog catalog;

    const auto result = catalog.add(make_tag("tag-1", "Temperature"));

    EXPECT_TRUE(result.valid());
    EXPECT_FALSE(result.has_errors());
    EXPECT_FALSE(catalog.empty());
    EXPECT_EQ(catalog.size(), 1);
}

TEST(TagCatalogTests, FindsTagById)
{
    using namespace dispatcher::domain;

    TagCatalog catalog;

    ASSERT_TRUE(catalog.add(make_tag("tag-1", "Temperature")).valid());

    const auto found = catalog.find_by_id(TagId{ "tag-1" });

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->tag_id().value(), "tag-1");
    EXPECT_EQ(found->name(), "Temperature");
}

TEST(TagCatalogTests, ReturnsEmptyOptionalForUnknownTagId)
{
    using namespace dispatcher::domain;

    const TagCatalog catalog;

    const auto found = catalog.find_by_id(TagId{ "unknown" });

    EXPECT_FALSE(found.has_value());
}

TEST(TagCatalogTests, FindsTagByAddress)
{
    using namespace dispatcher::domain;

    TagCatalog catalog;

    ASSERT_TRUE(
        catalog.add(
            make_tag_with_address(
                "org-1",
                "site-1",
                "area-1",
                "device-1",
                "tag-pressure",
                "Pressure"
            )
        ).valid()
    );

    const TagAddress address(
        OrganizationId{ "org-1" },
        SiteId{ "site-1" },
        AreaId{ "area-1" },
        DeviceId{ "device-1" },
        TagId{ "tag-pressure" }
    );

    const auto found = catalog.find_by_address(address);

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name(), "Pressure");
}

TEST(TagCatalogTests, RejectsDuplicateTagId)
{
    dispatcher::domain::TagCatalog catalog;

    ASSERT_TRUE(catalog.add(make_tag("tag-1", "Temperature")).valid());

    const auto result = catalog.add(make_tag("tag-1", "Pressure"));

    EXPECT_FALSE(result.valid());
    EXPECT_TRUE(result.has_errors());
    EXPECT_EQ(catalog.size(), 1);
}

TEST(TagCatalogTests, RejectsDuplicateAddress)
{
    using namespace dispatcher::domain;

    TagCatalog catalog;

    ASSERT_TRUE(
        catalog.add(
            make_tag_with_address(
                "org-1",
                "site-1",
                "area-1",
                "device-1",
                "tag-1",
                "Temperature"
            )
        ).valid()
    );

    const auto result = catalog.add(
        make_tag_with_address(
            "org-1",
            "site-1",
            "area-1",
            "device-1",
            "tag-1",
            "Pressure"
        )
    );

    EXPECT_FALSE(result.valid());
    EXPECT_TRUE(result.has_errors());
    EXPECT_EQ(catalog.size(), 1);
}

TEST(TagCatalogTests, RejectsInvalidTagDefinition)
{
    using namespace dispatcher::domain;

    TagCatalog catalog;

    const auto invalid_tag = TagDefinitionBuilder{}
        .tag_id(TagId{ "" })
        .name("")
        .data_type(DataType::Float64)
        .build();

    const auto result = catalog.add(invalid_tag);

    EXPECT_FALSE(result.valid());
    EXPECT_TRUE(result.has_errors());
    EXPECT_EQ(catalog.size(), 0);
}

TEST(TagCatalogTests, ReturnsAllTags)
{
    dispatcher::domain::TagCatalog catalog;

    ASSERT_TRUE(catalog.add(make_tag("tag-1", "Temperature")).valid());
    ASSERT_TRUE(catalog.add(make_tag("tag-2", "Pressure")).valid());

    ASSERT_EQ(catalog.tags().size(), 2);
    EXPECT_EQ(catalog.tags()[0].name(), "Temperature");
    EXPECT_EQ(catalog.tags()[1].name(), "Pressure");
}