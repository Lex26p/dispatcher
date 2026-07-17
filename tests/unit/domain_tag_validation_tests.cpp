#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/deadband.hpp>
#include <dispatcher/domain/history_policy.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/scaling.hpp>
#include <dispatcher/domain/tag_definition.hpp>
#include <dispatcher/domain/tag_definition_validation.hpp>

#include <gtest/gtest.h>

#include <string>

namespace
{
    dispatcher::domain::TagDefinition make_valid_tag_definition()
    {
        using namespace dispatcher::domain;

        return TagDefinition(
            OrganizationId{ "org-1" },
            SiteId{ "site-1" },
            AreaId{ "area-1" },
            DeviceId{ "device-1" },
            TagId{ "tag-1" },
            "temperature",
            "Main motor temperature",
            DataType::Float64,
            "C",
            HistoryPolicy::OnChangeWithForcedSample,
            Deadband{ 0.1 },
            Scaling{ 1.0, 0.0 },
            true,
            1,
            "Motor Temperature"
        );
    }
}

TEST(TagDefinitionValidationTests, ValidTagDefinitionPassesValidation)
{
    const auto tag_definition = make_valid_tag_definition();

    const auto result = dispatcher::domain::validate_tag_definition(tag_definition);

    EXPECT_TRUE(result.valid());
    EXPECT_FALSE(result.has_errors());
    EXPECT_TRUE(result.errors().empty());
}

TEST(TagDefinitionValidationTests, EmptyTagIdFailsValidation)
{
    using namespace dispatcher::domain;

    const TagDefinition tag_definition(
        OrganizationId{ "org-1" },
        SiteId{ "site-1" },
        AreaId{ "area-1" },
        DeviceId{ "device-1" },
        TagId{ "" },
        "temperature",
        "Main motor temperature",
        DataType::Float64,
        "C",
        HistoryPolicy::OnChangeWithForcedSample,
        Deadband{ 0.1 },
        Scaling{ 1.0, 0.0 },
        true,
        1,
        "Motor Temperature"
    );

    const auto result = validate_tag_definition(tag_definition);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());
    ASSERT_EQ(result.errors().size(), 1);

    EXPECT_EQ(result.errors()[0].field, "tag_id");
    EXPECT_EQ(result.errors()[0].message, "tag_id must not be empty");
}

TEST(TagDefinitionValidationTests, EmptyLocalNameFailsValidation)
{
    using namespace dispatcher::domain;

    const TagDefinition tag_definition(
        OrganizationId{ "org-1" },
        SiteId{ "site-1" },
        AreaId{ "area-1" },
        DeviceId{ "device-1" },
        TagId{ "tag-1" },
        "",
        "Main motor temperature",
        DataType::Float64,
        "C",
        HistoryPolicy::OnChangeWithForcedSample,
        Deadband{ 0.1 },
        Scaling{ 1.0, 0.0 },
        true,
        1,
        "Motor Temperature"
    );

    const auto result = validate_tag_definition(tag_definition);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());
    ASSERT_EQ(result.errors().size(), 1);

    EXPECT_EQ(result.errors()[0].field, "local_name");
    EXPECT_EQ(result.errors()[0].message, "local_name must not be empty");
}

TEST(TagDefinitionValidationTests, LocalNameWithSlashFailsValidation)
{
    using namespace dispatcher::domain;

    const TagDefinition tag_definition(
        OrganizationId{ "org-1" },
        SiteId{ "site-1" },
        AreaId{ "area-1" },
        DeviceId{ "device-1" },
        TagId{ "tag-1" },
        "motor/temperature",
        "Main motor temperature",
        DataType::Float64,
        "C",
        HistoryPolicy::OnChangeWithForcedSample,
        Deadband{ 0.1 },
        Scaling{ 1.0, 0.0 },
        true,
        1,
        "Motor Temperature"
    );

    const auto result = validate_tag_definition(tag_definition);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());
    ASSERT_EQ(result.errors().size(), 1);

    EXPECT_EQ(result.errors()[0].field, "local_name");
}

TEST(TagDefinitionValidationTests, NegativeDeadbandFailsValidation)
{
    using namespace dispatcher::domain;

    const TagDefinition tag_definition(
        OrganizationId{ "org-1" },
        SiteId{ "site-1" },
        AreaId{ "area-1" },
        DeviceId{ "device-1" },
        TagId{ "tag-1" },
        "temperature",
        "Main motor temperature",
        DataType::Float64,
        "C",
        HistoryPolicy::OnChangeWithForcedSample,
        Deadband{ -0.1 },
        Scaling{ 1.0, 0.0 },
        true,
        1,
        "Motor Temperature"
    );

    const auto result = validate_tag_definition(tag_definition);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());
    ASSERT_EQ(result.errors().size(), 1);

    EXPECT_EQ(result.errors()[0].field, "deadband");
}

TEST(TagDefinitionValidationTests, ZeroConfigVersionFailsValidation)
{
    using namespace dispatcher::domain;

    const TagDefinition tag_definition(
        OrganizationId{ "org-1" },
        SiteId{ "site-1" },
        AreaId{ "area-1" },
        DeviceId{ "device-1" },
        TagId{ "tag-1" },
        "temperature",
        "Main motor temperature",
        DataType::Float64,
        "C",
        HistoryPolicy::OnChangeWithForcedSample,
        Deadband{ 0.1 },
        Scaling{ 1.0, 0.0 },
        true,
        0,
        "Motor Temperature"
    );

    const auto result = validate_tag_definition(tag_definition);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());
    ASSERT_EQ(result.errors().size(), 1);

    EXPECT_EQ(result.errors()[0].field, "config_version");
}

TEST(TagDefinitionValidationTests, MultipleEmptyRequiredFieldsReturnMultipleErrors)
{
    using namespace dispatcher::domain;

    const TagDefinition tag_definition(
        OrganizationId{ "" },
        SiteId{ "" },
        AreaId{ "" },
        DeviceId{ "" },
        TagId{ "" },
        "",
        "",
        DataType::Float64,
        "C",
        HistoryPolicy::OnChangeWithForcedSample,
        Deadband{ -1.0 },
        Scaling{ 1.0, 0.0 },
        true,
        0,
        ""
    );

    const auto result = validate_tag_definition(tag_definition);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());
    EXPECT_EQ(result.errors().size(), 8);
}