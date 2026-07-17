#include <dispatcher/alarm/alarm_acknowledgement_command.hpp>
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
    dispatcher::alarm::AlarmDefinition make_alarm_definition()
    {
        return dispatcher::alarm::AlarmDefinitionBuilder{}
            .alarm_id(dispatcher::domain::AlarmId{ "alarm-temperature-high" })
            .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
            .name("temperature_high")
            .description("Runtime acknowledgement audit test alarm")
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
                    .name = "acknowledgement-audit-alarms",
                    .description = "Acknowledgement audit alarm configuration",
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

TEST(AlarmRuntimeAcknowledgementAuditTests, CommandAcknowledgementWritesAuditRecord)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(runtime.reload_configuration(make_snapshot()).valid());

    const auto activate_result = runtime.evaluate_configured(
        make_telemetry_value(81.0, 1)
    );

    ASSERT_EQ(activate_result.batch_result().activated_count(), 1);

    const dispatcher::alarm::AlarmAcknowledgementCommand command(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        "operator-1",
        "Checked process and acknowledged"
    );

    const auto result = runtime.acknowledge(command);

    ASSERT_TRUE(result.acknowledged());

    ASSERT_EQ(runtime.acknowledgement_store().size(), 1);

    const auto& record = runtime.acknowledgement_store().records()[0];

    EXPECT_EQ(
        record.alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_EQ(record.operator_id(), "operator-1");
    EXPECT_EQ(record.comment(), "Checked process and acknowledged");

    EXPECT_EQ(
        record.status(),
        dispatcher::alarm::AlarmAcknowledgementStatus::Acknowledged
    );

    EXPECT_TRUE(record.acknowledged());
    EXPECT_FALSE(record.skipped());

    EXPECT_EQ(record.previous_state(), dispatcher::alarm::AlarmState::Active);

    EXPECT_EQ(
        record.new_state(),
        dispatcher::alarm::AlarmState::Acknowledged
    );

    ASSERT_TRUE(record.event_sequence().has_value());
    EXPECT_EQ(*record.event_sequence(), 1);

    EXPECT_EQ(runtime.runtime_snapshot().acknowledgement_store_size, 1);
}

TEST(AlarmRuntimeAcknowledgementAuditTests, NotActiveCommandWritesAuditRecord)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(runtime.reload_configuration(make_snapshot()).valid());

    const dispatcher::alarm::AlarmAcknowledgementCommand command(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        "operator-2",
        "Trying to acknowledge inactive alarm"
    );

    const auto result = runtime.acknowledge(command);

    ASSERT_TRUE(result.not_active());

    ASSERT_EQ(runtime.acknowledgement_store().size(), 1);

    const auto& record = runtime.acknowledgement_store().records()[0];

    EXPECT_EQ(
        record.status(),
        dispatcher::alarm::AlarmAcknowledgementStatus::NotActive
    );

    EXPECT_TRUE(record.skipped());
    EXPECT_EQ(record.operator_id(), "operator-2");
    EXPECT_EQ(record.comment(), "Trying to acknowledge inactive alarm");
    EXPECT_EQ(record.previous_state(), dispatcher::alarm::AlarmState::Normal);
    EXPECT_EQ(record.new_state(), dispatcher::alarm::AlarmState::Normal);
    EXPECT_FALSE(record.event_sequence().has_value());

    EXPECT_TRUE(runtime.event_store().empty());
    EXPECT_EQ(runtime.statistics().acknowledged_count(), 0);
}

TEST(AlarmRuntimeAcknowledgementAuditTests, UnknownAlarmCommandWritesAuditRecord)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(runtime.reload_configuration(make_snapshot()).valid());

    const dispatcher::alarm::AlarmAcknowledgementCommand command(
        dispatcher::domain::AlarmId{ "unknown-alarm" },
        "operator-3",
        "Wrong alarm id"
    );

    const auto result = runtime.acknowledge(command);

    ASSERT_TRUE(result.unknown_alarm());

    ASSERT_EQ(runtime.acknowledgement_store().size(), 1);

    const auto& record = runtime.acknowledgement_store().records()[0];

    EXPECT_EQ(
        record.status(),
        dispatcher::alarm::AlarmAcknowledgementStatus::UnknownAlarm
    );

    EXPECT_EQ(
        record.alarm_id(),
        dispatcher::domain::AlarmId{ "unknown-alarm" }
    );

    EXPECT_TRUE(record.skipped());
    EXPECT_EQ(record.operator_id(), "operator-3");
    EXPECT_EQ(record.comment(), "Wrong alarm id");
    EXPECT_FALSE(record.event_sequence().has_value());

    EXPECT_TRUE(runtime.event_store().empty());
}

TEST(AlarmRuntimeAcknowledgementAuditTests, AlreadyAcknowledgedCommandWritesAuditRecord)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(runtime.reload_configuration(make_snapshot()).valid());

    ASSERT_EQ(
        runtime.evaluate_configured(
            make_telemetry_value(81.0, 1)
        ).batch_result().activated_count(),
        1
    );

    const dispatcher::alarm::AlarmAcknowledgementCommand first_command(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        "operator-1",
        "First acknowledgement"
    );

    ASSERT_TRUE(runtime.acknowledge(first_command).acknowledged());

    const dispatcher::alarm::AlarmAcknowledgementCommand second_command(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        "operator-2",
        "Duplicate acknowledgement"
    );

    const auto result = runtime.acknowledge(second_command);

    ASSERT_TRUE(result.already_acknowledged());

    ASSERT_EQ(runtime.acknowledgement_store().size(), 2);

    const auto& record = runtime.acknowledgement_store().records()[1];

    EXPECT_EQ(
        record.status(),
        dispatcher::alarm::AlarmAcknowledgementStatus::AlreadyAcknowledged
    );

    EXPECT_EQ(record.operator_id(), "operator-2");
    EXPECT_EQ(record.comment(), "Duplicate acknowledgement");

    EXPECT_EQ(
        record.previous_state(),
        dispatcher::alarm::AlarmState::Acknowledged
    );

    EXPECT_EQ(
        record.new_state(),
        dispatcher::alarm::AlarmState::Acknowledged
    );

    EXPECT_FALSE(record.event_sequence().has_value());

    EXPECT_EQ(runtime.event_store().size(), 2);
    EXPECT_EQ(runtime.statistics().acknowledged_count(), 1);
}

TEST(AlarmRuntimeAcknowledgementAuditTests, InvalidCommandWritesAuditRecord)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(runtime.reload_configuration(make_snapshot()).valid());

    const dispatcher::alarm::AlarmAcknowledgementCommand command(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        "",
        "Missing operator id"
    );

    const auto result = runtime.acknowledge(command);

    ASSERT_TRUE(result.invalid_command());

    ASSERT_EQ(runtime.acknowledgement_store().size(), 1);

    const auto& record = runtime.acknowledgement_store().records()[0];

    EXPECT_EQ(
        record.status(),
        dispatcher::alarm::AlarmAcknowledgementStatus::InvalidCommand
    );

    EXPECT_EQ(record.operator_id(), "");
    EXPECT_EQ(record.comment(), "Missing operator id");
    EXPECT_TRUE(record.skipped());
    EXPECT_FALSE(record.event_sequence().has_value());

    EXPECT_TRUE(runtime.event_store().empty());
    EXPECT_EQ(runtime.statistics().acknowledged_count(), 0);
}