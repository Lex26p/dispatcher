#include <dispatcher/alarm/alarm_acknowledgement_command.hpp>
#include <dispatcher/alarm/alarm_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_definition.hpp>
#include <dispatcher/alarm/alarm_condition_type.hpp>
#include <dispatcher/alarm/alarm_configuration_metadata.hpp>
#include <dispatcher/alarm/alarm_configuration_snapshot_builder.hpp>
#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_operator_snapshot.hpp>
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
            .description("Operator snapshot test alarm")
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
                    .name = "operator-snapshot-alarms",
                    .description = "Operator snapshot alarm configuration",
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

TEST(AlarmOperatorSnapshotTests, DefaultSnapshotHelpersWork)
{
    const dispatcher::alarm::AlarmOperatorSnapshot snapshot;

    EXPECT_EQ(snapshot.active_or_acknowledged_alarm_count(), 0);
    EXPECT_FALSE(snapshot.has_configured_alarms());
    EXPECT_FALSE(snapshot.has_active_alarms());
    EXPECT_FALSE(snapshot.has_acknowledged_alarms());
    EXPECT_FALSE(snapshot.has_unacknowledged_alarms());
    EXPECT_FALSE(snapshot.has_active_or_acknowledged_alarms());
    EXPECT_FALSE(snapshot.requires_operator_attention());
}

TEST(AlarmOperatorSnapshotTests, ReloadedConfigurationStartsAsNormal)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(
        runtime.reload_configuration(
            make_two_alarm_snapshot()
        ).valid()
    );

    const auto snapshot = runtime.operator_snapshot();

    EXPECT_EQ(snapshot.configuration_version, 7);
    EXPECT_EQ(snapshot.configured_alarm_count, 2);

    EXPECT_EQ(snapshot.normal_alarm_count, 2);
    EXPECT_EQ(snapshot.active_alarm_count, 0);
    EXPECT_EQ(snapshot.acknowledged_alarm_count, 0);
    EXPECT_EQ(snapshot.unacknowledged_alarm_count, 0);

    EXPECT_EQ(snapshot.event_store_size, 0);
    EXPECT_EQ(snapshot.acknowledgement_store_size, 0);

    EXPECT_TRUE(snapshot.has_configured_alarms());
    EXPECT_FALSE(snapshot.requires_operator_attention());
}

TEST(AlarmOperatorSnapshotTests, ActiveAlarmRequiresOperatorAttention)
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

    const auto snapshot = runtime.operator_snapshot();

    EXPECT_EQ(snapshot.normal_alarm_count, 1);
    EXPECT_EQ(snapshot.active_alarm_count, 1);
    EXPECT_EQ(snapshot.acknowledged_alarm_count, 0);
    EXPECT_EQ(snapshot.unacknowledged_alarm_count, 1);

    EXPECT_EQ(snapshot.active_or_acknowledged_alarm_count(), 1);

    EXPECT_EQ(snapshot.event_store_size, 1);
    EXPECT_EQ(snapshot.acknowledgement_store_size, 0);

    EXPECT_EQ(snapshot.activated_count, 1);
    EXPECT_EQ(snapshot.acknowledged_count, 0);
    EXPECT_EQ(snapshot.cleared_count, 0);

    EXPECT_TRUE(snapshot.has_active_alarms());
    EXPECT_TRUE(snapshot.has_unacknowledged_alarms());
    EXPECT_TRUE(snapshot.has_active_or_acknowledged_alarms());
    EXPECT_TRUE(snapshot.requires_operator_attention());
}

TEST(AlarmOperatorSnapshotTests, AcknowledgedAlarmDoesNotRequireAttention)
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

    const auto snapshot = runtime.operator_snapshot();

    EXPECT_EQ(snapshot.normal_alarm_count, 1);
    EXPECT_EQ(snapshot.active_alarm_count, 0);
    EXPECT_EQ(snapshot.acknowledged_alarm_count, 1);
    EXPECT_EQ(snapshot.unacknowledged_alarm_count, 0);

    EXPECT_EQ(snapshot.active_or_acknowledged_alarm_count(), 1);

    EXPECT_EQ(snapshot.event_store_size, 2);
    EXPECT_EQ(snapshot.acknowledgement_store_size, 1);

    EXPECT_EQ(snapshot.activated_count, 1);
    EXPECT_EQ(snapshot.acknowledged_count, 1);
    EXPECT_EQ(snapshot.cleared_count, 0);

    EXPECT_FALSE(snapshot.has_active_alarms());
    EXPECT_TRUE(snapshot.has_acknowledged_alarms());
    EXPECT_FALSE(snapshot.has_unacknowledged_alarms());
    EXPECT_TRUE(snapshot.has_active_or_acknowledged_alarms());
    EXPECT_FALSE(snapshot.requires_operator_attention());
}

TEST(AlarmOperatorSnapshotTests, ClearedAlarmReturnsToNormalSnapshot)
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

    const auto clear_result = runtime.evaluate_configured(
        make_telemetry_value(
            "tag-temperature",
            79.0,
            2
        )
    );

    ASSERT_EQ(clear_result.batch_result().cleared_count(), 1);

    const auto snapshot = runtime.operator_snapshot();

    EXPECT_EQ(snapshot.normal_alarm_count, 2);
    EXPECT_EQ(snapshot.active_alarm_count, 0);
    EXPECT_EQ(snapshot.acknowledged_alarm_count, 0);
    EXPECT_EQ(snapshot.unacknowledged_alarm_count, 0);

    EXPECT_EQ(snapshot.active_or_acknowledged_alarm_count(), 0);

    EXPECT_EQ(snapshot.event_store_size, 3);
    EXPECT_EQ(snapshot.acknowledgement_store_size, 1);

    EXPECT_EQ(snapshot.activated_count, 1);
    EXPECT_EQ(snapshot.acknowledged_count, 1);
    EXPECT_EQ(snapshot.cleared_count, 1);

    EXPECT_FALSE(snapshot.has_active_or_acknowledged_alarms());
    EXPECT_FALSE(snapshot.requires_operator_attention());
}

TEST(AlarmOperatorSnapshotTests, DefaultSnapshotHasNoSuppression)
{
    const dispatcher::alarm::AlarmOperatorSnapshot snapshot;

    EXPECT_EQ(snapshot.suppression_store_size, 0);
    EXPECT_EQ(snapshot.suppressed_alarm_count, 0);
    EXPECT_EQ(snapshot.shelved_alarm_count, 0);
    EXPECT_EQ(snapshot.inhibited_alarm_count, 0);
    EXPECT_EQ(snapshot.operator_controlled_suppression_count, 0);
    EXPECT_EQ(snapshot.system_controlled_suppression_count, 0);

    EXPECT_FALSE(snapshot.has_any_suppression());
    EXPECT_FALSE(snapshot.has_suppressed_alarms());
    EXPECT_FALSE(snapshot.has_shelved_alarms());
    EXPECT_FALSE(snapshot.has_inhibited_alarms());
}