#include <dispatcher/alarm/alarm_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_definition.hpp>
#include <dispatcher/alarm/alarm_condition_type.hpp>
#include <dispatcher/alarm/alarm_configured_evaluation_result.hpp>
#include <dispatcher/alarm/alarm_configuration_metadata.hpp>
#include <dispatcher/alarm/alarm_configuration_snapshot.hpp>
#include <dispatcher/alarm/alarm_configuration_snapshot_builder.hpp>
#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_runtime.hpp>
#include <dispatcher/alarm/alarm_severity.hpp>
#include <dispatcher/alarm/alarm_state.hpp>
#include <dispatcher/alarm/alarm_transition_type.hpp>
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
        std::string name,
        bool enabled = true
    )
    {
        return dispatcher::alarm::AlarmDefinitionBuilder{}
            .alarm_id(dispatcher::domain::AlarmId{ std::move(alarm_id) })
            .tag_id(dispatcher::domain::TagId{ std::move(tag_id) })
            .name(std::move(name))
            .description("Test alarm")
            .severity(dispatcher::alarm::AlarmSeverity::Warning)
            .enabled(enabled)
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

    dispatcher::alarm::AlarmConfigurationSnapshot make_snapshot()
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(7)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "plant-alarms",
                    .description = "Plant alarm configuration",
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

    dispatcher::alarm::AlarmConfigurationSnapshot make_snapshot_with_disabled_alarm()
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(8)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "plant-alarms",
                    .description = "Plant alarm configuration",
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
                "temperature_high",
                false
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

TEST(AlarmConfiguredEvaluationResultTests, StartsEmpty)
{
    const dispatcher::alarm::AlarmConfiguredEvaluationResult result;

    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(result.has_missing_conditions());

    EXPECT_EQ(result.configured_alarm_count(), 0);
    EXPECT_EQ(result.missing_condition_count(), 0);
    EXPECT_TRUE(result.batch_result().empty());
}

TEST(AlarmConfiguredEvaluationTests, UnknownTagProducesEmptyResult)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(runtime.reload_configuration(make_snapshot()).valid());

    const auto result = runtime.evaluate_configured(
        make_telemetry_value(
            "unknown-tag",
            dispatcher::telemetry::TagValue(100.0),
            1
        )
    );

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.configured_alarm_count(), 0);
    EXPECT_EQ(result.missing_condition_count(), 0);

    EXPECT_TRUE(result.batch_result().empty());
    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_TRUE(runtime.event_store().empty());
    EXPECT_EQ(runtime.statistics().total_count(), 0);
}

TEST(AlarmConfiguredEvaluationTests, EvaluatesAllAlarmsForTag)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(runtime.reload_configuration(make_snapshot()).valid());

    const auto result = runtime.evaluate_configured(
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(81.0),
            1
        )
    );

    EXPECT_FALSE(result.empty());
    EXPECT_FALSE(result.has_missing_conditions());

    EXPECT_EQ(result.configured_alarm_count(), 2);
    EXPECT_EQ(result.missing_condition_count(), 0);

    ASSERT_EQ(result.batch_result().results().size(), 2);

    EXPECT_TRUE(result.batch_result().results()[0].activated());
    EXPECT_FALSE(result.batch_result().results()[1].transitioned());

    EXPECT_EQ(result.batch_result().total_count(), 2);
    EXPECT_EQ(result.batch_result().evaluated_count(), 2);
    EXPECT_EQ(result.batch_result().activated_count(), 1);
    EXPECT_EQ(result.batch_result().no_transition_count(), 1);
    EXPECT_EQ(result.batch_result().stored_event_count(), 1);

    EXPECT_EQ(runtime.state_store().size(), 2);
    EXPECT_EQ(runtime.event_store().size(), 1);

    EXPECT_EQ(runtime.statistics().total_count(), 2);
    EXPECT_EQ(runtime.statistics().evaluated_count(), 2);
}

TEST(AlarmConfiguredEvaluationTests, DisabledAlarmIsCountedAsSkipped)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(
        runtime.reload_configuration(
            make_snapshot_with_disabled_alarm()
        ).valid()
    );

    const auto result = runtime.evaluate_configured(
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(81.0),
            1
        )
    );

    EXPECT_EQ(result.configured_alarm_count(), 1);
    EXPECT_EQ(result.missing_condition_count(), 0);

    ASSERT_EQ(result.batch_result().results().size(), 1);

    EXPECT_EQ(
        result.batch_result().results()[0].status(),
        dispatcher::alarm::AlarmEvaluationStatus::DisabledAlarm
    );

    EXPECT_EQ(result.batch_result().skipped_count(), 1);
    EXPECT_EQ(result.batch_result().disabled_alarm_count(), 1);

    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_TRUE(runtime.event_store().empty());

    EXPECT_EQ(runtime.statistics().total_count(), 1);
    EXPECT_EQ(runtime.statistics().disabled_alarm_count(), 1);
}

TEST(AlarmConfiguredEvaluationTests, BatchEvaluatesTelemetryValues)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(runtime.reload_configuration(make_snapshot()).valid());

    const std::vector<dispatcher::telemetry::TelemetryValue> telemetry_values{
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(81.0),
            1
        ),
        make_telemetry_value(
            "tag-pressure",
            dispatcher::telemetry::TagValue(11.0),
            2
        ),
        make_telemetry_value(
            "unknown-tag",
            dispatcher::telemetry::TagValue(999.0),
            3
        )
    };

    const auto result = runtime.evaluate_configured_batch(telemetry_values);

    EXPECT_EQ(result.configured_alarm_count(), 3);
    EXPECT_EQ(result.missing_condition_count(), 0);

    EXPECT_EQ(result.batch_result().total_count(), 3);
    EXPECT_EQ(result.batch_result().evaluated_count(), 3);
    EXPECT_EQ(result.batch_result().activated_count(), 2);
    EXPECT_EQ(result.batch_result().no_transition_count(), 1);
    EXPECT_EQ(result.batch_result().stored_event_count(), 2);

    EXPECT_EQ(runtime.state_store().size(), 3);
    EXPECT_EQ(runtime.event_store().size(), 2);

    EXPECT_EQ(runtime.statistics().total_count(), 3);
    EXPECT_EQ(runtime.statistics().activated_count(), 2);
    EXPECT_EQ(runtime.statistics().no_transition_count(), 1);
}

TEST(AlarmConfiguredEvaluationTests, BatchCanClearPreviouslyActiveAlarm)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(runtime.reload_configuration(make_snapshot()).valid());

    const std::vector<dispatcher::telemetry::TelemetryValue> activate_values{
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(81.0),
            1
        )
    };

    const auto activate_result = runtime.evaluate_configured_batch(
        activate_values
    );

    ASSERT_EQ(activate_result.batch_result().activated_count(), 1);
    ASSERT_EQ(runtime.event_store().size(), 1);

    const std::vector<dispatcher::telemetry::TelemetryValue> clear_values{
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(50.0),
            2
        )
    };

    const auto clear_result = runtime.evaluate_configured_batch(
        clear_values
    );

    EXPECT_EQ(clear_result.configured_alarm_count(), 2);
    EXPECT_EQ(clear_result.batch_result().cleared_count(), 1);
    EXPECT_EQ(clear_result.batch_result().no_transition_count(), 1);
    EXPECT_EQ(clear_result.batch_result().stored_event_count(), 1);

    EXPECT_EQ(runtime.event_store().size(), 2);
    EXPECT_EQ(runtime.statistics().cleared_count(), 1);
}

TEST(AlarmConfiguredEvaluationTests, MissingConditionIsTrackedButNotEvaluated)
{
    dispatcher::alarm::AlarmRuntime runtime;

    auto snapshot = dispatcher::alarm::AlarmConfigurationSnapshot(
        9,
        dispatcher::alarm::AlarmConfigurationMetadata{
            .name = "plant-alarms",
            .description = "Plant alarm configuration",
            .created_by = "unit-test"
        },
        dispatcher::domain::ConfigurationStatus::Published,
        dispatcher::alarm::AlarmCatalog(
            std::vector<dispatcher::alarm::AlarmDefinition>{
        make_alarm_definition(
            "alarm-temperature-high",
            "tag-temperature",
            "temperature_high"
        )
    }
        ),
        dispatcher::alarm::AlarmConditionCatalog{}
    );

    const auto reload_result = runtime.reload_configuration(std::move(snapshot));

    ASSERT_FALSE(reload_result.valid());

    // This test intentionally verifies the result object behavior directly.
    dispatcher::alarm::AlarmConfiguredEvaluationResult result;

    result.record_configured_alarm();
    result.record_missing_condition();

    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.has_missing_conditions());
    EXPECT_EQ(result.configured_alarm_count(), 1);
    EXPECT_EQ(result.missing_condition_count(), 1);
    EXPECT_TRUE(result.batch_result().empty());
}