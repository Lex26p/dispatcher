#include <dispatcher/alarm/alarm_condition_type.hpp>
#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_evaluation_batch_result.hpp>
#include <dispatcher/alarm/alarm_evaluation_candidate.hpp>
#include <dispatcher/alarm/alarm_runtime.hpp>
#include <dispatcher/alarm/alarm_state.hpp>
#include <dispatcher/alarm/alarm_transition_type.hpp>
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

TEST(AlarmEvaluationBatchResultTests, EmptyBatchResultReportsEmpty)
{
    const dispatcher::alarm::AlarmEvaluationBatchResult result;

    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(result.all_evaluated());
    EXPECT_FALSE(result.has_skipped());

    EXPECT_TRUE(result.results().empty());

    EXPECT_EQ(result.total_count(), 0);
    EXPECT_EQ(result.evaluated_count(), 0);
    EXPECT_EQ(result.skipped_count(), 0);

    EXPECT_EQ(result.activated_count(), 0);
    EXPECT_EQ(result.cleared_count(), 0);
    EXPECT_EQ(result.no_transition_count(), 0);

    EXPECT_EQ(result.stored_event_count(), 0);
}

TEST(AlarmEvaluationBatchResultTests, RecordsActivatedResult)
{
    dispatcher::alarm::AlarmEvaluationBatchResult batch_result;

    batch_result.record(
        dispatcher::alarm::AlarmEvaluationResult(
            dispatcher::alarm::AlarmEvaluationStatus::Evaluated,
            dispatcher::alarm::AlarmTransitionType::Activated,
            dispatcher::alarm::AlarmState::Normal,
            dispatcher::alarm::AlarmState::Active,
            true,
            std::nullopt
        )
    );

    EXPECT_FALSE(batch_result.empty());
    EXPECT_TRUE(batch_result.all_evaluated());
    EXPECT_FALSE(batch_result.has_skipped());

    EXPECT_EQ(batch_result.results().size(), 1);
    EXPECT_EQ(batch_result.total_count(), 1);
    EXPECT_EQ(batch_result.evaluated_count(), 1);
    EXPECT_EQ(batch_result.activated_count(), 1);
}

TEST(AlarmRuntimeBatchTests, EmptyBatchDoesNothing)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const std::vector<dispatcher::alarm::AlarmEvaluationCandidate> candidates;

    const auto result = runtime.evaluate_batch(candidates);

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.total_count(), 0);
    EXPECT_TRUE(result.results().empty());

    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_TRUE(runtime.event_store().empty());
    EXPECT_EQ(runtime.statistics().total_count(), 0);
}

TEST(AlarmRuntimeBatchTests, EvaluatesBatchInOrderAndStoresEvents)
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

    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.all_evaluated());
    EXPECT_FALSE(result.has_skipped());

    ASSERT_EQ(result.results().size(), 3);

    EXPECT_TRUE(result.results()[0].activated());
    EXPECT_FALSE(result.results()[1].transitioned());
    EXPECT_TRUE(result.results()[2].cleared());

    EXPECT_EQ(result.total_count(), 3);
    EXPECT_EQ(result.evaluated_count(), 3);
    EXPECT_EQ(result.skipped_count(), 0);

    EXPECT_EQ(result.activated_count(), 1);
    EXPECT_EQ(result.cleared_count(), 1);
    EXPECT_EQ(result.no_transition_count(), 1);
    EXPECT_EQ(result.stored_event_count(), 2);

    EXPECT_EQ(
        runtime.state_store().state_of(definition.alarm_id()),
        dispatcher::alarm::AlarmState::Normal
    );

    ASSERT_EQ(runtime.event_store().size(), 2);

    EXPECT_EQ(
        runtime.event_store().events()[0].transition_type(),
        dispatcher::alarm::AlarmTransitionType::Activated
    );

    EXPECT_EQ(
        runtime.event_store().events()[1].transition_type(),
        dispatcher::alarm::AlarmTransitionType::Cleared
    );

    EXPECT_EQ(runtime.statistics().total_count(), 3);
    EXPECT_EQ(runtime.statistics().evaluated_count(), 3);
    EXPECT_EQ(runtime.statistics().activated_count(), 1);
    EXPECT_EQ(runtime.statistics().cleared_count(), 1);
    EXPECT_EQ(runtime.statistics().no_transition_count(), 1);
    EXPECT_EQ(runtime.statistics().stored_event_count(), 2);
}

TEST(AlarmRuntimeBatchTests, BatchResultCountsSkippedItems)
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

    const std::vector<dispatcher::alarm::AlarmEvaluationCandidate> candidates{
        make_candidate(
            disabled_definition,
            condition,
            make_telemetry_value(
                "tag-temperature",
                dispatcher::telemetry::TagValue(81.0),
                1
            )
        ),
        make_candidate(
            enabled_definition,
            condition,
            make_telemetry_value(
                "tag-pressure",
                dispatcher::telemetry::TagValue(81.0),
                2
            )
        ),
        make_candidate(
            enabled_definition,
            condition,
            make_telemetry_value(
                "tag-temperature",
                dispatcher::telemetry::TagValue(std::string{"auto"}),
                3
            )
        )
    };

    const auto result = runtime.evaluate_batch(candidates);

    EXPECT_FALSE(result.empty());
    EXPECT_FALSE(result.all_evaluated());
    EXPECT_TRUE(result.has_skipped());

    ASSERT_EQ(result.results().size(), 3);

    EXPECT_EQ(
        result.results()[0].status(),
        dispatcher::alarm::AlarmEvaluationStatus::DisabledAlarm
    );

    EXPECT_EQ(
        result.results()[1].status(),
        dispatcher::alarm::AlarmEvaluationStatus::TagMismatch
    );

    EXPECT_EQ(
        result.results()[2].status(),
        dispatcher::alarm::AlarmEvaluationStatus::UnsupportedValueType
    );

    EXPECT_EQ(result.total_count(), 3);
    EXPECT_EQ(result.evaluated_count(), 0);
    EXPECT_EQ(result.skipped_count(), 3);

    EXPECT_EQ(result.disabled_alarm_count(), 1);
    EXPECT_EQ(result.tag_mismatch_count(), 1);
    EXPECT_EQ(result.unsupported_value_type_count(), 1);

    EXPECT_EQ(result.activated_count(), 0);
    EXPECT_EQ(result.cleared_count(), 0);
    EXPECT_EQ(result.no_transition_count(), 0);
    EXPECT_EQ(result.stored_event_count(), 0);

    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_TRUE(runtime.event_store().empty());

    EXPECT_EQ(runtime.statistics().total_count(), 3);
    EXPECT_EQ(runtime.statistics().evaluated_count(), 0);
    EXPECT_EQ(runtime.statistics().skipped_count(), 3);
    EXPECT_EQ(runtime.statistics().disabled_alarm_count(), 1);
    EXPECT_EQ(runtime.statistics().tag_mismatch_count(), 1);
    EXPECT_EQ(runtime.statistics().unsupported_value_type_count(), 1);
}

TEST(AlarmRuntimeBatchTests, BatchCanEvaluateIndependentAlarms)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto temperature_definition = make_alarm_definition(
        "alarm-temperature-high",
        "tag-temperature",
        true
    );

    const auto pressure_definition = make_alarm_definition(
        "alarm-pressure-high",
        "tag-pressure",
        true
    );

    const auto condition = make_high_condition();

    const std::vector<dispatcher::alarm::AlarmEvaluationCandidate> candidates{
        make_candidate(
            temperature_definition,
            condition,
            make_telemetry_value(
                "tag-temperature",
                dispatcher::telemetry::TagValue(81.0),
                1
            )
        ),
        make_candidate(
            pressure_definition,
            condition,
            make_telemetry_value(
                "tag-pressure",
                dispatcher::telemetry::TagValue(90.0),
                2
            )
        )
    };

    const auto result = runtime.evaluate_batch(candidates);

    EXPECT_TRUE(result.all_evaluated());
    EXPECT_EQ(result.activated_count(), 2);
    EXPECT_EQ(result.stored_event_count(), 2);

    EXPECT_EQ(
        runtime.state_store().state_of(temperature_definition.alarm_id()),
        dispatcher::alarm::AlarmState::Active
    );

    EXPECT_EQ(
        runtime.state_store().state_of(pressure_definition.alarm_id()),
        dispatcher::alarm::AlarmState::Active
    );

    EXPECT_EQ(runtime.event_store().size(), 2);
}