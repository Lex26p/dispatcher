#include <dispatcher/alarm/alarm_definition.hpp>
#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_definition_validation.hpp>
#include <dispatcher/alarm/alarm_severity.hpp>
#include <dispatcher/alarm/alarm_state.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <gtest/gtest.h>

TEST(AlarmSeverityTests, ToStringReturnsExpectedValues)
{
    using dispatcher::alarm::AlarmSeverity;
    using dispatcher::alarm::to_string;

    EXPECT_EQ(to_string(AlarmSeverity::Info), "info");
    EXPECT_EQ(to_string(AlarmSeverity::Warning), "warning");
    EXPECT_EQ(to_string(AlarmSeverity::Critical), "critical");
}

TEST(AlarmStateTests, ToStringReturnsExpectedValues)
{
    using dispatcher::alarm::AlarmState;
    using dispatcher::alarm::to_string;

    EXPECT_EQ(to_string(AlarmState::Normal), "normal");
    EXPECT_EQ(to_string(AlarmState::Active), "active");
    EXPECT_EQ(to_string(AlarmState::Acknowledged), "acknowledged");
}

TEST(AlarmDefinitionTests, StoresAlarmDefinitionValues)
{
    const dispatcher::alarm::AlarmDefinition definition(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        dispatcher::domain::TagId{ "tag-temperature" },
        "temperature_high",
        "Temperature is too high",
        dispatcher::alarm::AlarmSeverity::Critical,
        true,
        5
    );

    EXPECT_EQ(
        definition.alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_EQ(
        definition.tag_id(),
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    EXPECT_EQ(definition.name(), "temperature_high");
    EXPECT_EQ(definition.description(), "Temperature is too high");
    EXPECT_EQ(definition.severity(), dispatcher::alarm::AlarmSeverity::Critical);
    EXPECT_TRUE(definition.enabled());
    EXPECT_EQ(definition.config_version(), 5);
}

TEST(AlarmDefinitionBuilderTests, BuildsAlarmDefinition)
{
    const auto definition = dispatcher::alarm::AlarmDefinitionBuilder{}
        .alarm_id(dispatcher::domain::AlarmId{ "alarm-temperature-high" })
        .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
        .name("temperature_high")
        .description("Temperature is too high")
        .severity(dispatcher::alarm::AlarmSeverity::Critical)
        .enabled(false)
        .config_version(7)
        .build();

    EXPECT_EQ(
        definition.alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_EQ(
        definition.tag_id(),
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    EXPECT_EQ(definition.name(), "temperature_high");
    EXPECT_EQ(definition.description(), "Temperature is too high");
    EXPECT_EQ(definition.severity(), dispatcher::alarm::AlarmSeverity::Critical);
    EXPECT_FALSE(definition.enabled());
    EXPECT_EQ(definition.config_version(), 7);
}

TEST(AlarmDefinitionBuilderTests, UsesDefaults)
{
    const auto definition = dispatcher::alarm::AlarmDefinitionBuilder{}
        .alarm_id(dispatcher::domain::AlarmId{ "alarm-temperature-high" })
        .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
        .name("temperature_high")
        .build();

    EXPECT_EQ(definition.severity(), dispatcher::alarm::AlarmSeverity::Warning);
    EXPECT_TRUE(definition.enabled());
    EXPECT_EQ(definition.config_version(), 1);
}

TEST(AlarmDefinitionValidationTests, ValidDefinitionPassesValidation)
{
    const auto definition = dispatcher::alarm::AlarmDefinitionBuilder{}
        .alarm_id(dispatcher::domain::AlarmId{ "alarm-temperature-high" })
        .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
        .name("temperature_high")
        .description("Temperature is too high")
        .severity(dispatcher::alarm::AlarmSeverity::Critical)
        .build();

    const auto result = dispatcher::alarm::validate_alarm_definition(definition);

    EXPECT_TRUE(result.valid());
    EXPECT_FALSE(result.has_errors());
}

TEST(AlarmDefinitionValidationTests, RejectsMissingAlarmId)
{
    const auto definition = dispatcher::alarm::AlarmDefinitionBuilder{}
        .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
        .name("temperature_high")
        .build();

    const auto result = dispatcher::alarm::validate_alarm_definition(definition);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "alarm_id");
}

TEST(AlarmDefinitionValidationTests, RejectsMissingTagId)
{
    const auto definition = dispatcher::alarm::AlarmDefinitionBuilder{}
        .alarm_id(dispatcher::domain::AlarmId{ "alarm-temperature-high" })
        .name("temperature_high")
        .build();

    const auto result = dispatcher::alarm::validate_alarm_definition(definition);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "tag_id");
}

TEST(AlarmDefinitionValidationTests, RejectsMissingName)
{
    const auto definition = dispatcher::alarm::AlarmDefinitionBuilder{}
        .alarm_id(dispatcher::domain::AlarmId{ "alarm-temperature-high" })
        .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
        .build();

    const auto result = dispatcher::alarm::validate_alarm_definition(definition);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "name");
}

TEST(AlarmDefinitionValidationTests, RejectsNameWithSlash)
{
    const auto definition = dispatcher::alarm::AlarmDefinitionBuilder{}
        .alarm_id(dispatcher::domain::AlarmId{ "alarm-temperature-high" })
        .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
        .name("temperature/high")
        .build();

    const auto result = dispatcher::alarm::validate_alarm_definition(definition);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "name");
}

TEST(AlarmDefinitionValidationTests, RejectsZeroConfigVersion)
{
    const auto definition = dispatcher::alarm::AlarmDefinitionBuilder{}
        .alarm_id(dispatcher::domain::AlarmId{ "alarm-temperature-high" })
        .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
        .name("temperature_high")
        .config_version(0)
        .build();

    const auto result = dispatcher::alarm::validate_alarm_definition(definition);

    ASSERT_FALSE(result.valid());
    ASSERT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "config_version");
}