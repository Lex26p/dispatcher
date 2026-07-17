#include <dispatcher/alarm/alarm_catalog.hpp>
#include <dispatcher/alarm/alarm_catalog_validation.hpp>
#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_severity.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace
{
    dispatcher::alarm::AlarmDefinition make_alarm_definition(
        std::string alarm_id,
        std::string tag_id,
        std::string name
    )
    {
        return dispatcher::alarm::AlarmDefinitionBuilder{}
            .alarm_id(dispatcher::domain::AlarmId{ std::move(alarm_id) })
            .tag_id(dispatcher::domain::TagId{ std::move(tag_id) })
            .name(std::move(name))
            .description("Test alarm")
            .severity(dispatcher::alarm::AlarmSeverity::Warning)
            .enabled(true)
            .config_version(1)
            .build();
    }
}

TEST(AlarmCatalogTests, DefaultCatalogStartsEmpty)
{
    const dispatcher::alarm::AlarmCatalog catalog;

    EXPECT_TRUE(catalog.empty());
    EXPECT_EQ(catalog.size(), 0);
    EXPECT_TRUE(catalog.definitions().empty());
}

TEST(AlarmCatalogTests, StoresDefinitions)
{
    const dispatcher::alarm::AlarmCatalog catalog(
        std::vector<dispatcher::alarm::AlarmDefinition>{
        make_alarm_definition(
            "alarm-temperature-high",
            "tag-temperature",
            "temperature_high"
        ),
            make_alarm_definition(
                "alarm-pressure-low",
                "tag-pressure",
                "pressure_low"
            )
    }
    );

    EXPECT_FALSE(catalog.empty());
    EXPECT_EQ(catalog.size(), 2);
    EXPECT_EQ(catalog.definitions().size(), 2);
}

TEST(AlarmCatalogTests, FindsByAlarmId)
{
    const dispatcher::alarm::AlarmCatalog catalog(
        std::vector<dispatcher::alarm::AlarmDefinition>{
        make_alarm_definition(
            "alarm-temperature-high",
            "tag-temperature",
            "temperature_high"
        ),
            make_alarm_definition(
                "alarm-pressure-low",
                "tag-pressure",
                "pressure_low"
            )
    }
    );

    const auto found = catalog.find_by_alarm_id(
        dispatcher::domain::AlarmId{ "alarm-pressure-low" }
    );

    ASSERT_TRUE(found.has_value());

    EXPECT_EQ(
        found->alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-pressure-low" }
    );

    EXPECT_EQ(
        found->tag_id(),
        dispatcher::domain::TagId{ "tag-pressure" }
    );

    EXPECT_EQ(found->name(), "pressure_low");
}

TEST(AlarmCatalogTests, FindByAlarmIdReturnsEmptyWhenMissing)
{
    const dispatcher::alarm::AlarmCatalog catalog(
        std::vector<dispatcher::alarm::AlarmDefinition>{
        make_alarm_definition(
            "alarm-temperature-high",
            "tag-temperature",
            "temperature_high"
        )
    }
    );

    const auto found = catalog.find_by_alarm_id(
        dispatcher::domain::AlarmId{ "unknown-alarm" }
    );

    EXPECT_FALSE(found.has_value());
}

TEST(AlarmCatalogTests, FindsByTagId)
{
    const dispatcher::alarm::AlarmCatalog catalog(
        std::vector<dispatcher::alarm::AlarmDefinition>{
        make_alarm_definition(
            "alarm-temperature-high",
            "tag-temperature",
            "temperature_high"
        ),
            make_alarm_definition(
                "alarm-temperature-low",
                "tag-temperature",
                "temperature_low"
            ),
            make_alarm_definition(
                "alarm-pressure-low",
                "tag-pressure",
                "pressure_low"
            )
    }
    );

    const auto found = catalog.find_by_tag_id(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_EQ(found.size(), 2);

    EXPECT_EQ(
        found[0].alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_EQ(
        found[1].alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-low" }
    );
}

TEST(AlarmCatalogTests, FindByTagIdReturnsEmptyWhenMissing)
{
    const dispatcher::alarm::AlarmCatalog catalog(
        std::vector<dispatcher::alarm::AlarmDefinition>{
        make_alarm_definition(
            "alarm-temperature-high",
            "tag-temperature",
            "temperature_high"
        )
    }
    );

    const auto found = catalog.find_by_tag_id(
        dispatcher::domain::TagId{ "unknown-tag" }
    );

    EXPECT_TRUE(found.empty());
}

TEST(AlarmCatalogValidationTests, ValidCatalogPassesValidation)
{
    const dispatcher::alarm::AlarmCatalog catalog(
        std::vector<dispatcher::alarm::AlarmDefinition>{
        make_alarm_definition(
            "alarm-temperature-high",
            "tag-temperature",
            "temperature_high"
        ),
            make_alarm_definition(
                "alarm-temperature-low",
                "tag-temperature",
                "temperature_low"
            ),
            make_alarm_definition(
                "alarm-pressure-low",
                "tag-pressure",
                "pressure_low"
            )
    }
    );

    const auto result = dispatcher::alarm::validate_alarm_catalog(catalog);

    EXPECT_TRUE(result.valid());
    EXPECT_FALSE(result.has_errors());
}

TEST(AlarmCatalogValidationTests, RejectsInvalidAlarmDefinition)
{
    const dispatcher::alarm::AlarmCatalog catalog(
        std::vector<dispatcher::alarm::AlarmDefinition>{
        dispatcher::alarm::AlarmDefinitionBuilder{}
            .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
            .name("temperature_high")
            .build()
    }
    );

    const auto result = dispatcher::alarm::validate_alarm_catalog(catalog);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "alarm_definition.alarm_id");
}

TEST(AlarmCatalogValidationTests, RejectsDuplicateAlarmId)
{
    const dispatcher::alarm::AlarmCatalog catalog(
        std::vector<dispatcher::alarm::AlarmDefinition>{
        make_alarm_definition(
            "alarm-duplicate",
            "tag-temperature",
            "temperature_high"
        ),
            make_alarm_definition(
                "alarm-duplicate",
                "tag-pressure",
                "pressure_low"
            )
    }
    );

    const auto result = dispatcher::alarm::validate_alarm_catalog(catalog);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    bool found_duplicate_alarm_id_error = false;

    for (const auto& error : result.errors())
    {
        if (error.field == "alarm_id")
        {
            found_duplicate_alarm_id_error = true;
        }
    }

    EXPECT_TRUE(found_duplicate_alarm_id_error);
}

TEST(AlarmCatalogValidationTests, RejectsDuplicateNameWithinSameTag)
{
    const dispatcher::alarm::AlarmCatalog catalog(
        std::vector<dispatcher::alarm::AlarmDefinition>{
        make_alarm_definition(
            "alarm-temperature-high",
            "tag-temperature",
            "temperature_limit"
        ),
            make_alarm_definition(
                "alarm-temperature-low",
                "tag-temperature",
                "temperature_limit"
            )
    }
    );

    const auto result = dispatcher::alarm::validate_alarm_catalog(catalog);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    bool found_duplicate_tag_name_error = false;

    for (const auto& error : result.errors())
    {
        if (error.field == "tag_id.name")
        {
            found_duplicate_tag_name_error = true;
        }
    }

    EXPECT_TRUE(found_duplicate_tag_name_error);
}

TEST(AlarmCatalogValidationTests, AllowsSameNameForDifferentTags)
{
    const dispatcher::alarm::AlarmCatalog catalog(
        std::vector<dispatcher::alarm::AlarmDefinition>{
        make_alarm_definition(
            "alarm-temperature-high",
            "tag-temperature",
            "high"
        ),
            make_alarm_definition(
                "alarm-pressure-high",
                "tag-pressure",
                "high"
            )
    }
    );

    const auto result = dispatcher::alarm::validate_alarm_catalog(catalog);

    EXPECT_TRUE(result.valid());
    EXPECT_FALSE(result.has_errors());
}