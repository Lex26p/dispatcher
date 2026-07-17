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
    dispatcher::alarm::AlarmDefinition make_alarm_definition(
        std::string alarm_id,
        std::string tag_id,
        std::string name
    )
    {
        return dispatcher::alarm::AlarmDefinitionBuilder{}
            .alarm_id(dispatcher::domain::AlarmId{ std::move(alarm_id) })
            .tag_id(dispatcher::domain::TagId{ std::move(tag_id) })
            .name(std::move(name))
            .description("Runtime state query test alarm")
            .severity(dispatcher::alarm::AlarmSeverity::Warning)
            .enabled(true)
            .config_version(1)
            .build();
    }

    dispatcher::alarm::AlarmConditionDefinition make_condition_definition(
        std::string alarm_id,
        dispatcher::alarm::AlarmConditionType condition_type,
        double threshold
    )
    {
        return dispatcher::alarm::AlarmConditionDefinition(
            dispatcher::domain::AlarmId{ std::move(alarm_id) },
            dispatcher::alarm::ThresholdAlarmCondition(
                condition_type,
                threshold
            )
        );
    }

    dispatcher::alarm::AlarmConfigurationSnapshot make_two_alarm_snapshot()
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(7)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "state-query-alarms",
                    .description = "State query alarm configuration",
                    .created_by = "unit-test"
                }
            )
            .status(dispatcher::domain::ConfigurationStatus::Published)
            .alarm_catalog(
                dispatcher::alarm::AlarmCatalog(
                    std::vector<dispatcher::alarm::AlarmDefinition>{
            make_alarm_definition(
                "alarm-temperature-high",
                "tag-temperature",
                "temperature_high"
            ),
                make_alarm_definition(
                    "alarm-pressure-high",
                    "tag-pressure",
                    "pressure_high"
                )
        }
                )
            )
            .condition_catalog(
                dispatcher::alarm::AlarmConditionCatalog(
                    std::vector<dispatcher::alarm::AlarmConditionDefinition>{
            make_condition_definition(
                "alarm-temperature-high",
                dispatcher::alarm::AlarmConditionType::High,
                80.0
            ),
                make_condition_definition(
                    "alarm-pressure-high",
                    dispatcher::alarm::AlarmConditionType::High,
                    10.0
                )
        }
                )
            )
            .build();
    }

    dispatcher::alarm::AlarmConfigurationSnapshot make_temperature_only_snapshot()
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(8)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "state-query-temperature-only",
                    .description = "State query temperature only alarm configuration",
                    .created_by = "unit-test"
                }
            )
            .status(dispatcher::domain::ConfigurationStatus::Published)
            .alarm_catalog(
                dispatcher::alarm::AlarmCatalog(
                    std::vector<dispatcher::alarm::AlarmDefinition>{
            make_alarm_definition(
                "alarm-temperature-high",
                "tag-temperature",
                "temperature_high"
            )
        }
                )
            )
            .condition_catalog(
                dispatcher::alarm::AlarmConditionCatalog(
                    std::vector<dispatcher::alarm::AlarmConditionDefinition>{
            make_condition_definition(
                "alarm-temperature-high",
                dispatcher::alarm::AlarmConditionType::High,
                80.0
            )
        }
                )
            )
            .build();
    }

    dispatcher::telemetry::TelemetryValue make_telemetry_value(
        std::string tag_id,
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
            TagId{ std::move(tag_id) },
            TagValue(value),
            Quality::Good,
            now,
            now,
            sequence
        );
    }
}

TEST(AlarmRuntimeStateQueryTests, PublishedConfiguredAlarmsStartAsNormal)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(
        runtime.reload_configuration(
            make_two_alarm_snapshot()
        ).valid()
    );

    EXPECT_EQ(runtime.normal_alarms().size(), 2);
    EXPECT_EQ(runtime.active_alarms().size(), 0);
    EXPECT_EQ(runtime.unacknowledged_alarms().size(), 0);
    EXPECT_EQ(runtime.acknowledged_alarms().size(), 0);

    EXPECT_EQ(
        runtime.alarms_by_state(dispatcher::alarm::AlarmState::Normal).size(),
        2
    );

    EXPECT_EQ(
        runtime.alarms_by_state(dispatcher::alarm::AlarmState::Active).size(),
        0
    );

    EXPECT_EQ(
        runtime.alarms_by_state(
            dispatcher::alarm::AlarmState::Acknowledged
        ).size(),
        0
    );
}

TEST(AlarmRuntimeStateQueryTests, ActiveAlarmAppearsAsActiveAndUnacknowledged)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(
        runtime.reload_configuration(
            make_two_alarm_snapshot()
        ).valid()
    );

    const auto result = runtime.evaluate_configured(
        make_telemetry_value(
            "tag-temperature",
            81.0,
            1
        )
    );

    ASSERT_EQ(result.batch_result().activated_count(), 1);

    const auto active_alarms = runtime.active_alarms();
    const auto unacknowledged_alarms = runtime.unacknowledged_alarms();
    const auto normal_alarms = runtime.normal_alarms();

    ASSERT_EQ(active_alarms.size(), 1);
    ASSERT_EQ(unacknowledged_alarms.size(), 1);
    ASSERT_EQ(normal_alarms.size(), 1);

    EXPECT_EQ(
        active_alarms[0].alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_EQ(
        unacknowledged_alarms[0].alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_EQ(
        normal_alarms[0].alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-pressure-high" }
    );

    EXPECT_EQ(runtime.acknowledged_alarms().size(), 0);
}

TEST(AlarmRuntimeStateQueryTests, AcknowledgedAlarmMovesOutOfUnacknowledgedQuery)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(
        runtime.reload_configuration(
            make_two_alarm_snapshot()
        ).valid()
    );

    ASSERT_EQ(
        runtime.evaluate_configured(
            make_telemetry_value(
                "tag-temperature",
                81.0,
                1
            )
        ).batch_result().activated_count(),
        1
    );

    const dispatcher::alarm::AlarmAcknowledgementCommand command(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" },
        "operator-1",
        "Checked process"
    );

    ASSERT_TRUE(runtime.acknowledge(command).acknowledged());

    EXPECT_EQ(runtime.active_alarms().size(), 0);
    EXPECT_EQ(runtime.unacknowledged_alarms().size(), 0);

    const auto acknowledged_alarms = runtime.acknowledged_alarms();

    ASSERT_EQ(acknowledged_alarms.size(), 1);

    EXPECT_EQ(
        acknowledged_alarms[0].alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_EQ(runtime.normal_alarms().size(), 1);
}

TEST(AlarmRuntimeStateQueryTests, AcknowledgedAlarmReturnsToNormalAfterClear)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(
        runtime.reload_configuration(
            make_two_alarm_snapshot()
        ).valid()
    );

    ASSERT_EQ(
        runtime.evaluate_configured(
            make_telemetry_value(
                "tag-temperature",
                81.0,
                1
            )
        ).batch_result().activated_count(),
        1
    );

    ASSERT_TRUE(
        runtime.acknowledge(
            dispatcher::alarm::AlarmAcknowledgementCommand(
                dispatcher::domain::AlarmId{ "alarm-temperature-high" },
                "operator-1",
                "Checked process"
            )
        ).acknowledged()
    );

    ASSERT_EQ(runtime.acknowledged_alarms().size(), 1);

    const auto clear_result = runtime.evaluate_configured(
        make_telemetry_value(
            "tag-temperature",
            79.0,
            2
        )
    );

    ASSERT_EQ(clear_result.batch_result().cleared_count(), 1);

    EXPECT_EQ(runtime.active_alarms().size(), 0);
    EXPECT_EQ(runtime.unacknowledged_alarms().size(), 0);
    EXPECT_EQ(runtime.acknowledged_alarms().size(), 0);
    EXPECT_EQ(runtime.normal_alarms().size(), 2);
}

TEST(AlarmRuntimeStateQueryTests, QueriesFollowConfigurationReloadPrune)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(
        runtime.reload_configuration(
            make_two_alarm_snapshot()
        ).valid()
    );

    const auto activate_result = runtime.evaluate_configured_batch(
        std::vector<dispatcher::telemetry::TelemetryValue>{
        make_telemetry_value(
            "tag-temperature",
            81.0,
            1
        ),
            make_telemetry_value(
                "tag-pressure",
                11.0,
                2
            )
    }
    );

    ASSERT_EQ(activate_result.batch_result().activated_count(), 2);
    ASSERT_EQ(runtime.active_alarms().size(), 2);

    ASSERT_TRUE(
        runtime.reload_configuration(
            make_temperature_only_snapshot()
        ).valid()
    );

    const auto active_alarms = runtime.active_alarms();

    ASSERT_EQ(active_alarms.size(), 1);

    EXPECT_EQ(
        active_alarms[0].alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_EQ(runtime.unacknowledged_alarms().size(), 1);
    EXPECT_EQ(runtime.normal_alarms().size(), 0);
    EXPECT_EQ(runtime.acknowledged_alarms().size(), 0);
}