#include <dispatcher/alarm/alarm_acknowledgement_command.hpp>
#include <dispatcher/alarm/alarm_acknowledgement_command_validation.hpp>
#include <dispatcher/alarm/alarm_acknowledgement_result.hpp>
#include <dispatcher/alarm/alarm_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_definition.hpp>
#include <dispatcher/alarm/alarm_condition_type.hpp>
#include <dispatcher/alarm/alarm_configuration_metadata.hpp>
#include <dispatcher/alarm/alarm_configuration_snapshot_builder.hpp>
#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_runtime.hpp>
#include <dispatcher/alarm/alarm_severity.hpp>
#include <dispatcher/alarm/alarm_state.hpp>
#include <dispatcher/alarm/threshold_alarm_condition.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace
{
    dispatcher::alarm::AlarmDefinition make_alarm_definition()
    {
        return dispatcher::alarm::AlarmDefinitionBuilder{}
            .alarm_id(dispatcher::domain::AlarmId{ "alarm-temperature-high" })
            .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
            .name("temperature_high")
            .description("Acknowledgement command validation test alarm")
            .severity(dispatcher::alarm::AlarmSeverity::Warning)
            .enabled(true)
            .config_version(1)
            .build();
    }

    dispatcher::alarm::AlarmConditionDefinition make_condition_definition()
    {
        return dispatcher::alarm::AlarmConditionDefinition(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" },
            dispatcher::alarm::ThresholdAlarmCondition(
                dispatcher::alarm::AlarmConditionType::High,
                80.0
            )
        );
    }

    dispatcher::alarm::AlarmConfigurationSnapshot make_snapshot()
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(7)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "acknowledgement-command-validation-alarms",
                    .description = "Acknowledgement command validation configuration",
                    .created_by = "unit-test"
                }
            )
            .status(dispatcher::domain::ConfigurationStatus::Published)
            .alarm_catalog(
                dispatcher::alarm::AlarmCatalog(
                    std::vector<dispatcher::alarm::AlarmDefinition>{
            make_alarm_definition()
        }
                )
            )
            .condition_catalog(
                dispatcher::alarm::AlarmConditionCatalog(
                    std::vector<dispatcher::alarm::AlarmConditionDefinition>{
            make_condition_definition()
        }
                )
            )
            .build();
    }
}

TEST(AlarmAcknowledgementCommandValidationTests, ValidCommandPassesValidation)
{
    const dispatcher::alarm::AlarmAcknowledgementCommand command(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        "operator-1",
        "Checked process"
    );

    const auto result =
        dispatcher::alarm::validate_alarm_acknowledgement_command(command);

    EXPECT_TRUE(result.valid());
}

TEST(AlarmAcknowledgementCommandValidationTests, EmptyAlarmIdFailsValidation)
{
    const dispatcher::alarm::AlarmAcknowledgementCommand command(
        dispatcher::domain::AlarmId{ "" },
        "operator-1",
        "Checked process"
    );

    const auto result =
        dispatcher::alarm::validate_alarm_acknowledgement_command(command);

    ASSERT_FALSE(result.valid());
    ASSERT_EQ(result.errors().size(), 1);

    EXPECT_EQ(result.errors()[0].field, "alarm_id");
}

TEST(AlarmAcknowledgementCommandValidationTests, EmptyOperatorIdFailsValidation)
{
    const dispatcher::alarm::AlarmAcknowledgementCommand command(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        "",
        "Checked process"
    );

    const auto result =
        dispatcher::alarm::validate_alarm_acknowledgement_command(command);

    ASSERT_FALSE(result.valid());
    ASSERT_EQ(result.errors().size(), 1);

    EXPECT_EQ(result.errors()[0].field, "operator_id");
}

TEST(AlarmAcknowledgementCommandValidationTests, EmptyCommentIsAllowed)
{
    const dispatcher::alarm::AlarmAcknowledgementCommand command(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        "operator-1",
        ""
    );

    const auto result =
        dispatcher::alarm::validate_alarm_acknowledgement_command(command);

    EXPECT_TRUE(result.valid());
}

TEST(AlarmAcknowledgementCommandValidationTests, RuntimeRejectsInvalidCommandAndWritesAuditRecord)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(runtime.reload_configuration(make_snapshot()).valid());

    const dispatcher::alarm::AlarmAcknowledgementCommand command(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        "",
        "Missing operator"
    );

    const auto result = runtime.acknowledge(command);

    EXPECT_TRUE(result.skipped());
    EXPECT_TRUE(result.invalid_command());

    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::AlarmAcknowledgementStatus::InvalidCommand
    );

    EXPECT_EQ(
        dispatcher::alarm::to_string(result.status()),
        "invalid_command"
    );

    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_TRUE(runtime.event_store().empty());
    EXPECT_EQ(runtime.statistics().total_count(), 0);
    EXPECT_EQ(runtime.statistics().acknowledged_count(), 0);

    ASSERT_EQ(runtime.acknowledgement_store().size(), 1);

    const auto& record = runtime.acknowledgement_store().records()[0];

    EXPECT_EQ(
        record.status(),
        dispatcher::alarm::AlarmAcknowledgementStatus::InvalidCommand
    );

    EXPECT_EQ(
        record.alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_EQ(record.operator_id(), "");
    EXPECT_EQ(record.comment(), "Missing operator");
    EXPECT_FALSE(record.event_sequence().has_value());
}