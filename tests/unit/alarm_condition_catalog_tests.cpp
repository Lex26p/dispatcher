#include <dispatcher/alarm/alarm_condition_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_catalog_validation.hpp>
#include <dispatcher/alarm/alarm_condition_definition.hpp>
#include <dispatcher/alarm/alarm_condition_definition_validation.hpp>
#include <dispatcher/alarm/alarm_condition_type.hpp>
#include <dispatcher/alarm/threshold_alarm_condition.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <gtest/gtest.h>

#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{
    dispatcher::alarm::AlarmConditionDefinition make_condition_definition(
        std::string alarm_id,
        dispatcher::alarm::AlarmConditionType condition_type,
        double threshold
    )
    {
        return dispatcher::alarm::AlarmConditionDefinition(
            dispatcher::domain::AlarmId{ std::move(alarm_id) },
            dispatcher::alarm::ThresholdAlarmCondition(
                condition_type,
                threshold
            )
        );
    }
}

TEST(AlarmConditionDefinitionTests, StoresConditionDefinitionValues)
{
    const auto definition = make_condition_definition(
        "alarm-temperature-high",
        dispatcher::alarm::AlarmConditionType::High,
        80.0
    );

    EXPECT_EQ(
        definition.alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_EQ(
        definition.condition().condition_type(),
        dispatcher::alarm::AlarmConditionType::High
    );

    EXPECT_DOUBLE_EQ(definition.condition().threshold(), 80.0);
}

TEST(AlarmConditionDefinitionValidationTests, ValidDefinitionPassesValidation)
{
    const auto definition = make_condition_definition(
        "alarm-temperature-high",
        dispatcher::alarm::AlarmConditionType::High,
        80.0
    );

    const auto result =
        dispatcher::alarm::validate_alarm_condition_definition(definition);

    EXPECT_TRUE(result.valid());
    EXPECT_FALSE(result.has_errors());
}

TEST(AlarmConditionDefinitionValidationTests, RejectsMissingAlarmId)
{
    const dispatcher::alarm::AlarmConditionDefinition definition(
        dispatcher::domain::AlarmId{},
        dispatcher::alarm::ThresholdAlarmCondition(
            dispatcher::alarm::AlarmConditionType::High,
            80.0
        )
    );

    const auto result =
        dispatcher::alarm::validate_alarm_condition_definition(definition);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "alarm_id");
}

TEST(AlarmConditionDefinitionValidationTests, RejectsInfiniteThreshold)
{
    const auto definition = make_condition_definition(
        "alarm-temperature-high",
        dispatcher::alarm::AlarmConditionType::High,
        std::numeric_limits<double>::infinity()
    );

    const auto result =
        dispatcher::alarm::validate_alarm_condition_definition(definition);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "threshold");
}

TEST(AlarmConditionDefinitionValidationTests, RejectsNaNThreshold)
{
    const auto definition = make_condition_definition(
        "alarm-temperature-high",
        dispatcher::alarm::AlarmConditionType::High,
        std::numeric_limits<double>::quiet_NaN()
    );

    const auto result =
        dispatcher::alarm::validate_alarm_condition_definition(definition);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "threshold");
}

TEST(AlarmConditionCatalogTests, DefaultCatalogStartsEmpty)
{
    const dispatcher::alarm::AlarmConditionCatalog catalog;

    EXPECT_TRUE(catalog.empty());
    EXPECT_EQ(catalog.size(), 0);
    EXPECT_TRUE(catalog.definitions().empty());
}

TEST(AlarmConditionCatalogTests, StoresDefinitions)
{
    const dispatcher::alarm::AlarmConditionCatalog catalog(
        std::vector<dispatcher::alarm::AlarmConditionDefinition>{
        make_condition_definition(
            "alarm-temperature-high",
            dispatcher::alarm::AlarmConditionType::High,
            80.0
        ),
            make_condition_definition(
                "alarm-pressure-low",
                dispatcher::alarm::AlarmConditionType::Low,
                20.0
            )
    }
    );

    EXPECT_FALSE(catalog.empty());
    EXPECT_EQ(catalog.size(), 2);
    EXPECT_EQ(catalog.definitions().size(), 2);
}

TEST(AlarmConditionCatalogTests, FindsByAlarmId)
{
    const dispatcher::alarm::AlarmConditionCatalog catalog(
        std::vector<dispatcher::alarm::AlarmConditionDefinition>{
        make_condition_definition(
            "alarm-temperature-high",
            dispatcher::alarm::AlarmConditionType::High,
            80.0
        ),
            make_condition_definition(
                "alarm-pressure-low",
                dispatcher::alarm::AlarmConditionType::Low,
                20.0
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
        found->condition().condition_type(),
        dispatcher::alarm::AlarmConditionType::Low
    );

    EXPECT_DOUBLE_EQ(found->condition().threshold(), 20.0);
}

TEST(AlarmConditionCatalogTests, FindByAlarmIdReturnsEmptyWhenMissing)
{
    const dispatcher::alarm::AlarmConditionCatalog catalog(
        std::vector<dispatcher::alarm::AlarmConditionDefinition>{
        make_condition_definition(
            "alarm-temperature-high",
            dispatcher::alarm::AlarmConditionType::High,
            80.0
        )
    }
    );

    const auto found = catalog.find_by_alarm_id(
        dispatcher::domain::AlarmId{ "unknown-alarm" }
    );

    EXPECT_FALSE(found.has_value());
}

TEST(AlarmConditionCatalogValidationTests, ValidCatalogPassesValidation)
{
    const dispatcher::alarm::AlarmConditionCatalog catalog(
        std::vector<dispatcher::alarm::AlarmConditionDefinition>{
        make_condition_definition(
            "alarm-temperature-high",
            dispatcher::alarm::AlarmConditionType::High,
            80.0
        ),
            make_condition_definition(
                "alarm-pressure-low",
                dispatcher::alarm::AlarmConditionType::Low,
                20.0
            )
    }
    );

    const auto result = dispatcher::alarm::validate_alarm_condition_catalog(
        catalog
    );

    EXPECT_TRUE(result.valid());
    EXPECT_FALSE(result.has_errors());
}

TEST(AlarmConditionCatalogValidationTests, RejectsInvalidConditionDefinition)
{
    const dispatcher::alarm::AlarmConditionCatalog catalog(
        std::vector<dispatcher::alarm::AlarmConditionDefinition>{
        dispatcher::alarm::AlarmConditionDefinition(
            dispatcher::domain::AlarmId{},
            dispatcher::alarm::ThresholdAlarmCondition(
                dispatcher::alarm::AlarmConditionType::High,
                80.0
            )
        )
    }
    );

    const auto result = dispatcher::alarm::validate_alarm_condition_catalog(
        catalog
    );

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(
        result.errors()[0].field,
        "alarm_condition_definition.alarm_id"
    );
}

TEST(AlarmConditionCatalogValidationTests, RejectsDuplicateAlarmId)
{
    const dispatcher::alarm::AlarmConditionCatalog catalog(
        std::vector<dispatcher::alarm::AlarmConditionDefinition>{
        make_condition_definition(
            "alarm-duplicate",
            dispatcher::alarm::AlarmConditionType::High,
            80.0
        ),
            make_condition_definition(
                "alarm-duplicate",
                dispatcher::alarm::AlarmConditionType::Low,
                20.0
            )
    }
    );

    const auto result = dispatcher::alarm::validate_alarm_condition_catalog(
        catalog
    );

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