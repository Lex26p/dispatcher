#include <dispatcher/alarm/alarm_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_definition.hpp>
#include <dispatcher/alarm/alarm_condition_type.hpp>
#include <dispatcher/alarm/alarm_configuration_metadata.hpp>
#include <dispatcher/alarm/alarm_configuration_snapshot.hpp>
#include <dispatcher/alarm/alarm_configuration_snapshot_builder.hpp>
#include <dispatcher/alarm/alarm_configuration_snapshot_validation.hpp>
#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_severity.hpp>
#include <dispatcher/alarm/threshold_alarm_condition.hpp>
#include <dispatcher/domain/configuration_status.hpp>
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

    dispatcher::alarm::AlarmCatalog make_alarm_catalog()
    {
        return dispatcher::alarm::AlarmCatalog(
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
    }

    dispatcher::alarm::AlarmConditionCatalog make_condition_catalog()
    {
        return dispatcher::alarm::AlarmConditionCatalog(
            std::vector<dispatcher::alarm::AlarmConditionDefinition>{
            make_condition_definition(
                "alarm-temperature-high",
                dispatcher::alarm::AlarmConditionType::High,
                80.0
            ),
                make_condition_definition(
                    "alarm-temperature-low",
                    dispatcher::alarm::AlarmConditionType::Low,
                    20.0
                ),
                make_condition_definition(
                    "alarm-pressure-low",
                    dispatcher::alarm::AlarmConditionType::Low,
                    5.0
                )
        }
        );
    }
}

TEST(AlarmConfigurationSnapshotTests, StoresSnapshotValues)
{
    const dispatcher::alarm::AlarmConfigurationMetadata metadata{
        .name = "plant-alarms",
        .description = "Plant alarm configuration",
        .created_by = "unit-test"
    };

    const dispatcher::alarm::AlarmConfigurationSnapshot snapshot(
        7,
        metadata,
        dispatcher::domain::ConfigurationStatus::Published,
        make_alarm_catalog(),
        make_condition_catalog()
    );

    EXPECT_EQ(snapshot.config_version(), 7);
    EXPECT_EQ(snapshot.metadata().name, "plant-alarms");
    EXPECT_EQ(snapshot.metadata().description, "Plant alarm configuration");
    EXPECT_EQ(snapshot.metadata().created_by, "unit-test");

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::domain::ConfigurationStatus::Published
    );

    EXPECT_FALSE(snapshot.draft());
    EXPECT_TRUE(snapshot.published());

    EXPECT_EQ(snapshot.alarm_catalog().size(), 3);
    EXPECT_EQ(snapshot.catalog().size(), 3);
    EXPECT_EQ(snapshot.condition_catalog().size(), 3);
}

TEST(AlarmConfigurationSnapshotTests, FindsByAlarmId)
{
    const auto snapshot = dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
        .config_version(3)
        .status(dispatcher::domain::ConfigurationStatus::Published)
        .alarm_catalog(make_alarm_catalog())
        .condition_catalog(make_condition_catalog())
        .build();

    const auto found = snapshot.find_by_alarm_id(
        dispatcher::domain::AlarmId{ "alarm-temperature-low" }
    );

    ASSERT_TRUE(found.has_value());

    EXPECT_EQ(
        found->alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-low" }
    );

    EXPECT_EQ(
        found->tag_id(),
        dispatcher::domain::TagId{ "tag-temperature" }
    );
}

TEST(AlarmConfigurationSnapshotTests, FindsByTagId)
{
    const auto snapshot = dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
        .config_version(3)
        .status(dispatcher::domain::ConfigurationStatus::Published)
        .alarm_catalog(make_alarm_catalog())
        .condition_catalog(make_condition_catalog())
        .build();

    const auto found = snapshot.find_by_tag_id(
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

TEST(AlarmConfigurationSnapshotBuilderTests, BuildsSnapshot)
{
    const dispatcher::alarm::AlarmConfigurationMetadata metadata{
        .name = "plant-alarms",
        .description = "Plant alarm configuration",
        .created_by = "unit-test"
    };

    const auto snapshot = dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
        .config_version(11)
        .metadata(metadata)
        .status(dispatcher::domain::ConfigurationStatus::Published)
        .alarm_catalog(make_alarm_catalog())
        .condition_catalog(make_condition_catalog())
        .build();

    EXPECT_EQ(snapshot.config_version(), 11);
    EXPECT_EQ(snapshot.metadata().name, "plant-alarms");
    EXPECT_TRUE(snapshot.published());
    EXPECT_EQ(snapshot.alarm_catalog().size(), 3);
    EXPECT_EQ(snapshot.condition_catalog().size(), 3);
}

TEST(AlarmConfigurationSnapshotBuilderTests, CatalogAliasStillBuildsAlarmCatalog)
{
    const auto snapshot = dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
        .catalog(make_alarm_catalog())
        .condition_catalog(make_condition_catalog())
        .build();

    EXPECT_EQ(snapshot.alarm_catalog().size(), 3);
    EXPECT_EQ(snapshot.catalog().size(), 3);
}

TEST(AlarmConfigurationSnapshotBuilderTests, UsesDefaults)
{
    const auto snapshot = dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
    .build();

    EXPECT_EQ(snapshot.config_version(), 1);
    EXPECT_EQ(snapshot.metadata().name, "alarm-configuration");

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::domain::ConfigurationStatus::Draft
    );

    EXPECT_TRUE(snapshot.draft());
    EXPECT_FALSE(snapshot.published());
    EXPECT_TRUE(snapshot.alarm_catalog().empty());
    EXPECT_TRUE(snapshot.condition_catalog().empty());
}

TEST(AlarmConfigurationSnapshotValidationTests, ValidSnapshotPassesValidation)
{
    const auto snapshot = dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
        .config_version(1)
        .metadata(
            dispatcher::alarm::AlarmConfigurationMetadata{
                .name = "plant-alarms",
                .description = "Plant alarm configuration",
                .created_by = "unit-test"
            }
        )
        .status(dispatcher::domain::ConfigurationStatus::Published)
        .alarm_catalog(make_alarm_catalog())
        .condition_catalog(make_condition_catalog())
        .build();

    const auto result =
        dispatcher::alarm::validate_alarm_configuration_snapshot(snapshot);

    EXPECT_TRUE(result.valid());
    EXPECT_FALSE(result.has_errors());
}

TEST(AlarmConfigurationSnapshotValidationTests, RejectsZeroConfigVersion)
{
    const auto snapshot = dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
        .config_version(0)
        .alarm_catalog(make_alarm_catalog())
        .condition_catalog(make_condition_catalog())
        .build();

    const auto result =
        dispatcher::alarm::validate_alarm_configuration_snapshot(snapshot);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "config_version");
}

TEST(AlarmConfigurationSnapshotValidationTests, RejectsEmptyMetadataName)
{
    const auto snapshot = dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
        .metadata(
            dispatcher::alarm::AlarmConfigurationMetadata{
                .name = "",
                .description = "Plant alarm configuration",
                .created_by = "unit-test"
            }
        )
        .alarm_catalog(make_alarm_catalog())
        .condition_catalog(make_condition_catalog())
        .build();

    const auto result =
        dispatcher::alarm::validate_alarm_configuration_snapshot(snapshot);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "metadata.name");
}

TEST(AlarmConfigurationSnapshotValidationTests, RejectsMetadataNameWithSlash)
{
    const auto snapshot = dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
        .metadata(
            dispatcher::alarm::AlarmConfigurationMetadata{
                .name = "plant/alarms",
                .description = "Plant alarm configuration",
                .created_by = "unit-test"
            }
        )
        .alarm_catalog(make_alarm_catalog())
        .condition_catalog(make_condition_catalog())
        .build();

    const auto result =
        dispatcher::alarm::validate_alarm_configuration_snapshot(snapshot);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "metadata.name");
}

TEST(AlarmConfigurationSnapshotValidationTests, RejectsInvalidAlarmCatalog)
{
    const dispatcher::alarm::AlarmCatalog invalid_alarm_catalog(
        std::vector<dispatcher::alarm::AlarmDefinition>{
        dispatcher::alarm::AlarmDefinitionBuilder{}
            .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
            .name("temperature_high")
            .build()
    }
    );

    const auto snapshot = dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
        .alarm_catalog(invalid_alarm_catalog)
        .build();

    const auto result =
        dispatcher::alarm::validate_alarm_configuration_snapshot(snapshot);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "alarm_catalog.alarm_definition.alarm_id");
}

TEST(AlarmConfigurationSnapshotValidationTests, RejectsInvalidConditionCatalog)
{
    const dispatcher::alarm::AlarmConditionCatalog invalid_condition_catalog(
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

    const auto snapshot = dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
        .condition_catalog(invalid_condition_catalog)
        .build();

    const auto result =
        dispatcher::alarm::validate_alarm_configuration_snapshot(snapshot);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(
        result.errors()[0].field,
        "condition_catalog.alarm_condition_definition.alarm_id"
    );
}

TEST(AlarmConfigurationSnapshotValidationTests, RejectsAlarmDefinitionWithoutCondition)
{
    const dispatcher::alarm::AlarmConditionCatalog missing_one_condition(
        std::vector<dispatcher::alarm::AlarmConditionDefinition>{
        make_condition_definition(
            "alarm-temperature-high",
            dispatcher::alarm::AlarmConditionType::High,
            80.0
        ),
            make_condition_definition(
                "alarm-temperature-low",
                dispatcher::alarm::AlarmConditionType::Low,
                20.0
            )
    }
    );

    const auto snapshot = dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
        .alarm_catalog(make_alarm_catalog())
        .condition_catalog(missing_one_condition)
        .build();

    const auto result =
        dispatcher::alarm::validate_alarm_configuration_snapshot(snapshot);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    bool found_missing_condition_error = false;

    for (const auto& error : result.errors())
    {
        if (error.field == "condition_catalog"
            && error.message == "alarm definition must have condition")
        {
            found_missing_condition_error = true;
        }
    }

    EXPECT_TRUE(found_missing_condition_error);
}

TEST(AlarmConfigurationSnapshotValidationTests, RejectsConditionReferencingMissingAlarmDefinition)
{
    const dispatcher::alarm::AlarmConditionCatalog condition_catalog(
        std::vector<dispatcher::alarm::AlarmConditionDefinition>{
        make_condition_definition(
            "alarm-temperature-high",
            dispatcher::alarm::AlarmConditionType::High,
            80.0
        ),
            make_condition_definition(
                "unknown-alarm",
                dispatcher::alarm::AlarmConditionType::Low,
                20.0
            ),
            make_condition_definition(
                "alarm-pressure-low",
                dispatcher::alarm::AlarmConditionType::Low,
                5.0
            )
    }
    );

    const auto snapshot = dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
        .alarm_catalog(make_alarm_catalog())
        .condition_catalog(condition_catalog)
        .build();

    const auto result =
        dispatcher::alarm::validate_alarm_configuration_snapshot(snapshot);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    bool found_missing_alarm_error = false;

    for (const auto& error : result.errors())
    {
        if (error.field == "condition_catalog.alarm_id"
            && error.message == "condition must reference existing alarm definition")
        {
            found_missing_alarm_error = true;
        }
    }

    EXPECT_TRUE(found_missing_alarm_error);
}