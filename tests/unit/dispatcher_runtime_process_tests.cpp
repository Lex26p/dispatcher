#include <dispatcher/alarm/alarm_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_definition.hpp>
#include <dispatcher/alarm/alarm_condition_type.hpp>
#include <dispatcher/alarm/alarm_configuration_metadata.hpp>
#include <dispatcher/alarm/alarm_configuration_snapshot_builder.hpp>
#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_severity.hpp>
#include <dispatcher/alarm/threshold_alarm_condition.hpp>
#include <dispatcher/core/telemetry_ingest_status.hpp>
#include <dispatcher/domain/configuration_snapshot_builder.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/device_definition_builder.hpp>
#include <dispatcher/domain/history_policy.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/domain/tag_definition_builder.hpp>
#include <dispatcher/history/history_write_result.hpp>
#include <dispatcher/runtime/dispatcher_runtime.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
    dispatcher::domain::DeviceDefinition make_device()
    {
        return dispatcher::domain::DeviceDefinitionBuilder{}
            .device_id(dispatcher::domain::DeviceId{ "device-1" })
            .organization_id(dispatcher::domain::OrganizationId{ "org-1" })
            .site_id(dispatcher::domain::SiteId{ "site-1" })
            .area_id(dispatcher::domain::AreaId{ "area-1" })
            .local_name("device-1")
            .display_name("Device 1")
            .enabled(true)
            .build();
    }

    dispatcher::domain::TagDefinition make_tag(
        dispatcher::domain::HistoryPolicy history_policy =
        dispatcher::domain::HistoryPolicy::EveryPoll
    )
    {
        return dispatcher::domain::TagDefinitionBuilder{}
            .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
            .organization_id(dispatcher::domain::OrganizationId{ "org-1" })
            .site_id(dispatcher::domain::SiteId{ "site-1" })
            .area_id(dispatcher::domain::AreaId{ "area-1" })
            .device_id(dispatcher::domain::DeviceId{ "device-1" })
            .local_name("temperature")
            .display_name("Temperature")
            .data_type(dispatcher::domain::DataType::Float64)
            .history_policy(history_policy)
            .enabled(true)
            .build();
    }

    dispatcher::domain::ConfigurationSnapshot make_telemetry_configuration(
        dispatcher::domain::HistoryPolicy history_policy =
        dispatcher::domain::HistoryPolicy::EveryPoll
    )
    {
        dispatcher::domain::ConfigurationSnapshotBuilder builder;

        builder.config_version(7);
        builder.status(dispatcher::domain::ConfigurationStatus::Published);

        const auto device_result = builder.add_device(make_device());

        if (device_result.has_errors())
        {
            throw std::runtime_error(
                "failed to add device to telemetry configuration: "
                + device_result.errors().front().field
                + " - "
                + device_result.errors().front().message
            );
        }

        const auto tag_result = builder.add_tag(make_tag(history_policy));

        if (tag_result.has_errors())
        {
            throw std::runtime_error(
                "failed to add tag to telemetry configuration: "
                + tag_result.errors().front().field
                + " - "
                + tag_result.errors().front().message
            );
        }

        return builder.build();
    }

    dispatcher::alarm::AlarmDefinition make_alarm_definition()
    {
        return dispatcher::alarm::AlarmDefinitionBuilder{}
            .alarm_id(dispatcher::domain::AlarmId{ "alarm-temperature-low" })
            .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
            .name("temperature_low")
            .description("Temperature low alarm")
            .severity(dispatcher::alarm::AlarmSeverity::Warning)
            .enabled(true)
            .config_version(7)
            .build();
    }

    dispatcher::alarm::AlarmConditionDefinition make_alarm_condition_definition()
    {
        return dispatcher::alarm::AlarmConditionDefinition(
            dispatcher::domain::AlarmId{ "alarm-temperature-low" },
            dispatcher::alarm::ThresholdAlarmCondition(
                dispatcher::alarm::AlarmConditionType::Low,
                10.0
            )
        );
    }

    dispatcher::alarm::AlarmConfigurationSnapshot make_alarm_configuration()
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(7)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "runtime-process-alarms",
                    .description = "Runtime process alarm configuration",
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
            make_alarm_condition_definition()
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
            TagValue(static_cast<double>(value)),
            Quality::Good,
            now,
            now,
            sequence
        );
    }
}

TEST(DispatcherRuntimeProcessTests, ProcessAcceptedTelemetryWritesHistoryAndEvaluatesAlarm)
{
    dispatcher::runtime::DispatcherRuntime runtime(
        make_telemetry_configuration(),
        make_alarm_configuration()
    );

    ASSERT_TRUE(
        runtime.telemetry_ingestor()
        .configuration_snapshot()
        .find_tag_by_id(dispatcher::domain::TagId{ "tag-temperature" })
        .has_value()
    );

    const auto summary = runtime.process(
        make_telemetry_value(
            "tag-temperature",
            5.0,
            1
        )
    );

    ASSERT_EQ(
        summary.telemetry_status,
        dispatcher::core::TelemetryIngestStatus::Accepted
    ) << "Actual telemetry status: "
        << dispatcher::core::to_string(summary.telemetry_status);

    EXPECT_TRUE(summary.telemetry_accepted());
    EXPECT_TRUE(summary.telemetry_stored());
    EXPECT_FALSE(summary.telemetry_no_change());
    EXPECT_FALSE(summary.telemetry_rejected());

    EXPECT_EQ(
        summary.history_status,
        dispatcher::history::HistoryWriteStatus::Written
    );

    EXPECT_TRUE(summary.history_written());
    EXPECT_FALSE(summary.history_skipped());

    EXPECT_EQ(summary.configured_alarm_count, 1);
    EXPECT_EQ(summary.missing_condition_count, 0);

    EXPECT_EQ(summary.alarm_total_count, 1);
    EXPECT_EQ(summary.alarm_evaluated_count, 1);
    EXPECT_EQ(summary.alarm_skipped_count, 0);

    EXPECT_EQ(summary.alarm_activated_count, 1);
    EXPECT_EQ(summary.alarm_acknowledged_count, 0);
    EXPECT_EQ(summary.alarm_cleared_count, 0);
    EXPECT_EQ(summary.alarm_stored_event_count, 1);

    EXPECT_TRUE(summary.has_configured_alarms());
    EXPECT_FALSE(summary.has_missing_conditions());
    EXPECT_TRUE(summary.alarm_evaluated());
    EXPECT_FALSE(summary.alarm_skipped());
    EXPECT_TRUE(summary.has_alarm_events());
    EXPECT_TRUE(summary.has_alarm_transitions());
    EXPECT_TRUE(summary.successful());

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.telemetry.configuration_version, 7);
    EXPECT_EQ(snapshot.telemetry.current_state_size, 1);

    EXPECT_EQ(snapshot.history.store_size, 1);

    EXPECT_EQ(snapshot.alarm.configuration_version, 7);
    EXPECT_EQ(snapshot.alarm.configured_alarm_count, 1);
    EXPECT_EQ(snapshot.alarm.event_store_size, 1);

    EXPECT_EQ(snapshot.alarm_operator.configured_alarm_count, 1);
    EXPECT_EQ(snapshot.alarm_operator.active_alarm_count, 1);
    EXPECT_EQ(snapshot.alarm_operator.unacknowledged_alarm_count, 1);

    EXPECT_TRUE(snapshot.has_current_state());
    EXPECT_TRUE(snapshot.has_history_samples());
    EXPECT_TRUE(snapshot.has_alarm_events());
    EXPECT_FALSE(snapshot.has_acknowledgement_audit());
    EXPECT_TRUE(snapshot.requires_operator_attention());
}

TEST(DispatcherRuntimeProcessTests, ProcessRejectedTelemetrySkipsHistoryAndAlarmEvaluation)
{
    dispatcher::runtime::DispatcherRuntime runtime(
        make_telemetry_configuration(),
        make_alarm_configuration()
    );

    ASSERT_TRUE(
        runtime.telemetry_ingestor()
        .configuration_snapshot()
        .find_tag_by_id(dispatcher::domain::TagId{ "tag-temperature" })
        .has_value()
    );

    const auto summary = runtime.process(
        make_telemetry_value(
            "unknown-tag",
            5.0,
            1
        )
    );

    ASSERT_EQ(
        summary.telemetry_status,
        dispatcher::core::TelemetryIngestStatus::UnknownTag
    ) << "Actual telemetry status: "
        << dispatcher::core::to_string(summary.telemetry_status);

    EXPECT_FALSE(summary.telemetry_accepted());
    EXPECT_FALSE(summary.telemetry_stored());
    EXPECT_FALSE(summary.telemetry_no_change());
    EXPECT_TRUE(summary.telemetry_rejected());

    EXPECT_EQ(
        summary.history_status,
        dispatcher::history::HistoryWriteStatus::SkippedNotStored
    );

    EXPECT_FALSE(summary.history_written());
    EXPECT_TRUE(summary.history_skipped());

    EXPECT_EQ(summary.configured_alarm_count, 0);
    EXPECT_EQ(summary.missing_condition_count, 0);

    EXPECT_EQ(summary.alarm_total_count, 0);
    EXPECT_EQ(summary.alarm_evaluated_count, 0);
    EXPECT_EQ(summary.alarm_skipped_count, 0);

    EXPECT_EQ(summary.alarm_activated_count, 0);
    EXPECT_EQ(summary.alarm_acknowledged_count, 0);
    EXPECT_EQ(summary.alarm_cleared_count, 0);
    EXPECT_EQ(summary.alarm_stored_event_count, 0);

    EXPECT_FALSE(summary.has_configured_alarms());
    EXPECT_FALSE(summary.has_missing_conditions());
    EXPECT_FALSE(summary.alarm_evaluated());
    EXPECT_FALSE(summary.alarm_skipped());
    EXPECT_FALSE(summary.has_alarm_events());
    EXPECT_FALSE(summary.has_alarm_transitions());
    EXPECT_FALSE(summary.successful());

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.telemetry.current_state_size, 0);
    EXPECT_EQ(snapshot.history.store_size, 0);
    EXPECT_EQ(snapshot.alarm.event_store_size, 0);

    EXPECT_FALSE(snapshot.has_current_state());
    EXPECT_FALSE(snapshot.has_history_samples());
    EXPECT_FALSE(snapshot.has_alarm_events());
    EXPECT_FALSE(snapshot.requires_operator_attention());
}

TEST(DispatcherRuntimeProcessTests, LiveOnlyHistoryPolicySkipsHistoryButEvaluatesAlarm)
{
    dispatcher::runtime::DispatcherRuntime runtime(
        make_telemetry_configuration(
            dispatcher::domain::HistoryPolicy::LiveOnly
        ),
        make_alarm_configuration()
    );

    ASSERT_TRUE(
        runtime.telemetry_ingestor()
        .configuration_snapshot()
        .find_tag_by_id(dispatcher::domain::TagId{ "tag-temperature" })
        .has_value()
    );

    const auto summary = runtime.process(
        make_telemetry_value(
            "tag-temperature",
            5.0,
            1
        )
    );

    ASSERT_EQ(
        summary.telemetry_status,
        dispatcher::core::TelemetryIngestStatus::Accepted
    ) << "Actual telemetry status: "
        << dispatcher::core::to_string(summary.telemetry_status);

    EXPECT_TRUE(summary.telemetry_accepted());
    EXPECT_TRUE(summary.telemetry_stored());
    EXPECT_FALSE(summary.telemetry_no_change());
    EXPECT_FALSE(summary.telemetry_rejected());

    EXPECT_EQ(
        summary.history_status,
        dispatcher::history::HistoryWriteStatus::SkippedByPolicy
    );

    EXPECT_FALSE(summary.history_written());
    EXPECT_TRUE(summary.history_skipped());

    EXPECT_EQ(summary.configured_alarm_count, 1);
    EXPECT_EQ(summary.missing_condition_count, 0);

    EXPECT_EQ(summary.alarm_total_count, 1);
    EXPECT_EQ(summary.alarm_evaluated_count, 1);
    EXPECT_EQ(summary.alarm_skipped_count, 0);

    EXPECT_EQ(summary.alarm_activated_count, 1);
    EXPECT_EQ(summary.alarm_acknowledged_count, 0);
    EXPECT_EQ(summary.alarm_cleared_count, 0);
    EXPECT_EQ(summary.alarm_stored_event_count, 1);

    EXPECT_TRUE(summary.has_configured_alarms());
    EXPECT_FALSE(summary.has_missing_conditions());
    EXPECT_TRUE(summary.alarm_evaluated());
    EXPECT_FALSE(summary.alarm_skipped());
    EXPECT_TRUE(summary.has_alarm_events());
    EXPECT_TRUE(summary.has_alarm_transitions());
    EXPECT_TRUE(summary.successful());

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.telemetry.current_state_size, 1);
    EXPECT_EQ(snapshot.history.store_size, 0);
    EXPECT_EQ(snapshot.alarm.event_store_size, 1);

    EXPECT_TRUE(snapshot.has_current_state());
    EXPECT_FALSE(snapshot.has_history_samples());
    EXPECT_TRUE(snapshot.has_alarm_events());
    EXPECT_TRUE(snapshot.requires_operator_attention());
}