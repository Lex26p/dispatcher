#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_state.hpp>
#include <dispatcher/alarm/alarm_state_store.hpp>
#include <dispatcher/alarm/alarm_state_transition.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <utility>

namespace
{
    dispatcher::alarm::AlarmDefinition make_alarm_definition(
        dispatcher::domain::AlarmId alarm_id,
        dispatcher::domain::TagId tag_id
    )
    {
        return dispatcher::alarm::AlarmDefinitionBuilder{}
            .alarm_id(std::move(alarm_id))
            .tag_id(std::move(tag_id))
            .name("temperature_high")
            .description("Temperature is too high")
            .severity(dispatcher::alarm::AlarmSeverity::Critical)
            .enabled(true)
            .config_version(1)
            .build();
    }

    dispatcher::telemetry::TelemetryValue make_telemetry_value(
        dispatcher::domain::TagId tag_id,
        double value,
        std::uint64_t sequence
    )
    {
        using dispatcher::domain::Quality;
        using dispatcher::telemetry::TagValue;
        using dispatcher::telemetry::TelemetryValue;

        const auto now = TelemetryValue::Clock::now();

        return TelemetryValue(
            std::move(tag_id),
            TagValue(value),
            Quality::Good,
            now,
            now,
            sequence
        );
    }
}

TEST(AlarmStateStoreTests, StartsEmpty)
{
    const dispatcher::alarm::AlarmStateStore store;

    EXPECT_TRUE(store.empty());
    EXPECT_EQ(store.size(), 0);
}

TEST(AlarmStateStoreTests, UnknownAlarmDefaultsToNormal)
{
    const dispatcher::alarm::AlarmStateStore store;

    EXPECT_FALSE(
        store.has_state(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" }
        )
    );

    EXPECT_EQ(
        store.state_of(dispatcher::domain::AlarmId{ "alarm-temperature-high" }),
        dispatcher::alarm::AlarmState::Normal
    );
}

TEST(AlarmStateStoreTests, SetStateStoresState)
{
    dispatcher::alarm::AlarmStateStore store;

    store.set_state(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        dispatcher::alarm::AlarmState::Active
    );

    EXPECT_FALSE(store.empty());
    EXPECT_EQ(store.size(), 1);

    EXPECT_TRUE(
        store.has_state(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" }
        )
    );

    EXPECT_EQ(
        store.state_of(dispatcher::domain::AlarmId{ "alarm-temperature-high" }),
        dispatcher::alarm::AlarmState::Active
    );
}

TEST(AlarmStateStoreTests, SetStateOverwritesExistingState)
{
    dispatcher::alarm::AlarmStateStore store;

    store.set_state(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        dispatcher::alarm::AlarmState::Active
    );

    store.set_state(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        dispatcher::alarm::AlarmState::Acknowledged
    );

    EXPECT_EQ(store.size(), 1);

    EXPECT_EQ(
        store.state_of(dispatcher::domain::AlarmId{ "alarm-temperature-high" }),
        dispatcher::alarm::AlarmState::Acknowledged
    );
}

TEST(AlarmStateStoreTests, StoresStatesIndependentlyByAlarmId)
{
    dispatcher::alarm::AlarmStateStore store;

    store.set_state(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        dispatcher::alarm::AlarmState::Active
    );

    store.set_state(
        dispatcher::domain::AlarmId{ "alarm-pressure-low" },
        dispatcher::alarm::AlarmState::Acknowledged
    );

    EXPECT_EQ(store.size(), 2);

    EXPECT_EQ(
        store.state_of(dispatcher::domain::AlarmId{ "alarm-temperature-high" }),
        dispatcher::alarm::AlarmState::Active
    );

    EXPECT_EQ(
        store.state_of(dispatcher::domain::AlarmId{ "alarm-pressure-low" }),
        dispatcher::alarm::AlarmState::Acknowledged
    );
}

TEST(AlarmStateStoreTests, ApplyTransitionStoresNewState)
{
    dispatcher::alarm::AlarmStateStore store;

    const auto definition = make_alarm_definition(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    const auto telemetry_value = make_telemetry_value(
        dispatcher::domain::TagId{ "tag-temperature" },
        81.0,
        1
    );

    const auto transition = dispatcher::alarm::evaluate_alarm_state_transition(
        definition,
        dispatcher::alarm::AlarmState::Normal,
        true,
        telemetry_value
    );

    ASSERT_TRUE(transition.activated());

    store.apply_transition(
        definition.alarm_id(),
        transition
    );

    EXPECT_TRUE(store.has_state(definition.alarm_id()));
    EXPECT_EQ(store.state_of(definition.alarm_id()), dispatcher::alarm::AlarmState::Active);
}

TEST(AlarmStateStoreTests, ApplyTransitionStoresNormalAfterClear)
{
    dispatcher::alarm::AlarmStateStore store;

    const auto definition = make_alarm_definition(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    store.set_state(
        definition.alarm_id(),
        dispatcher::alarm::AlarmState::Active
    );

    const auto telemetry_value = make_telemetry_value(
        dispatcher::domain::TagId{ "tag-temperature" },
        79.0,
        2
    );

    const auto transition = dispatcher::alarm::evaluate_alarm_state_transition(
        definition,
        store.state_of(definition.alarm_id()),
        false,
        telemetry_value
    );

    ASSERT_TRUE(transition.cleared());

    store.apply_transition(
        definition.alarm_id(),
        transition
    );

    EXPECT_EQ(store.state_of(definition.alarm_id()), dispatcher::alarm::AlarmState::Normal);
}

TEST(AlarmStateStoreTests, ApplyTransitionAlsoStoresNoTransitionState)
{
    dispatcher::alarm::AlarmStateStore store;

    const auto definition = make_alarm_definition(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    const auto telemetry_value = make_telemetry_value(
        dispatcher::domain::TagId{ "tag-temperature" },
        79.0,
        3
    );

    const auto transition = dispatcher::alarm::evaluate_alarm_state_transition(
        definition,
        dispatcher::alarm::AlarmState::Normal,
        false,
        telemetry_value
    );

    ASSERT_FALSE(transition.transitioned());

    store.apply_transition(
        definition.alarm_id(),
        transition
    );

    EXPECT_TRUE(store.has_state(definition.alarm_id()));
    EXPECT_EQ(store.state_of(definition.alarm_id()), dispatcher::alarm::AlarmState::Normal);
}

TEST(AlarmStateStoreTests, ClearRemovesAllStates)
{
    dispatcher::alarm::AlarmStateStore store;

    store.set_state(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        dispatcher::alarm::AlarmState::Active
    );

    store.set_state(
        dispatcher::domain::AlarmId{ "alarm-pressure-low" },
        dispatcher::alarm::AlarmState::Acknowledged
    );

    ASSERT_EQ(store.size(), 2);

    store.clear();

    EXPECT_TRUE(store.empty());
    EXPECT_EQ(store.size(), 0);

    EXPECT_EQ(
        store.state_of(dispatcher::domain::AlarmId{ "alarm-temperature-high" }),
        dispatcher::alarm::AlarmState::Normal
    );
}