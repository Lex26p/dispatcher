#include <dispatcher/alarm/alarm_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_definition.hpp>
#include <dispatcher/alarm/alarm_condition_type.hpp>
#include <dispatcher/alarm/alarm_configuration_metadata.hpp>
#include <dispatcher/alarm/alarm_configuration_snapshot_builder.hpp>
#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_runtime.hpp>
#include <dispatcher/alarm/alarm_severity.hpp>
#include <dispatcher/alarm/threshold_alarm_condition.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <gtest/gtest.h>

#include <cstdint>
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
            .description("Indexed alarm test")
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

    dispatcher::alarm::AlarmConfigurationSnapshot make_indexed_snapshot(
        std::uint64_t config_version
    )
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(config_version)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "indexed-alarms",
                    .description = "Indexed alarm configuration",
                    .created_by = "unit-test"
                }
            )
            .status(dispatcher::domain::ConfigurationStatus::Published)
            .alarm_catalog(
                dispatcher::alarm::AlarmCatalog(
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
                    "alarm-pressure-high",
                    "tag-pressure",
                    "pressure_high"
                )
        }
                )
            )
            .condition_catalog(
                dispatcher::alarm::AlarmConditionCatalog(
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
                    "alarm-pressure-high",
                    dispatcher::alarm::AlarmConditionType::High,
                    10.0
                )
        }
                )
            )
            .build();
    }

    dispatcher::alarm::AlarmConfigurationSnapshot make_replacement_snapshot()
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(9)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "replacement-alarms",
                    .description = "Replacement alarm configuration",
                    .created_by = "unit-test"
                }
            )
            .status(dispatcher::domain::ConfigurationStatus::Published)
            .alarm_catalog(
                dispatcher::alarm::AlarmCatalog(
                    std::vector<dispatcher::alarm::AlarmDefinition>{
            make_alarm_definition(
                "alarm-flow-high",
                "tag-flow",
                "flow_high"
            )
        }
                )
            )
            .condition_catalog(
                dispatcher::alarm::AlarmConditionCatalog(
                    std::vector<dispatcher::alarm::AlarmConditionDefinition>{
            make_condition_definition(
                "alarm-flow-high",
                dispatcher::alarm::AlarmConditionType::High,
                100.0
            )
        }
                )
            )
            .build();
    }

    dispatcher::alarm::AlarmConfigurationSnapshot make_draft_snapshot()
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(10)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "draft-alarms",
                    .description = "Draft alarm configuration",
                    .created_by = "unit-test"
                }
            )
            .status(dispatcher::domain::ConfigurationStatus::Draft)
            .alarm_catalog(
                dispatcher::alarm::AlarmCatalog(
                    std::vector<dispatcher::alarm::AlarmDefinition>{
            make_alarm_definition(
                "alarm-flow-high",
                "tag-flow",
                "flow_high"
            )
        }
                )
            )
            .condition_catalog(
                dispatcher::alarm::AlarmConditionCatalog(
                    std::vector<dispatcher::alarm::AlarmConditionDefinition>{
            make_condition_definition(
                "alarm-flow-high",
                dispatcher::alarm::AlarmConditionType::High,
                100.0
            )
        }
                )
            )
            .build();
    }

    dispatcher::telemetry::TelemetryValue make_telemetry_value(
        std::string tag_id,
        dispatcher::telemetry::TagValue value,
        std::uint64_t sequence
    )
    {
        using dispatcher::domain::Quality;
        using dispatcher::domain::TagId;
        using dispatcher::telemetry::TelemetryValue;

        const auto now = TelemetryValue::Clock::now();

        return TelemetryValue(
            TagId{ std::move(tag_id) },
            std::move(value),
            Quality::Good,
            now,
            now,
            sequence
        );
    }
}

TEST(AlarmRuntimeIndexTests, DefaultRuntimeHasEmptyConfigurationIndexes)
{
    const dispatcher::alarm::AlarmRuntime runtime;

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.configuration_version, 1);
    EXPECT_EQ(snapshot.configured_alarm_count, 0);
    EXPECT_EQ(snapshot.indexed_tag_count, 0);
    EXPECT_EQ(snapshot.indexed_condition_count, 0);
}

TEST(AlarmRuntimeIndexTests, ReloadBuildsConfigurationIndexes)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto reload_result = runtime.reload_configuration(
        make_indexed_snapshot(7)
    );

    ASSERT_TRUE(reload_result.valid());

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.configuration_version, 7);
    EXPECT_EQ(snapshot.configured_alarm_count, 3);
    EXPECT_EQ(snapshot.indexed_tag_count, 2);
    EXPECT_EQ(snapshot.indexed_condition_count, 3);
}

TEST(AlarmRuntimeIndexTests, ConfiguredEvaluationUsesIndexedAlarms)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(
        runtime.reload_configuration(
            make_indexed_snapshot(7)
        ).valid()
    );

    const auto result = runtime.evaluate_configured(
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(81.0),
            1
        )
    );

    EXPECT_EQ(result.configured_alarm_count(), 2);
    EXPECT_EQ(result.missing_condition_count(), 0);

    ASSERT_EQ(result.batch_result().results().size(), 2);

    EXPECT_TRUE(result.batch_result().results()[0].activated());
    EXPECT_FALSE(result.batch_result().results()[1].transitioned());

    EXPECT_EQ(result.batch_result().activated_count(), 1);
    EXPECT_EQ(result.batch_result().no_transition_count(), 1);

    EXPECT_EQ(runtime.state_store().size(), 2);
    EXPECT_EQ(runtime.event_store().size(), 1);
}

TEST(AlarmRuntimeIndexTests, SuccessfulReloadReplacesIndexes)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(
        runtime.reload_configuration(
            make_indexed_snapshot(7)
        ).valid()
    );

    ASSERT_EQ(runtime.runtime_snapshot().configured_alarm_count, 3);
    ASSERT_EQ(runtime.runtime_snapshot().indexed_tag_count, 2);

    ASSERT_TRUE(
        runtime.reload_configuration(
            make_replacement_snapshot()
        ).valid()
    );

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.configuration_version, 9);
    EXPECT_EQ(snapshot.configured_alarm_count, 1);
    EXPECT_EQ(snapshot.indexed_tag_count, 1);
    EXPECT_EQ(snapshot.indexed_condition_count, 1);

    const auto old_tag_result = runtime.evaluate_configured(
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(81.0),
            1
        )
    );

    EXPECT_TRUE(old_tag_result.empty());

    const auto new_tag_result = runtime.evaluate_configured(
        make_telemetry_value(
            "tag-flow",
            dispatcher::telemetry::TagValue(101.0),
            2
        )
    );

    EXPECT_EQ(new_tag_result.configured_alarm_count(), 1);
    EXPECT_EQ(new_tag_result.batch_result().activated_count(), 1);
}

TEST(AlarmRuntimeIndexTests, FailedReloadDoesNotReplaceIndexes)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(
        runtime.reload_configuration(
            make_indexed_snapshot(7)
        ).valid()
    );

    const auto rejected_result = runtime.reload_configuration(
        make_draft_snapshot()
    );

    ASSERT_FALSE(rejected_result.valid());

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.configuration_version, 7);
    EXPECT_EQ(snapshot.configured_alarm_count, 3);
    EXPECT_EQ(snapshot.indexed_tag_count, 2);
    EXPECT_EQ(snapshot.indexed_condition_count, 3);

    const auto result = runtime.evaluate_configured(
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(81.0),
            1
        )
    );

    EXPECT_EQ(result.configured_alarm_count(), 2);
}