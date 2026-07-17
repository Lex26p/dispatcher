#include <dispatcher/alarm/alarm_condition_type.hpp>
#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_evaluation_result.hpp>
#include <dispatcher/alarm/alarm_evaluator.hpp>
#include <dispatcher/alarm/alarm_state.hpp>
#include <dispatcher/alarm/alarm_state_store.hpp>
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

    dispatcher::alarm::ThresholdAlarmCondition make_high_condition()
    {
        return dispatcher::alarm::ThresholdAlarmCondition(
            dispatcher::alarm::AlarmConditionType::High,
            80.0
        );
    }
}

TEST(AlarmEvaluationStatusTests, ToStringReturnsExpectedValues)
{
    using dispatcher::alarm::AlarmEvaluationStatus;
    using dispatcher::alarm::to_string;

    EXPECT_EQ(to_string(AlarmEvaluationStatus::Evaluated), "evaluated");
    EXPECT_EQ(to_string(AlarmEvaluationStatus::DisabledAlarm), "disabled_alarm");
    EXPECT_EQ(to_string(AlarmEvaluationStatus::TagMismatch), "tag_mismatch");
    EXPECT_EQ(
        to_string(AlarmEvaluationStatus::UnsupportedValueType),
        "unsupported_value_type"
    );
}

TEST(AlarmEvaluationResultTests, EvaluatedResultReportsState)
{
    const dispatcher::alarm::AlarmEvaluationResult result(
        dispatcher::alarm::AlarmEvaluationStatus::Evaluated,
        dispatcher::alarm::AlarmTransitionType::Activated,
        dispatcher::alarm::AlarmState::Normal,
        dispatcher::alarm::AlarmState::Active,
        true,
        std::nullopt
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_FALSE(result.skipped());

    EXPECT_TRUE(result.transitioned());
    EXPECT_TRUE(result.activated());
    EXPECT_FALSE(result.cleared());

    EXPECT_EQ(result.transition_type(), dispatcher::alarm::AlarmTransitionType::Activated);
    EXPECT_EQ(result.previous_state(), dispatcher::alarm::AlarmState::Normal);
    EXPECT_EQ(result.new_state(), dispatcher::alarm::AlarmState::Active);
    EXPECT_TRUE(result.condition_active());
    EXPECT_FALSE(result.event().has_value());
}

TEST(AlarmEvaluationResultTests, SkippedResultReportsSkipped)
{
    const dispatcher::alarm::AlarmEvaluationResult result(
        dispatcher::alarm::AlarmEvaluationStatus::DisabledAlarm,
        dispatcher::alarm::AlarmTransitionType::None,
        dispatcher::alarm::AlarmState::Normal,
        dispatcher::alarm::AlarmState::Normal,
        false,
        std::nullopt
    );

    EXPECT_FALSE(result.evaluated());
    EXPECT_TRUE(result.skipped());

    EXPECT_FALSE(result.transitioned());
    EXPECT_FALSE(result.activated());
    EXPECT_FALSE(result.cleared());

    EXPECT_EQ(result.status(), dispatcher::alarm::AlarmEvaluationStatus::DisabledAlarm);
}

TEST(AlarmEvaluatorTests, DisabledAlarmIsSkippedAndDoesNotUpdateStateStore)
{
    dispatcher::alarm::AlarmStateStore state_store;
    dispatcher::alarm::AlarmEvaluator evaluator(state_store);

    const auto definition = make_alarm_definition(false);
    const auto condition = make_high_condition();

    const auto result = evaluator.evaluate(
        definition,
        condition,
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(81.0)
        )
    );

    EXPECT_EQ(result.status(), dispatcher::alarm::AlarmEvaluationStatus::DisabledAlarm);
    EXPECT_TRUE(result.skipped());
    EXPECT_FALSE(result.event().has_value());

    EXPECT_FALSE(state_store.has_state(definition.alarm_id()));
    EXPECT_EQ(state_store.size(), 0);
}

TEST(AlarmEvaluatorTests, TagMismatchIsSkippedAndDoesNotUpdateStateStore)
{
    dispatcher::alarm::AlarmStateStore state_store;
    dispatcher::alarm::AlarmEvaluator evaluator(state_store);

    const auto definition = make_alarm_definition();
    const auto condition = make_high_condition();

    const auto result = evaluator.evaluate(
        definition,
        condition,
        make_telemetry_value(
            "tag-pressure",
            dispatcher::telemetry::TagValue(81.0)
        )
    );

    EXPECT_EQ(result.status(), dispatcher::alarm::AlarmEvaluationStatus::TagMismatch);
    EXPECT_TRUE(result.skipped());
    EXPECT_FALSE(result.event().has_value());

    EXPECT_FALSE(state_store.has_state(definition.alarm_id()));
    EXPECT_EQ(state_store.size(), 0);
}

TEST(AlarmEvaluatorTests, UnsupportedValueTypeIsSkippedAndDoesNotUpdateStateStore)
{
    dispatcher::alarm::AlarmStateStore state_store;
    dispatcher::alarm::AlarmEvaluator evaluator(state_store);

    const auto definition = make_alarm_definition();
    const auto condition = make_high_condition();

    const auto result = evaluator.evaluate(
        definition,
        condition,
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(std::string{ "auto" })
        )
    );

    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::AlarmEvaluationStatus::UnsupportedValueType
    );

    EXPECT_TRUE(result.skipped());
    EXPECT_FALSE(result.event().has_value());

    EXPECT_FALSE(state_store.has_state(definition.alarm_id()));
    EXPECT_EQ(state_store.size(), 0);
}

TEST(AlarmEvaluatorTests, NormalToActiveCreatesEventAndUpdatesStateStore)
{
    dispatcher::alarm::AlarmStateStore state_store;
    dispatcher::alarm::AlarmEvaluator evaluator(state_store);

    const auto definition = make_alarm_definition();
    const auto condition = make_high_condition();

    const auto result = evaluator.evaluate(
        definition,
        condition,
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(81.0),
            42
        )
    );

    EXPECT_EQ(result.status(), dispatcher::alarm::AlarmEvaluationStatus::Evaluated);
    EXPECT_TRUE(result.evaluated());
    EXPECT_TRUE(result.condition_active());

    EXPECT_TRUE(result.transitioned());
    EXPECT_TRUE(result.activated());
    EXPECT_FALSE(result.cleared());

    EXPECT_EQ(result.previous_state(), dispatcher::alarm::AlarmState::Normal);
    EXPECT_EQ(result.new_state(), dispatcher::alarm::AlarmState::Active);

    ASSERT_TRUE(result.event().has_value());
    EXPECT_EQ(
        result.event()->transition_type(),
        dispatcher::alarm::AlarmTransitionType::Activated
    );
    EXPECT_EQ(result.event()->sequence(), 42);

    EXPECT_TRUE(state_store.has_state(definition.alarm_id()));
    EXPECT_EQ(state_store.state_of(definition.alarm_id()), dispatcher::alarm::AlarmState::Active);
}

TEST(AlarmEvaluatorTests, ActiveToActiveDoesNotCreateEventButKeepsState)
{
    dispatcher::alarm::AlarmStateStore state_store;

    const auto definition = make_alarm_definition();

    state_store.set_state(
        definition.alarm_id(),
        dispatcher::alarm::AlarmState::Active
    );

    dispatcher::alarm::AlarmEvaluator evaluator(state_store);

    const auto condition = make_high_condition();

    const auto result = evaluator.evaluate(
        definition,
        condition,
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(82.0),
            43
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_TRUE(result.condition_active());

    EXPECT_FALSE(result.transitioned());
    EXPECT_FALSE(result.event().has_value());

    EXPECT_EQ(result.previous_state(), dispatcher::alarm::AlarmState::Active);
    EXPECT_EQ(result.new_state(), dispatcher::alarm::AlarmState::Active);

    EXPECT_EQ(state_store.state_of(definition.alarm_id()), dispatcher::alarm::AlarmState::Active);
}

TEST(AlarmEvaluatorTests, ActiveToNormalCreatesClearedEventAndUpdatesStateStore)
{
    dispatcher::alarm::AlarmStateStore state_store;

    const auto definition = make_alarm_definition();

    state_store.set_state(
        definition.alarm_id(),
        dispatcher::alarm::AlarmState::Active
    );

    dispatcher::alarm::AlarmEvaluator evaluator(state_store);

    const auto condition = make_high_condition();

    const auto result = evaluator.evaluate(
        definition,
        condition,
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(79.0),
            44
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_FALSE(result.condition_active());

    EXPECT_TRUE(result.transitioned());
    EXPECT_FALSE(result.activated());
    EXPECT_TRUE(result.cleared());

    EXPECT_EQ(result.previous_state(), dispatcher::alarm::AlarmState::Active);
    EXPECT_EQ(result.new_state(), dispatcher::alarm::AlarmState::Normal);

    ASSERT_TRUE(result.event().has_value());
    EXPECT_EQ(
        result.event()->transition_type(),
        dispatcher::alarm::AlarmTransitionType::Cleared
    );
    EXPECT_EQ(result.event()->sequence(), 44);

    EXPECT_EQ(state_store.state_of(definition.alarm_id()), dispatcher::alarm::AlarmState::Normal);
}

TEST(AlarmEvaluatorTests, NormalToNormalStoresKnownNormalState)
{
    dispatcher::alarm::AlarmStateStore state_store;
    dispatcher::alarm::AlarmEvaluator evaluator(state_store);

    const auto definition = make_alarm_definition();
    const auto condition = make_high_condition();

    const auto result = evaluator.evaluate(
        definition,
        condition,
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(79.0),
            45
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_FALSE(result.condition_active());

    EXPECT_FALSE(result.transitioned());
    EXPECT_FALSE(result.event().has_value());

    EXPECT_EQ(result.previous_state(), dispatcher::alarm::AlarmState::Normal);
    EXPECT_EQ(result.new_state(), dispatcher::alarm::AlarmState::Normal);

    EXPECT_TRUE(state_store.has_state(definition.alarm_id()));
    EXPECT_EQ(state_store.state_of(definition.alarm_id()), dispatcher::alarm::AlarmState::Normal);
}

TEST(AlarmEvaluatorTests, StateStoreAccessorReturnsUnderlyingStore)
{
    dispatcher::alarm::AlarmStateStore state_store;

    state_store.set_state(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        dispatcher::alarm::AlarmState::Active
    );

    dispatcher::alarm::AlarmEvaluator evaluator(state_store);

    EXPECT_EQ(evaluator.state_store().size(), 1);
}