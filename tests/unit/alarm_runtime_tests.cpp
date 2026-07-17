#include <dispatcher/alarm/alarm_condition_type.hpp>
#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_runtime.hpp>
#include <dispatcher/alarm/alarm_runtime_statistics.hpp>
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

namespace
{
    dispatcher::alarm::AlarmDefinition make_alarm_definition(
        bool enabled = true
    )
    {
        return dispatcher::alarm::AlarmDefinitionBuilder{}
            .alarm_id(dispatcher::domain::AlarmId{ "alarm-temperature-high" })
            .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
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
}

TEST(AlarmRuntimeStatisticsTests, StartsAtZero)
{
    const dispatcher::alarm::AlarmRuntimeStatistics statistics;

    EXPECT_EQ(statistics.total_count(), 0);
    EXPECT_EQ(statistics.evaluated_count(), 0);
    EXPECT_EQ(statistics.skipped_count(), 0);

    EXPECT_EQ(statistics.disabled_alarm_count(), 0);
    EXPECT_EQ(statistics.tag_mismatch_count(), 0);
    EXPECT_EQ(statistics.unsupported_value_type_count(), 0);

    EXPECT_EQ(statistics.activated_count(), 0);
    EXPECT_EQ(statistics.cleared_count(), 0);
    EXPECT_EQ(statistics.no_transition_count(), 0);

    EXPECT_EQ(statistics.stored_event_count(), 0);
}

TEST(AlarmRuntimeTests, StartsWithEmptyStoresAndZeroStatistics)
{
    const dispatcher::alarm::AlarmRuntime runtime;

    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_TRUE(runtime.event_store().empty());

    EXPECT_EQ(runtime.statistics().total_count(), 0);
    EXPECT_EQ(runtime.statistics().evaluated_count(), 0);
    EXPECT_EQ(runtime.statistics().stored_event_count(), 0);
}

TEST(AlarmRuntimeTests, NormalToActiveStoresStateEventAndStatistics)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto definition = make_alarm_definition();
    const auto condition = make_high_condition();

    const auto result = runtime.evaluate(
        definition,
        condition,
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(81.0),
            42
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_TRUE(result.activated());

    EXPECT_EQ(
        runtime.state_store().state_of(definition.alarm_id()),
        dispatcher::alarm::AlarmState::Active
    );

    ASSERT_EQ(runtime.event_store().size(), 1);

    EXPECT_EQ(
        runtime.event_store().events()[0].transition_type(),
        dispatcher::alarm::AlarmTransitionType::Activated
    );

    EXPECT_EQ(runtime.event_store().events()[0].sequence(), 42);

    EXPECT_EQ(runtime.statistics().total_count(), 1);
    EXPECT_EQ(runtime.statistics().evaluated_count(), 1);
    EXPECT_EQ(runtime.statistics().skipped_count(), 0);

    EXPECT_EQ(runtime.statistics().activated_count(), 1);
    EXPECT_EQ(runtime.statistics().cleared_count(), 0);
    EXPECT_EQ(runtime.statistics().no_transition_count(), 0);

    EXPECT_EQ(runtime.statistics().stored_event_count(), 1);
}

TEST(AlarmRuntimeTests, ActiveToActiveKeepsStateAndDoesNotStoreEvent)
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

    const auto result = runtime.evaluate(
        definition,
        condition,
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(82.0),
            2
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_FALSE(result.transitioned());

    EXPECT_EQ(
        runtime.state_store().state_of(definition.alarm_id()),
        dispatcher::alarm::AlarmState::Active
    );

    EXPECT_EQ(runtime.event_store().size(), 1);

    EXPECT_EQ(runtime.statistics().total_count(), 2);
    EXPECT_EQ(runtime.statistics().evaluated_count(), 2);
    EXPECT_EQ(runtime.statistics().activated_count(), 1);
    EXPECT_EQ(runtime.statistics().no_transition_count(), 1);
    EXPECT_EQ(runtime.statistics().stored_event_count(), 1);
}

TEST(AlarmRuntimeTests, ActiveToNormalStoresClearedEventAndStatistics)
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

    const auto result = runtime.evaluate(
        definition,
        condition,
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(79.0),
            2
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_TRUE(result.cleared());

    EXPECT_EQ(
        runtime.state_store().state_of(definition.alarm_id()),
        dispatcher::alarm::AlarmState::Normal
    );

    ASSERT_EQ(runtime.event_store().size(), 2);

    EXPECT_EQ(
        runtime.event_store().events()[1].transition_type(),
        dispatcher::alarm::AlarmTransitionType::Cleared
    );

    EXPECT_EQ(runtime.event_store().events()[1].sequence(), 2);

    EXPECT_EQ(runtime.statistics().total_count(), 2);
    EXPECT_EQ(runtime.statistics().evaluated_count(), 2);
    EXPECT_EQ(runtime.statistics().activated_count(), 1);
    EXPECT_EQ(runtime.statistics().cleared_count(), 1);
    EXPECT_EQ(runtime.statistics().stored_event_count(), 2);
}

TEST(AlarmRuntimeTests, NormalToNormalStoresKnownNormalStateButNoEvent)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto definition = make_alarm_definition();
    const auto condition = make_high_condition();

    const auto result = runtime.evaluate(
        definition,
        condition,
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(79.0),
            1
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_FALSE(result.transitioned());

    EXPECT_TRUE(runtime.state_store().has_state(definition.alarm_id()));

    EXPECT_EQ(
        runtime.state_store().state_of(definition.alarm_id()),
        dispatcher::alarm::AlarmState::Normal
    );

    EXPECT_TRUE(runtime.event_store().empty());

    EXPECT_EQ(runtime.statistics().total_count(), 1);
    EXPECT_EQ(runtime.statistics().evaluated_count(), 1);
    EXPECT_EQ(runtime.statistics().no_transition_count(), 1);
    EXPECT_EQ(runtime.statistics().stored_event_count(), 0);
}

TEST(AlarmRuntimeTests, DisabledAlarmUpdatesStatisticsButDoesNotStoreStateOrEvent)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto definition = make_alarm_definition(false);
    const auto condition = make_high_condition();

    const auto result = runtime.evaluate(
        definition,
        condition,
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(81.0),
            1
        )
    );

    EXPECT_TRUE(result.skipped());
    EXPECT_EQ(result.status(), dispatcher::alarm::AlarmEvaluationStatus::DisabledAlarm);

    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_TRUE(runtime.event_store().empty());

    EXPECT_EQ(runtime.statistics().total_count(), 1);
    EXPECT_EQ(runtime.statistics().evaluated_count(), 0);
    EXPECT_EQ(runtime.statistics().skipped_count(), 1);
    EXPECT_EQ(runtime.statistics().disabled_alarm_count(), 1);
}

TEST(AlarmRuntimeTests, TagMismatchUpdatesStatisticsButDoesNotStoreStateOrEvent)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto definition = make_alarm_definition();
    const auto condition = make_high_condition();

    const auto result = runtime.evaluate(
        definition,
        condition,
        make_telemetry_value(
            "tag-pressure",
            dispatcher::telemetry::TagValue(81.0),
            1
        )
    );

    EXPECT_TRUE(result.skipped());
    EXPECT_EQ(result.status(), dispatcher::alarm::AlarmEvaluationStatus::TagMismatch);

    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_TRUE(runtime.event_store().empty());

    EXPECT_EQ(runtime.statistics().total_count(), 1);
    EXPECT_EQ(runtime.statistics().evaluated_count(), 0);
    EXPECT_EQ(runtime.statistics().skipped_count(), 1);
    EXPECT_EQ(runtime.statistics().tag_mismatch_count(), 1);
}

TEST(AlarmRuntimeTests, UnsupportedValueTypeUpdatesStatisticsButDoesNotStoreStateOrEvent)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto definition = make_alarm_definition();
    const auto condition = make_high_condition();

    const auto result = runtime.evaluate(
        definition,
        condition,
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(std::string{ "auto" }),
            1
        )
    );

    EXPECT_TRUE(result.skipped());
    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::AlarmEvaluationStatus::UnsupportedValueType
    );

    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_TRUE(runtime.event_store().empty());

    EXPECT_EQ(runtime.statistics().total_count(), 1);
    EXPECT_EQ(runtime.statistics().evaluated_count(), 0);
    EXPECT_EQ(runtime.statistics().skipped_count(), 1);
    EXPECT_EQ(runtime.statistics().unsupported_value_type_count(), 1);
}

TEST(AlarmRuntimeTests, ResetStatisticsDoesNotClearStores)
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

    ASSERT_EQ(runtime.statistics().total_count(), 1);
    ASSERT_EQ(runtime.event_store().size(), 1);
    ASSERT_EQ(
        runtime.state_store().state_of(definition.alarm_id()),
        dispatcher::alarm::AlarmState::Active
    );

    runtime.reset_statistics();

    EXPECT_EQ(runtime.statistics().total_count(), 0);
    EXPECT_EQ(runtime.statistics().evaluated_count(), 0);
    EXPECT_EQ(runtime.statistics().stored_event_count(), 0);

    EXPECT_EQ(runtime.event_store().size(), 1);
    EXPECT_EQ(
        runtime.state_store().state_of(definition.alarm_id()),
        dispatcher::alarm::AlarmState::Active
    );
}