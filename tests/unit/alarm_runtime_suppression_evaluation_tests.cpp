#include <dispatcher/alarm/alarm_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_definition.hpp>
#include <dispatcher/alarm/alarm_condition_type.hpp>
#include <dispatcher/alarm/alarm_configuration_metadata.hpp>
#include <dispatcher/alarm/alarm_configuration_snapshot.hpp>
#include <dispatcher/alarm/alarm_configuration_snapshot_builder.hpp>
#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_runtime.hpp>
#include <dispatcher/alarm/alarm_suppression_command.hpp>
#include <dispatcher/alarm/alarm_suppression_mode.hpp>
#include <dispatcher/alarm/alarm_suppression_reason.hpp>
#include <dispatcher/alarm/alarm_suppression_status.hpp>
#include <dispatcher/alarm/alarm_transition_type.hpp>
#include <dispatcher/alarm/threshold_alarm_condition.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <chrono>
#include <thread>

namespace
{
    dispatcher::alarm::AlarmDefinition make_suppression_eval_alarm_definition(
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

    dispatcher::alarm::AlarmConditionDefinition
        make_suppression_eval_condition_definition(
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

    dispatcher::alarm::AlarmConfigurationSnapshot
        make_single_alarm_suppression_eval_snapshot()
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(11)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "suppression-test-alarms",
                    .description = "Suppression evaluation test configuration",
                    .created_by = "unit-test"
                }
            )
            .status(dispatcher::domain::ConfigurationStatus::Published)
            .alarm_catalog(
                dispatcher::alarm::AlarmCatalog(
                    std::vector<dispatcher::alarm::AlarmDefinition>{
            make_suppression_eval_alarm_definition(
                "alarm-temperature-high",
                "tag-temperature",
                "temperature_high"
            )
        }
                )
            )
            .condition_catalog(
                dispatcher::alarm::AlarmConditionCatalog(
                    std::vector<dispatcher::alarm::AlarmConditionDefinition>{
            make_suppression_eval_condition_definition(
                "alarm-temperature-high",
                dispatcher::alarm::AlarmConditionType::High,
                80.0
            )
        }
                )
            )
            .build();
    }

    dispatcher::telemetry::TelemetryValue
        make_suppression_eval_telemetry_value(
            std::string tag_id = "tag-temperature",
            dispatcher::telemetry::TagValue value =
            dispatcher::telemetry::TagValue(81.0),
            std::uint64_t sequence = 1
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

    dispatcher::alarm::AlarmSuppressionCommand
        make_suppression_eval_command(
            dispatcher::domain::AlarmId alarm_id =
            dispatcher::domain::AlarmId{ "alarm-temperature-high" },
            dispatcher::alarm::AlarmSuppressionMode mode =
            dispatcher::alarm::AlarmSuppressionMode::Shelved
        )
    {
        return dispatcher::alarm::AlarmSuppressionCommand(
            alarm_id,
            "operator-1",
            mode,
            dispatcher::alarm::AlarmSuppressionReason::Maintenance,
            "planned maintenance"
        );
    }

    dispatcher::alarm::AlarmRuntime make_suppression_eval_runtime()
    {
        dispatcher::alarm::AlarmRuntime runtime;

        const auto reload_result = runtime.reload_configuration(
            make_single_alarm_suppression_eval_snapshot()
        );

        if (!reload_result.valid())
        {
            throw std::runtime_error(
                "failed to reload suppression evaluation configuration"
            );
        }

        return runtime;
    }
}

TEST(
    AlarmRuntimeSuppressionEvaluationTests,
    ConfiguredEvaluationEvaluatesUnsuppressedAlarm
)
{
    auto runtime = make_suppression_eval_runtime();

    const auto result = runtime.evaluate_configured(
        make_suppression_eval_telemetry_value()
    );

    EXPECT_EQ(result.configured_alarm_count(), 1);
    EXPECT_EQ(result.missing_condition_count(), 0);
    EXPECT_EQ(result.suppressed_alarm_count(), 0);

    EXPECT_FALSE(result.has_missing_conditions());
    EXPECT_FALSE(result.has_suppressed_alarms());

    ASSERT_EQ(result.batch_result().results().size(), 1);
    EXPECT_TRUE(result.batch_result().results().front().activated());

    EXPECT_EQ(runtime.state_store().size(), 1);
    EXPECT_EQ(runtime.event_store().size(), 1);

    EXPECT_EQ(
        runtime.event_store().events().front().transition_type(),
        dispatcher::alarm::AlarmTransitionType::Activated
    );

    EXPECT_EQ(runtime.statistics().total_count(), 1);
    EXPECT_EQ(runtime.statistics().evaluated_count(), 1);
    EXPECT_EQ(runtime.statistics().activated_count(), 1);
    EXPECT_EQ(runtime.statistics().stored_event_count(), 1);
}

TEST(
    AlarmRuntimeSuppressionEvaluationTests,
    ConfiguredEvaluationSkipsShelvedAlarm
)
{
    auto runtime = make_suppression_eval_runtime();

    const auto suppress_result = runtime.suppress(
        make_suppression_eval_command(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" },
            dispatcher::alarm::AlarmSuppressionMode::Shelved
        )
    );

    ASSERT_TRUE(suppress_result.success());

    const auto result = runtime.evaluate_configured(
        make_suppression_eval_telemetry_value()
    );

    EXPECT_EQ(result.configured_alarm_count(), 1);
    EXPECT_EQ(result.missing_condition_count(), 0);
    EXPECT_EQ(result.suppressed_alarm_count(), 1);

    EXPECT_FALSE(result.has_missing_conditions());
    EXPECT_TRUE(result.has_suppressed_alarms());

    EXPECT_TRUE(result.batch_result().empty());
    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_TRUE(runtime.event_store().empty());

    EXPECT_EQ(runtime.statistics().total_count(), 0);
    EXPECT_EQ(runtime.statistics().evaluated_count(), 0);
    EXPECT_EQ(runtime.statistics().activated_count(), 0);
    EXPECT_EQ(runtime.statistics().stored_event_count(), 0);
}

TEST(
    AlarmRuntimeSuppressionEvaluationTests,
    ConfiguredEvaluationSkipsSuppressedAlarm
)
{
    auto runtime = make_suppression_eval_runtime();

    const auto suppress_result = runtime.suppress(
        make_suppression_eval_command(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" },
            dispatcher::alarm::AlarmSuppressionMode::Suppressed
        )
    );

    ASSERT_TRUE(suppress_result.success());

    const auto result = runtime.evaluate_configured(
        make_suppression_eval_telemetry_value()
    );

    EXPECT_EQ(result.configured_alarm_count(), 1);
    EXPECT_EQ(result.missing_condition_count(), 0);
    EXPECT_EQ(result.suppressed_alarm_count(), 1);

    EXPECT_TRUE(result.has_suppressed_alarms());

    EXPECT_TRUE(result.batch_result().empty());
    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_TRUE(runtime.event_store().empty());

    EXPECT_EQ(runtime.statistics().total_count(), 0);
    EXPECT_EQ(runtime.statistics().stored_event_count(), 0);
}

TEST(
    AlarmRuntimeSuppressionEvaluationTests,
    ConfiguredEvaluationSkipsInhibitedAlarm
)
{
    auto runtime = make_suppression_eval_runtime();

    const auto suppress_result = runtime.suppress(
        make_suppression_eval_command(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" },
            dispatcher::alarm::AlarmSuppressionMode::Inhibited
        )
    );

    ASSERT_TRUE(suppress_result.success());

    const auto result = runtime.evaluate_configured(
        make_suppression_eval_telemetry_value()
    );

    EXPECT_EQ(result.configured_alarm_count(), 1);
    EXPECT_EQ(result.missing_condition_count(), 0);
    EXPECT_EQ(result.suppressed_alarm_count(), 1);

    EXPECT_TRUE(result.has_suppressed_alarms());

    EXPECT_TRUE(result.batch_result().empty());
    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_TRUE(runtime.event_store().empty());

    EXPECT_EQ(runtime.statistics().total_count(), 0);
    EXPECT_EQ(runtime.statistics().stored_event_count(), 0);
}

TEST(
    AlarmRuntimeSuppressionEvaluationTests,
    ConfiguredEvaluationEvaluatesAlarmAfterSuppressionIsCleared
)
{
    auto runtime = make_suppression_eval_runtime();

    ASSERT_TRUE(
        runtime.suppress(
            make_suppression_eval_command()
        ).success()
    );

    ASSERT_TRUE(
        runtime.clear_suppression(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" }
        ).success()
    );

    const auto result = runtime.evaluate_configured(
        make_suppression_eval_telemetry_value()
    );

    EXPECT_EQ(result.configured_alarm_count(), 1);
    EXPECT_EQ(result.missing_condition_count(), 0);
    EXPECT_EQ(result.suppressed_alarm_count(), 0);

    EXPECT_FALSE(result.has_suppressed_alarms());

    ASSERT_EQ(result.batch_result().results().size(), 1);
    EXPECT_TRUE(result.batch_result().results().front().activated());

    EXPECT_EQ(runtime.state_store().size(), 1);
    EXPECT_EQ(runtime.event_store().size(), 1);

    EXPECT_EQ(runtime.statistics().total_count(), 1);
    EXPECT_EQ(runtime.statistics().activated_count(), 1);
    EXPECT_EQ(runtime.statistics().stored_event_count(), 1);
}

TEST(
    AlarmRuntimeSuppressionEvaluationTests,
    ConfiguredBatchAggregatesSuppressedAlarmCount
)
{
    auto runtime = make_suppression_eval_runtime();

    ASSERT_TRUE(
        runtime.suppress(
            make_suppression_eval_command()
        ).success()
    );

    const std::vector<dispatcher::telemetry::TelemetryValue> telemetry_values{
        make_suppression_eval_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(81.0),
            1
        ),
        make_suppression_eval_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(82.0),
            2
        )
    };

    const auto result = runtime.evaluate_configured_batch(telemetry_values);

    EXPECT_EQ(result.configured_alarm_count(), 2);
    EXPECT_EQ(result.missing_condition_count(), 0);
    EXPECT_EQ(result.suppressed_alarm_count(), 2);

    EXPECT_TRUE(result.has_suppressed_alarms());

    EXPECT_TRUE(result.batch_result().empty());
    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_TRUE(runtime.event_store().empty());

    EXPECT_EQ(runtime.statistics().total_count(), 0);
    EXPECT_EQ(runtime.statistics().stored_event_count(), 0);
}

TEST(
    AlarmRuntimeSuppressionEvaluationTests,
    ConfiguredEvaluationResultTracksSuppressedAlarmCount
)
{
    dispatcher::alarm::AlarmConfiguredEvaluationResult result;

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.suppressed_alarm_count(), 0);
    EXPECT_FALSE(result.has_suppressed_alarms());

    result.record_configured_alarm();
    result.record_suppressed_alarm();

    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.configured_alarm_count(), 1);
    EXPECT_EQ(result.suppressed_alarm_count(), 1);
    EXPECT_TRUE(result.has_suppressed_alarms());
}

TEST(
    AlarmRuntimeSuppressionEvaluationTests,
    ExpiredSuppressionDoesNotBlockConfiguredEvaluation
)
{
    auto runtime = make_suppression_eval_runtime();

    const auto now =
        dispatcher::alarm::AlarmSuppressionCommand::Clock::now();

    const auto suppress_result = runtime.suppress(
        dispatcher::alarm::AlarmSuppressionCommand(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" },
            "operator-1",
            dispatcher::alarm::AlarmSuppressionMode::Shelved,
            dispatcher::alarm::AlarmSuppressionReason::Maintenance,
            "short shelf",
            now + std::chrono::milliseconds(1)
        )
    );

    ASSERT_TRUE(suppress_result.success());
    ASSERT_EQ(runtime.suppression_store().size(), 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    const auto result = runtime.evaluate_configured(
        make_suppression_eval_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(81.0),
            100
        )
    );

    EXPECT_EQ(result.configured_alarm_count(), 1);
    EXPECT_EQ(result.suppressed_alarm_count(), 0);
    EXPECT_FALSE(result.has_suppressed_alarms());

    ASSERT_EQ(result.batch_result().results().size(), 1);
    EXPECT_TRUE(result.batch_result().results().front().activated());

    EXPECT_EQ(runtime.suppression_store().size(), 0);
    EXPECT_EQ(
        runtime.runtime_suppression_snapshot().expired_removed_count,
        1
    );

    EXPECT_EQ(runtime.event_store().size(), 1);
    EXPECT_EQ(runtime.statistics().total_count(), 1);
    EXPECT_EQ(runtime.statistics().activated_count(), 1);
}