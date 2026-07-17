#include <dispatcher/alarm/alarm_condition_type.hpp>
#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_evaluation_result.hpp>
#include <dispatcher/alarm/alarm_evaluator.hpp>
#include <dispatcher/alarm/alarm_event_store.hpp>
#include <dispatcher/alarm/alarm_runtime_event.hpp>
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
#include <optional>
#include <string>
#include <utility>

namespace
{
    dispatcher::alarm::AlarmRuntimeEvent make_event(
        std::string alarm_id,
        std::string tag_id,
        dispatcher::alarm::AlarmTransitionType transition_type,
        dispatcher::alarm::AlarmState previous_state,
        dispatcher::alarm::AlarmState new_state,
        std::uint64_t sequence
    )
    {
        using dispatcher::alarm::AlarmRuntimeEvent;

        const auto now = AlarmRuntimeEvent::Clock::now();

        return AlarmRuntimeEvent(
            dispatcher::domain::AlarmId{ std::move(alarm_id) },
            dispatcher::domain::TagId{ std::move(tag_id) },
            dispatcher::alarm::AlarmSeverity::Critical,
            transition_type,
            previous_state,
            new_state,
            now,
            now,
            sequence
        );
    }

    dispatcher::alarm::AlarmDefinition make_alarm_definition()
    {
        return dispatcher::alarm::AlarmDefinitionBuilder{}
            .alarm_id(dispatcher::domain::AlarmId{ "alarm-temperature-high" })
            .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
            .name("temperature_high")
            .description("Temperature is too high")
            .severity(dispatcher::alarm::AlarmSeverity::Critical)
            .enabled(true)
            .config_version(1)
            .build();
    }

    dispatcher::telemetry::TelemetryValue make_telemetry_value(
        double value,
        std::uint64_t sequence
    )
    {
        using dispatcher::domain::Quality;
        using dispatcher::domain::TagId;
        using dispatcher::telemetry::TagValue;
        using dispatcher::telemetry::TelemetryValue;

        const auto now = TelemetryValue::Clock::now();

        return TelemetryValue(
            TagId{ "tag-temperature" },
            TagValue(value),
            Quality::Good,
            now,
            now,
            sequence
        );
    }
}

TEST(AlarmEventStoreTests, StartsEmpty)
{
    const dispatcher::alarm::AlarmEventStore store;

    EXPECT_TRUE(store.empty());
    EXPECT_EQ(store.size(), 0);
}

TEST(AlarmEventStoreTests, AppendStoresEvent)
{
    dispatcher::alarm::AlarmEventStore store;

    store.append(
        make_event(
            "alarm-temperature-high",
            "tag-temperature",
            dispatcher::alarm::AlarmTransitionType::Activated,
            dispatcher::alarm::AlarmState::Normal,
            dispatcher::alarm::AlarmState::Active,
            1
        )
    );

    ASSERT_EQ(store.size(), 1);
    ASSERT_EQ(store.events().size(), 1);

    EXPECT_EQ(
        store.events()[0].alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_EQ(
        store.events()[0].tag_id(),
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    EXPECT_EQ(
        store.events()[0].transition_type(),
        dispatcher::alarm::AlarmTransitionType::Activated
    );

    EXPECT_EQ(store.events()[0].sequence(), 1);
}

TEST(AlarmEventStoreTests, AppendIfPresentStoresEvent)
{
    dispatcher::alarm::AlarmEventStore store;

    const std::optional<dispatcher::alarm::AlarmRuntimeEvent> event{
        make_event(
            "alarm-temperature-high",
            "tag-temperature",
            dispatcher::alarm::AlarmTransitionType::Activated,
            dispatcher::alarm::AlarmState::Normal,
            dispatcher::alarm::AlarmState::Active,
            1
        )
    };

    const auto appended = store.append_if_present(event);

    EXPECT_TRUE(appended);
    EXPECT_EQ(store.size(), 1);
}

TEST(AlarmEventStoreTests, AppendIfPresentDoesNothingForEmptyOptional)
{
    dispatcher::alarm::AlarmEventStore store;

    const std::optional<dispatcher::alarm::AlarmRuntimeEvent> event;

    const auto appended = store.append_if_present(event);

    EXPECT_FALSE(appended);
    EXPECT_TRUE(store.empty());
}

TEST(AlarmEventStoreTests, FindByAlarmIdReturnsMatchingEvents)
{
    dispatcher::alarm::AlarmEventStore store;

    store.append(
        make_event(
            "alarm-temperature-high",
            "tag-temperature",
            dispatcher::alarm::AlarmTransitionType::Activated,
            dispatcher::alarm::AlarmState::Normal,
            dispatcher::alarm::AlarmState::Active,
            1
        )
    );

    store.append(
        make_event(
            "alarm-pressure-low",
            "tag-pressure",
            dispatcher::alarm::AlarmTransitionType::Activated,
            dispatcher::alarm::AlarmState::Normal,
            dispatcher::alarm::AlarmState::Active,
            2
        )
    );

    store.append(
        make_event(
            "alarm-temperature-high",
            "tag-temperature",
            dispatcher::alarm::AlarmTransitionType::Cleared,
            dispatcher::alarm::AlarmState::Active,
            dispatcher::alarm::AlarmState::Normal,
            3
        )
    );

    const auto result = store.find_by_alarm_id(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    ASSERT_EQ(result.size(), 2);

    EXPECT_EQ(result[0].sequence(), 1);
    EXPECT_EQ(result[1].sequence(), 3);
}

TEST(AlarmEventStoreTests, FindByTagIdReturnsMatchingEvents)
{
    dispatcher::alarm::AlarmEventStore store;

    store.append(
        make_event(
            "alarm-temperature-high",
            "tag-temperature",
            dispatcher::alarm::AlarmTransitionType::Activated,
            dispatcher::alarm::AlarmState::Normal,
            dispatcher::alarm::AlarmState::Active,
            1
        )
    );

    store.append(
        make_event(
            "alarm-pressure-low",
            "tag-pressure",
            dispatcher::alarm::AlarmTransitionType::Activated,
            dispatcher::alarm::AlarmState::Normal,
            dispatcher::alarm::AlarmState::Active,
            2
        )
    );

    store.append(
        make_event(
            "alarm-temperature-high-high",
            "tag-temperature",
            dispatcher::alarm::AlarmTransitionType::Activated,
            dispatcher::alarm::AlarmState::Normal,
            dispatcher::alarm::AlarmState::Active,
            3
        )
    );

    const auto result = store.find_by_tag_id(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_EQ(result.size(), 2);

    EXPECT_EQ(result[0].sequence(), 1);
    EXPECT_EQ(result[1].sequence(), 3);
}

TEST(AlarmEventStoreTests, FindReturnsEmptyWhenNoEventsMatch)
{
    dispatcher::alarm::AlarmEventStore store;

    store.append(
        make_event(
            "alarm-temperature-high",
            "tag-temperature",
            dispatcher::alarm::AlarmTransitionType::Activated,
            dispatcher::alarm::AlarmState::Normal,
            dispatcher::alarm::AlarmState::Active,
            1
        )
    );

    EXPECT_TRUE(
        store.find_by_alarm_id(
            dispatcher::domain::AlarmId{ "unknown-alarm" }
        ).empty()
    );

    EXPECT_TRUE(
        store.find_by_tag_id(
            dispatcher::domain::TagId{ "unknown-tag" }
        ).empty()
    );
}

TEST(AlarmEventStoreTests, AppendFromEvaluationResultStoresTransitionEvent)
{
    dispatcher::alarm::AlarmStateStore state_store;
    dispatcher::alarm::AlarmEvaluator evaluator(state_store);
    dispatcher::alarm::AlarmEventStore event_store;

    const auto definition = make_alarm_definition();

    const dispatcher::alarm::ThresholdAlarmCondition condition(
        dispatcher::alarm::AlarmConditionType::High,
        80.0
    );

    const auto evaluation_result = evaluator.evaluate(
        definition,
        condition,
        make_telemetry_value(81.0, 42)
    );

    ASSERT_TRUE(evaluation_result.activated());
    ASSERT_TRUE(evaluation_result.event().has_value());

    const auto appended = event_store.append_from_evaluation_result(
        evaluation_result
    );

    EXPECT_TRUE(appended);
    ASSERT_EQ(event_store.size(), 1);

    EXPECT_EQ(
        event_store.events()[0].transition_type(),
        dispatcher::alarm::AlarmTransitionType::Activated
    );

    EXPECT_EQ(event_store.events()[0].sequence(), 42);
}

TEST(AlarmEventStoreTests, AppendFromEvaluationResultDoesNothingWithoutEvent)
{
    dispatcher::alarm::AlarmStateStore state_store;
    dispatcher::alarm::AlarmEvaluator evaluator(state_store);
    dispatcher::alarm::AlarmEventStore event_store;

    const auto definition = make_alarm_definition();

    const dispatcher::alarm::ThresholdAlarmCondition condition(
        dispatcher::alarm::AlarmConditionType::High,
        80.0
    );

    const auto evaluation_result = evaluator.evaluate(
        definition,
        condition,
        make_telemetry_value(79.0, 43)
    );

    ASSERT_FALSE(evaluation_result.transitioned());
    ASSERT_FALSE(evaluation_result.event().has_value());

    const auto appended = event_store.append_from_evaluation_result(
        evaluation_result
    );

    EXPECT_FALSE(appended);
    EXPECT_TRUE(event_store.empty());
}

TEST(AlarmEventStoreTests, ClearRemovesAllEvents)
{
    dispatcher::alarm::AlarmEventStore store;

    store.append(
        make_event(
            "alarm-temperature-high",
            "tag-temperature",
            dispatcher::alarm::AlarmTransitionType::Activated,
            dispatcher::alarm::AlarmState::Normal,
            dispatcher::alarm::AlarmState::Active,
            1
        )
    );

    store.append(
        make_event(
            "alarm-temperature-high",
            "tag-temperature",
            dispatcher::alarm::AlarmTransitionType::Cleared,
            dispatcher::alarm::AlarmState::Active,
            dispatcher::alarm::AlarmState::Normal,
            2
        )
    );

    ASSERT_EQ(store.size(), 2);

    store.clear();

    EXPECT_TRUE(store.empty());
    EXPECT_EQ(store.size(), 0);
    EXPECT_TRUE(store.events().empty());
}