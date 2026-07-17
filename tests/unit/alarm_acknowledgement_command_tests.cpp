#include <dispatcher/alarm/alarm_acknowledgement_command.hpp>
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
            .description("Runtime acknowledgement command test alarm")
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
                    .name = "acknowledgement-command-alarms",
                    .description = "Acknowledgement command alarm configuration",
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

TEST(AlarmAcknowledgementCommandTests, StoresCommandValues)
{
    const dispatcher::alarm::AlarmAcknowledgementCommand command(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        "operator-1",
        "Checked process and acknowledged"
    );

    EXPECT_EQ(
        command.alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_EQ(command.operator_id(), "operator-1");
    EXPECT_EQ(command.comment(), "Checked process and acknowledged");
}

TEST(AlarmAcknowledgementCommandTests, RuntimeCanAcknowledgeUsingCommand)
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

    EXPECT_TRUE(result.acknowledged());

    EXPECT_EQ(
        runtime.state_store().state_of(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" }
        ),
        dispatcher::alarm::AlarmState::Acknowledged
    );

    ASSERT_TRUE(result.event().has_value());

    EXPECT_EQ(result.event()->sequence(), 1);
    EXPECT_EQ(runtime.event_store().size(), 2);
    EXPECT_EQ(runtime.statistics().acknowledged_count(), 1);
}

TEST(AlarmAcknowledgementCommandTests, CommandKeepsExistingAcknowledgeRules)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(runtime.reload_configuration(make_snapshot()).valid());

    const dispatcher::alarm::AlarmAcknowledgementCommand command(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        "operator-1",
        "Trying to acknowledge inactive alarm"
    );

    const auto result = runtime.acknowledge(command);

    EXPECT_TRUE(result.skipped());
    EXPECT_TRUE(result.not_active());

    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_TRUE(runtime.event_store().empty());
    EXPECT_EQ(runtime.statistics().acknowledged_count(), 0);
}