#include <dispatcher/alarm/alarm_condition_type.hpp>
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
    dispatcher::telemetry::TelemetryValue make_telemetry_value(
        std::string tag_id,
        dispatcher::telemetry::TagValue value
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
            1
        );
    }
}

TEST(AlarmConditionTypeTests, ToStringReturnsExpectedValues)
{
    using dispatcher::alarm::AlarmConditionType;
    using dispatcher::alarm::to_string;

    EXPECT_EQ(to_string(AlarmConditionType::High), "high");
    EXPECT_EQ(to_string(AlarmConditionType::HighHigh), "high_high");
    EXPECT_EQ(to_string(AlarmConditionType::Low), "low");
    EXPECT_EQ(to_string(AlarmConditionType::LowLow), "low_low");
}

TEST(ThresholdAlarmEvaluationStatusTests, ToStringReturnsExpectedValues)
{
    using dispatcher::alarm::ThresholdAlarmEvaluationStatus;
    using dispatcher::alarm::to_string;

    EXPECT_EQ(to_string(ThresholdAlarmEvaluationStatus::Evaluated), "evaluated");
    EXPECT_EQ(
        to_string(ThresholdAlarmEvaluationStatus::UnsupportedValueType),
        "unsupported_value_type"
    );
}

TEST(ThresholdAlarmConditionTests, StoresConditionTypeAndThreshold)
{
    const dispatcher::alarm::ThresholdAlarmCondition condition(
        dispatcher::alarm::AlarmConditionType::High,
        80.0
    );

    EXPECT_EQ(
        condition.condition_type(),
        dispatcher::alarm::AlarmConditionType::High
    );

    EXPECT_DOUBLE_EQ(condition.threshold(), 80.0);
}

TEST(ThresholdAlarmConditionTests, HighBecomesActiveWhenValueIsGreaterThanThreshold)
{
    const dispatcher::alarm::ThresholdAlarmCondition condition(
        dispatcher::alarm::AlarmConditionType::High,
        80.0
    );

    const auto result = condition.evaluate(
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(81.0)
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_TRUE(result.active());
    EXPECT_FALSE(result.normal());
}

TEST(ThresholdAlarmConditionTests, HighIsNormalWhenValueEqualsThreshold)
{
    const dispatcher::alarm::ThresholdAlarmCondition condition(
        dispatcher::alarm::AlarmConditionType::High,
        80.0
    );

    const auto result = condition.evaluate(
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(80.0)
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_FALSE(result.active());
    EXPECT_TRUE(result.normal());
}

TEST(ThresholdAlarmConditionTests, HighHighBecomesActiveWhenValueIsGreaterThanThreshold)
{
    const dispatcher::alarm::ThresholdAlarmCondition condition(
        dispatcher::alarm::AlarmConditionType::HighHigh,
        90.0
    );

    const auto result = condition.evaluate(
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(91.0)
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_TRUE(result.active());
}

TEST(ThresholdAlarmConditionTests, LowBecomesActiveWhenValueIsLessThanThreshold)
{
    const dispatcher::alarm::ThresholdAlarmCondition condition(
        dispatcher::alarm::AlarmConditionType::Low,
        20.0
    );

    const auto result = condition.evaluate(
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(19.0)
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_TRUE(result.active());
    EXPECT_FALSE(result.normal());
}

TEST(ThresholdAlarmConditionTests, LowIsNormalWhenValueEqualsThreshold)
{
    const dispatcher::alarm::ThresholdAlarmCondition condition(
        dispatcher::alarm::AlarmConditionType::Low,
        20.0
    );

    const auto result = condition.evaluate(
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(20.0)
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_FALSE(result.active());
    EXPECT_TRUE(result.normal());
}

TEST(ThresholdAlarmConditionTests, LowLowBecomesActiveWhenValueIsLessThanThreshold)
{
    const dispatcher::alarm::ThresholdAlarmCondition condition(
        dispatcher::alarm::AlarmConditionType::LowLow,
        10.0
    );

    const auto result = condition.evaluate(
        make_telemetry_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(9.0)
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_TRUE(result.active());
}

TEST(ThresholdAlarmConditionTests, SupportsInt32Values)
{
    const dispatcher::alarm::ThresholdAlarmCondition condition(
        dispatcher::alarm::AlarmConditionType::High,
        10.0
    );

    const auto result = condition.evaluate(
        make_telemetry_value(
            "tag-counter",
            dispatcher::telemetry::TagValue(std::int32_t{ 11 })
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_TRUE(result.active());
}

TEST(ThresholdAlarmConditionTests, SupportsInt64Values)
{
    const dispatcher::alarm::ThresholdAlarmCondition condition(
        dispatcher::alarm::AlarmConditionType::High,
        10.0
    );

    const auto result = condition.evaluate(
        make_telemetry_value(
            "tag-counter",
            dispatcher::telemetry::TagValue(std::int64_t{ 11 })
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_TRUE(result.active());
}

TEST(ThresholdAlarmConditionTests, SupportsFloat32Values)
{
    const dispatcher::alarm::ThresholdAlarmCondition condition(
        dispatcher::alarm::AlarmConditionType::Low,
        10.0
    );

    const auto result = condition.evaluate(
        make_telemetry_value(
            "tag-level",
            dispatcher::telemetry::TagValue(float{ 9.5F })
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_TRUE(result.active());
}

TEST(ThresholdAlarmConditionTests, SupportsFloat64Values)
{
    const dispatcher::alarm::ThresholdAlarmCondition condition(
        dispatcher::alarm::AlarmConditionType::High,
        10.0
    );

    const auto result = condition.evaluate(
        make_telemetry_value(
            "tag-level",
            dispatcher::telemetry::TagValue(10.5)
        )
    );

    EXPECT_TRUE(result.evaluated());
    EXPECT_TRUE(result.active());
}

TEST(ThresholdAlarmConditionTests, RejectsBooleanValues)
{
    const dispatcher::alarm::ThresholdAlarmCondition condition(
        dispatcher::alarm::AlarmConditionType::High,
        10.0
    );

    const auto result = condition.evaluate(
        make_telemetry_value(
            "tag-running",
            dispatcher::telemetry::TagValue(true)
        )
    );

    EXPECT_FALSE(result.evaluated());
    EXPECT_FALSE(result.active());
    EXPECT_FALSE(result.normal());
    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::ThresholdAlarmEvaluationStatus::UnsupportedValueType
    );
}

TEST(ThresholdAlarmConditionTests, RejectsStringValues)
{
    const dispatcher::alarm::ThresholdAlarmCondition condition(
        dispatcher::alarm::AlarmConditionType::High,
        10.0
    );

    const auto result = condition.evaluate(
        make_telemetry_value(
            "tag-mode",
            dispatcher::telemetry::TagValue(std::string{ "auto" })
        )
    );

    EXPECT_FALSE(result.evaluated());
    EXPECT_FALSE(result.active());
    EXPECT_FALSE(result.normal());
    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::ThresholdAlarmEvaluationStatus::UnsupportedValueType
    );
}