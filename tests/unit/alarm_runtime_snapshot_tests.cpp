#include <dispatcher/alarm/alarm_condition_type.hpp>
#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_evaluation_candidate.hpp>
#include <dispatcher/alarm/alarm_runtime.hpp>
#include <dispatcher/alarm/alarm_runtime_snapshot.hpp>
#include <dispatcher/alarm/alarm_state.hpp>
#include <dispatcher/alarm/threshold_alarm_condition.hpp>
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
        std::string alarm_id = "alarm-temperature-high",
        std::string tag_id = "tag-temperature",
        bool enabled = true
    )
    {
        return dispatcher::alarm::AlarmDefinitionBuilder{}
            .alarm_id(dispatcher::domain::AlarmId{ std::move(alarm_id) })
            .tag_id(dispatcher::domain::TagId{ std::move(tag_id) })
            .name("temperature_high")
            .description("Temperature is too high")
            .severity(dispatcher::alarm::AlarmSeverity::Critical)
            .enabled(enabled)
            .config_version(1)
            .build();
    }

    dispatcher::alarm::ThresholdAlarmCondition make_high_condition()
    {
        return dispatcher::alarm::ThresholdAlarmCondition(
            dispatcher::alarm::AlarmConditionType::High,
            80.0
        );
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

    dispatcher::alarm::AlarmEvaluationCandidate make_candidate(
        const dispatcher::alarm::AlarmDefinition& definition,
        const dispatcher::alarm::ThresholdAlarmCondition& condition,
        dispatcher::telemetry::TelemetryValue telemetry_value
    )
    {
        return dispatcher::alarm::AlarmEvaluationCandidate{
            definition,
            condition,
            std::move(telemetry_value)
        };
    }
}

TEST(AlarmRuntimeSnapshotTests, EmptyRuntimeSnapshotStartsAtZero)
{
    const dispatcher::alarm::AlarmRuntime runtime;

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.state_store_size, 0);
    EXPECT_EQ(snapshot.event_store_size, 0);

    EXPECT_EQ(snapshot.total_count, 0);
    EXPECT_EQ(snapshot.evaluated_count, 0);
    EXPECT_EQ(snapshot.skipped_count, 0);

    EXPECT_EQ(snapshot.disabled_alarm_count, 0);
    EXPECT_EQ(snapshot.tag_mismatch_count, 0);
    EXPECT_EQ(snapshot.unsupported_value_type_count, 0);

    EXPECT_EQ(snapshot.activated_count, 0);
    EXPECT_EQ(snapshot.cleared_count, 0);
    EXPECT_EQ(snapshot.no_transition_count, 0);

    EXPECT_EQ(snapshot.stored_event_count, 0);
}

TEST(AlarmRuntimeSnapshotTests, SnapshotReflectsActivatedAlarm)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto definition = make_alarm_definition();
    const auto condition = make_high_condition();

    ASSERT_TRUE(
        runtime.evaluate(
            definition,
            condition,
            make_telemetry_value(
                "tag-temperature",
                dispatcher::telemetry::TagValue(81.0),
                1
            )
        ).activated()
    );

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.state_store_size, 1);
    EXPECT_EQ(snapshot.event_store_size, 1);

    EXPECT_EQ(snapshot.total_count, 1);
    EXPECT_EQ(snapshot.evaluated_count, 1);
    EXPECT_EQ(snapshot.skipped_count, 0);

    EXPECT_EQ(snapshot.activated_count, 1);
    EXPECT_EQ(snapshot.cleared_count, 0);
    EXPECT_EQ(snapshot.no_transition_count, 0);

    EXPECT_EQ(snapshot.stored_event_count, 1);
}

TEST(AlarmRuntimeSnapshotTests, SnapshotReflectsNoTransition)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto definition = make_alarm_definition();
    const auto condition = make_high_condition();

    ASSERT_FALSE(
        runtime.evaluate(
            definition,
            condition,
            make_telemetry_value(
                "tag-temperature",
                dispatcher::telemetry::TagValue(79.0),
                1
            )
        ).transitioned()
    );

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.state_store_size, 1);
    EXPECT_EQ(snapshot.event_store_size, 0);

    EXPECT_EQ(snapshot.total_count, 1);
    EXPECT_EQ(snapshot.evaluated_count, 1);
    EXPECT_EQ(snapshot.skipped_count, 0);

    EXPECT_EQ(snapshot.activated_count, 0);
    EXPECT_EQ(snapshot.cleared_count, 0);
    EXPECT_EQ(snapshot.no_transition_count, 1);

    EXPECT_EQ(snapshot.stored_event_count, 0);
}

TEST(AlarmRuntimeSnapshotTests, SnapshotReflectsClearedAlarm)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto definition = make_alarm_definition();
    const auto condition = make_high_condition();

    ASSERT_TRUE(
        runtime.evaluate(
            definition,
            condition,
            make_telemetry_value(
                "tag-temperature",
                dispatcher::telemetry::TagValue(81.0),
                1
            )
        ).activated()
    );

    ASSERT_TRUE(
        runtime.evaluate(
            definition,
            condition,
            make_telemetry_value(
                "tag-temperature",
                dispatcher::telemetry::TagValue(79.0),
                2
            )
        ).cleared()
    );

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.state_store_size, 1);
    EXPECT_EQ(snapshot.event_store_size, 2);

    EXPECT_EQ(snapshot.total_count, 2);
    EXPECT_EQ(snapshot.evaluated_count, 2);
    EXPECT_EQ(snapshot.skipped_count, 0);

    EXPECT_EQ(snapshot.activated_count, 1);
    EXPECT_EQ(snapshot.cleared_count, 1);
    EXPECT_EQ(snapshot.no_transition_count, 0);

    EXPECT_EQ(snapshot.stored_event_count, 2);
}

TEST(AlarmRuntimeSnapshotTests, SnapshotReflectsSkippedEvaluations)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto disabled_definition = make_alarm_definition(
        "alarm-disabled",
        "tag-temperature",
        false
    );

    const auto enabled_definition = make_alarm_definition(
        "alarm-temperature-high",
        "tag-temperature",
        true
    );

    const auto condition = make_high_condition();

    EXPECT_TRUE(
        runtime.evaluate(
            disabled_definition,
            condition,
            make_telemetry_value(
                "tag-temperature",
                dispatcher::telemetry::TagValue(81.0),
                1
            )
        ).skipped()
    );

    EXPECT_TRUE(
        runtime.evaluate(
            enabled_definition,
            condition,
            make_telemetry_value(
                "tag-pressure",
                dispatcher::telemetry::TagValue(81.0),
                2
            )
        ).skipped()
    );

    EXPECT_TRUE(
        runtime.evaluate(
            enabled_definition,
            condition,
            make_telemetry_value(
                "tag-temperature",
                dispatcher::telemetry::TagValue(std::string{ "auto" }),
                3
            )
        ).skipped()
    );

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.state_store_size, 0);
    EXPECT_EQ(snapshot.event_store_size, 0);

    EXPECT_EQ(snapshot.total_count, 3);
    EXPECT_EQ(snapshot.evaluated_count, 0);
    EXPECT_EQ(snapshot.skipped_count, 3);

    EXPECT_EQ(snapshot.disabled_alarm_count, 1);
    EXPECT_EQ(snapshot.tag_mismatch_count, 1);
    EXPECT_EQ(snapshot.unsupported_value_type_count, 1);

    EXPECT_EQ(snapshot.activated_count, 0);
    EXPECT_EQ(snapshot.cleared_count, 0);
    EXPECT_EQ(snapshot.no_transition_count, 0);

    EXPECT_EQ(snapshot.stored_event_count, 0);
}

TEST(AlarmRuntimeSnapshotTests, SnapshotReflectsBatchEvaluation)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto definition = make_alarm_definition();
    const auto condition = make_high_condition();

    const std::vector<dispatcher::alarm::AlarmEvaluationCandidate> candidates{
        make_candidate(
            definition,
            condition,
            make_telemetry_value(
                "tag-temperature",
                dispatcher::telemetry::TagValue(81.0),
                1
            )
        ),
        make_candidate(
            definition,
            condition,
            make_telemetry_value(
                "tag-temperature",
                dispatcher::telemetry::TagValue(82.0),
                2
            )
        ),
        make_candidate(
            definition,
            condition,
            make_telemetry_value(
                "tag-temperature",
                dispatcher::telemetry::TagValue(79.0),
                3
            )
        )
    };

    const auto result = runtime.evaluate_batch(candidates);

    ASSERT_EQ(result.total_count(), 3);
    ASSERT_EQ(result.activated_count(), 1);
    ASSERT_EQ(result.no_transition_count(), 1);
    ASSERT_EQ(result.cleared_count(), 1);

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.state_store_size, 1);
    EXPECT_EQ(snapshot.event_store_size, 2);

    EXPECT_EQ(snapshot.total_count, 3);
    EXPECT_EQ(snapshot.evaluated_count, 3);
    EXPECT_EQ(snapshot.skipped_count, 0);

    EXPECT_EQ(snapshot.activated_count, 1);
    EXPECT_EQ(snapshot.cleared_count, 1);
    EXPECT_EQ(snapshot.no_transition_count, 1);

    EXPECT_EQ(snapshot.stored_event_count, 2);
}

TEST(AlarmRuntimeSnapshotTests, ResetStatisticsOnlyClearsSnapshotCounters)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto definition = make_alarm_definition();
    const auto condition = make_high_condition();

    ASSERT_TRUE(
        runtime.evaluate(
            definition,
            condition,
            make_telemetry_value(
                "tag-temperature",
                dispatcher::telemetry::TagValue(81.0),
                1
            )
        ).activated()
    );

    ASSERT_EQ(runtime.runtime_snapshot().state_store_size, 1);
    ASSERT_EQ(runtime.runtime_snapshot().event_store_size, 1);
    ASSERT_EQ(runtime.runtime_snapshot().total_count, 1);

    runtime.reset_statistics();

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.state_store_size, 1);
    EXPECT_EQ(snapshot.event_store_size, 1);

    EXPECT_EQ(snapshot.total_count, 0);
    EXPECT_EQ(snapshot.evaluated_count, 0);
    EXPECT_EQ(snapshot.skipped_count, 0);

    EXPECT_EQ(snapshot.activated_count, 0);
    EXPECT_EQ(snapshot.cleared_count, 0);
    EXPECT_EQ(snapshot.no_transition_count, 0);

    EXPECT_EQ(snapshot.stored_event_count, 0);
}