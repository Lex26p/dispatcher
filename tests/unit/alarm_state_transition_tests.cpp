#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_runtime_event.hpp>
#include <dispatcher/alarm/alarm_state.hpp>
#include <dispatcher/alarm/alarm_state_transition.hpp>
#include <dispatcher/alarm/alarm_transition_type.hpp>
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

TEST(AlarmTransitionTypeTests, ToStringReturnsExpectedValues)
{
    using dispatcher::alarm::AlarmTransitionType;
    using dispatcher::alarm::to_string;

    EXPECT_EQ(to_string(AlarmTransitionType::None), "none");
    EXPECT_EQ(to_string(AlarmTransitionType::Activated), "activated");
    EXPECT_EQ(to_string(AlarmTransitionType::Cleared), "cleared");
}

TEST(AlarmStateTransitionTests, NormalToActiveCreatesActivatedEvent)
{
    const auto definition = make_alarm_definition();
    const auto telemetry_value = make_telemetry_value(81.0, 42);

    const auto result = dispatcher::alarm::evaluate_alarm_state_transition(
        definition,
        dispatcher::alarm::AlarmState::Normal,
        true,
        telemetry_value
    );

    EXPECT_TRUE(result.transitioned());
    EXPECT_TRUE(result.activated());
    EXPECT_FALSE(result.cleared());

    EXPECT_EQ(result.transition_type(), dispatcher::alarm::AlarmTransitionType::Activated);
    EXPECT_EQ(result.previous_state(), dispatcher::alarm::AlarmState::Normal);
    EXPECT_EQ(result.new_state(), dispatcher::alarm::AlarmState::Active);

    ASSERT_TRUE(result.event().has_value());

    EXPECT_EQ(
        result.event()->alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_EQ(
        result.event()->tag_id(),
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    EXPECT_EQ(result.event()->severity(), dispatcher::alarm::AlarmSeverity::Critical);
    EXPECT_EQ(
        result.event()->transition_type(),
        dispatcher::alarm::AlarmTransitionType::Activated
    );

    EXPECT_EQ(result.event()->previous_state(), dispatcher::alarm::AlarmState::Normal);
    EXPECT_EQ(result.event()->new_state(), dispatcher::alarm::AlarmState::Active);
    EXPECT_EQ(result.event()->sequence(), 42);
}

TEST(AlarmStateTransitionTests, ActiveToNormalCreatesClearedEvent)
{
    const auto definition = make_alarm_definition();
    const auto telemetry_value = make_telemetry_value(79.0, 43);

    const auto result = dispatcher::alarm::evaluate_alarm_state_transition(
        definition,
        dispatcher::alarm::AlarmState::Active,
        false,
        telemetry_value
    );

    EXPECT_TRUE(result.transitioned());
    EXPECT_FALSE(result.activated());
    EXPECT_TRUE(result.cleared());

    EXPECT_EQ(result.transition_type(), dispatcher::alarm::AlarmTransitionType::Cleared);
    EXPECT_EQ(result.previous_state(), dispatcher::alarm::AlarmState::Active);
    EXPECT_EQ(result.new_state(), dispatcher::alarm::AlarmState::Normal);

    ASSERT_TRUE(result.event().has_value());

    EXPECT_EQ(
        result.event()->transition_type(),
        dispatcher::alarm::AlarmTransitionType::Cleared
    );

    EXPECT_EQ(result.event()->previous_state(), dispatcher::alarm::AlarmState::Active);
    EXPECT_EQ(result.event()->new_state(), dispatcher::alarm::AlarmState::Normal);
    EXPECT_EQ(result.event()->sequence(), 43);
}

TEST(AlarmStateTransitionTests, NormalToNormalDoesNotCreateEvent)
{
    const auto definition = make_alarm_definition();
    const auto telemetry_value = make_telemetry_value(79.0, 44);

    const auto result = dispatcher::alarm::evaluate_alarm_state_transition(
        definition,
        dispatcher::alarm::AlarmState::Normal,
        false,
        telemetry_value
    );

    EXPECT_FALSE(result.transitioned());
    EXPECT_FALSE(result.activated());
    EXPECT_FALSE(result.cleared());

    EXPECT_EQ(result.transition_type(), dispatcher::alarm::AlarmTransitionType::None);
    EXPECT_EQ(result.previous_state(), dispatcher::alarm::AlarmState::Normal);
    EXPECT_EQ(result.new_state(), dispatcher::alarm::AlarmState::Normal);
    EXPECT_FALSE(result.event().has_value());
}

TEST(AlarmStateTransitionTests, ActiveToActiveDoesNotCreateEvent)
{
    const auto definition = make_alarm_definition();
    const auto telemetry_value = make_telemetry_value(81.0, 45);

    const auto result = dispatcher::alarm::evaluate_alarm_state_transition(
        definition,
        dispatcher::alarm::AlarmState::Active,
        true,
        telemetry_value
    );

    EXPECT_FALSE(result.transitioned());
    EXPECT_EQ(result.transition_type(), dispatcher::alarm::AlarmTransitionType::None);
    EXPECT_EQ(result.previous_state(), dispatcher::alarm::AlarmState::Active);
    EXPECT_EQ(result.new_state(), dispatcher::alarm::AlarmState::Active);
    EXPECT_FALSE(result.event().has_value());
}

TEST(AlarmStateTransitionTests, AcknowledgedStaysAcknowledgedWhileConditionIsActive)
{
    const auto definition = make_alarm_definition();
    const auto telemetry_value = make_telemetry_value(81.0, 46);

    const auto result = dispatcher::alarm::evaluate_alarm_state_transition(
        definition,
        dispatcher::alarm::AlarmState::Acknowledged,
        true,
        telemetry_value
    );

    EXPECT_FALSE(result.transitioned());
    EXPECT_EQ(result.transition_type(), dispatcher::alarm::AlarmTransitionType::None);
    EXPECT_EQ(result.previous_state(), dispatcher::alarm::AlarmState::Acknowledged);
    EXPECT_EQ(result.new_state(), dispatcher::alarm::AlarmState::Acknowledged);
    EXPECT_FALSE(result.event().has_value());
}

TEST(AlarmStateTransitionTests, AcknowledgedToNormalCreatesClearedEvent)
{
    const auto definition = make_alarm_definition();
    const auto telemetry_value = make_telemetry_value(79.0, 47);

    const auto result = dispatcher::alarm::evaluate_alarm_state_transition(
        definition,
        dispatcher::alarm::AlarmState::Acknowledged,
        false,
        telemetry_value
    );

    EXPECT_TRUE(result.transitioned());
    EXPECT_TRUE(result.cleared());

    EXPECT_EQ(result.transition_type(), dispatcher::alarm::AlarmTransitionType::Cleared);
    EXPECT_EQ(result.previous_state(), dispatcher::alarm::AlarmState::Acknowledged);
    EXPECT_EQ(result.new_state(), dispatcher::alarm::AlarmState::Normal);

    ASSERT_TRUE(result.event().has_value());

    EXPECT_EQ(
        result.event()->transition_type(),
        dispatcher::alarm::AlarmTransitionType::Cleared
    );

    EXPECT_EQ(
        result.event()->previous_state(),
        dispatcher::alarm::AlarmState::Acknowledged
    );

    EXPECT_EQ(result.event()->new_state(), dispatcher::alarm::AlarmState::Normal);
    EXPECT_EQ(result.event()->sequence(), 47);
}

TEST(AlarmStateTransitionTests, EventUsesTelemetryTimestamps)
{
    const auto definition = make_alarm_definition();
    const auto telemetry_value = make_telemetry_value(81.0, 48);

    const auto result = dispatcher::alarm::evaluate_alarm_state_transition(
        definition,
        dispatcher::alarm::AlarmState::Normal,
        true,
        telemetry_value
    );

    ASSERT_TRUE(result.event().has_value());

    EXPECT_EQ(
        result.event()->source_timestamp(),
        telemetry_value.source_timestamp()
    );

    EXPECT_EQ(
        result.event()->event_timestamp(),
        telemetry_value.ingest_timestamp()
    );
}