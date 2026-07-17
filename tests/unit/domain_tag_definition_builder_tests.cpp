#include <dispatcher/domain/tag_definition_builder.hpp>
#include <dispatcher/domain/tag_definition_validation.hpp>

#include <gtest/gtest.h>

TEST(TagDefinitionBuilderTests, BuildsTagDefinitionWithExplicitValues)
{
    using namespace dispatcher::domain;

    const auto tag = TagDefinitionBuilder{}
        .organization_id(OrganizationId{ "org-1" })
        .site_id(SiteId{ "site-1" })
        .area_id(AreaId{ "area-1" })
        .device_id(DeviceId{ "device-1" })
        .tag_id(TagId{ "tag-1" })
        .name("Temperature")
        .description("Main motor temperature")
        .data_type(DataType::Float64)
        .engineering_unit("C")
        .history_policy(HistoryPolicy::OnChangeWithForcedSample)
        .deadband(Deadband{ 0.1 })
        .scaling(Scaling{ 2.0, 5.0 })
        .enabled(true)
        .config_version(7)
        .build();

    EXPECT_EQ(tag.organization_id().value(), "org-1");
    EXPECT_EQ(tag.site_id().value(), "site-1");
    EXPECT_EQ(tag.area_id().value(), "area-1");
    EXPECT_EQ(tag.device_id().value(), "device-1");
    EXPECT_EQ(tag.tag_id().value(), "tag-1");

    EXPECT_EQ(tag.name(), "Temperature");
    EXPECT_EQ(tag.description(), "Main motor temperature");
    EXPECT_EQ(tag.data_type(), DataType::Float64);
    EXPECT_EQ(tag.engineering_unit(), "C");
    EXPECT_EQ(tag.history_policy(), HistoryPolicy::OnChangeWithForcedSample);
    EXPECT_DOUBLE_EQ(tag.deadband().value(), 0.1);
    EXPECT_DOUBLE_EQ(tag.scaling().apply(10.0), 25.0);
    EXPECT_TRUE(tag.enabled());
    EXPECT_EQ(tag.config_version(), 7);
}

TEST(TagDefinitionBuilderTests, BuildsValidTagWithDefaultOptionalValues)
{
    using namespace dispatcher::domain;

    const auto tag = TagDefinitionBuilder{}
        .organization_id(OrganizationId{ "org-1" })
        .site_id(SiteId{ "site-1" })
        .area_id(AreaId{ "area-1" })
        .device_id(DeviceId{ "device-1" })
        .tag_id(TagId{ "tag-1" })
        .name("Pressure")
        .data_type(DataType::Float64)
        .build();

    const auto validation_result = validate_tag_definition(tag);

    EXPECT_TRUE(validation_result.valid());
    EXPECT_EQ(tag.description(), "");
    EXPECT_EQ(tag.engineering_unit(), "");
    EXPECT_EQ(tag.history_policy(), HistoryPolicy::Disabled);
    EXPECT_DOUBLE_EQ(tag.deadband().value(), 0.0);
    EXPECT_DOUBLE_EQ(tag.scaling().apply(10.0), 10.0);
    EXPECT_TRUE(tag.enabled());
    EXPECT_EQ(tag.config_version(), 1);
}

TEST(TagDefinitionBuilderTests, CanBuildInvalidTagForValidationLayer)
{
    using namespace dispatcher::domain;

    const auto tag = TagDefinitionBuilder{}
        .name("")
        .config_version(0)
        .deadband(Deadband{ -1.0 })
        .build();

    const auto validation_result = validate_tag_definition(tag);

    EXPECT_FALSE(validation_result.valid());
    EXPECT_TRUE(validation_result.has_errors());
}