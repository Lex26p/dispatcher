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
    dispatcher::domain::DeviceDefinition make_batch_device()
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

    dispatcher::domain::TagDefinition make_batch_tag(
        std::string tag_id,
        std::string local_name,
        dispatcher::domain::DataType data_type,
        dispatcher::domain::HistoryPolicy history_policy
    )
    {
        return dispatcher::domain::TagDefinitionBuilder{}
            .tag_id(dispatcher::domain::TagId{ std::move(tag_id) })
            .organization_id(dispatcher::domain::OrganizationId{ "org-1" })
            .site_id(dispatcher::domain::SiteId{ "site-1" })
            .area_id(dispatcher::domain::AreaId{ "area-1" })
            .device_id(dispatcher::domain::DeviceId{ "device-1" })
            .local_name(std::move(local_name))
            .display_name("Batch tag")
            .data_type(data_type)
            .history_policy(history_policy)
            .enabled(true)
            .build();
    }

    dispatcher::domain::ConfigurationSnapshot make_batch_telemetry_configuration(
        dispatcher::domain::HistoryPolicy history_policy =
        dispatcher::domain::HistoryPolicy::EveryPoll
    )
    {
        dispatcher::domain::ConfigurationSnapshotBuilder builder;

        builder.config_version(7);
        builder.status(dispatcher::domain::ConfigurationStatus::Published);

        const auto device_result = builder.add_device(make_batch_device());

        if (device_result.has_errors())
        {
            throw std::runtime_error(
                "failed to add device to telemetry configuration: "
                + device_result.errors().front().field
                + " - "
                + device_result.errors().front().message
            );
        }

        const auto temperature_tag_result = builder.add_tag(
            make_batch_tag(
                "tag-temperature",
                "temperature",
                dispatcher::domain::DataType::Float64,
                history_policy
            )
        );

        if (temperature_tag_result.has_errors())
        {
            throw std::runtime_error(
                "failed to add temperature tag to telemetry configuration: "
                + temperature_tag_result.errors().front().field
                + " - "
                + temperature_tag_result.errors().front().message
            );
        }

        const auto pressure_tag_result = builder.add_tag(
            make_batch_tag(
                "tag-pressure",
                "pressure",
                dispatcher::domain::DataType::Float64,
                history_policy
            )
        );

        if (pressure_tag_result.has_errors())
        {
            throw std::runtime_error(
                "failed to add pressure tag to telemetry configuration: "
                + pressure_tag_result.errors().front().field
                + " - "
                + pressure_tag_result.errors().front().message
            );
        }

        return builder.build();
    }

    dispatcher::alarm::AlarmDefinition make_batch_alarm_definition(
        std::string alarm_id,
        std::string tag_id,
        std::string name
    )
    {
        return dispatcher::alarm::AlarmDefinitionBuilder{}
            .alarm_id(dispatcher::domain::AlarmId{ std::move(alarm_id) })
            .tag_id(dispatcher::domain::TagId{ std::move(tag_id) })
            .name(std::move(name))
            .description("Batch alarm")
            .severity(dispatcher::alarm::AlarmSeverity::Warning)
            .enabled(true)
            .config_version(7)
            .build();
    }

    dispatcher::alarm::AlarmConditionDefinition make_batch_alarm_condition_definition(
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

    dispatcher::alarm::AlarmConfigurationSnapshot make_batch_alarm_configuration()
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(7)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "runtime-process-batch-alarms",
                    .description = "Runtime process batch alarm configuration",
                    .created_by = "unit-test"
                }
            )
            .status(dispatcher::domain::ConfigurationStatus::Published)
            .alarm_catalog(
                dispatcher::alarm::AlarmCatalog(
                    std::vector<dispatcher::alarm::AlarmDefinition>{
            make_batch_alarm_definition(
                "alarm-temperature-low",
                "tag-temperature",
                "temperature_low"
            ),
                make_batch_alarm_definition(
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
            make_batch_alarm_condition_definition(
                "alarm-temperature-low",
                dispatcher::alarm::AlarmConditionType::Low,
                10.0
            ),
                make_batch_alarm_condition_definition(
                    "alarm-pressure-high",
                    dispatcher::alarm::AlarmConditionType::High,
                    100.0
                )
        }
                )
            )
            .build();
    }

    dispatcher::telemetry::TelemetryValue make_batch_telemetry_value(
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

TEST(DispatcherRuntimeProcessBatchTests, EmptyBatchReturnsEmptySummary)
{
    dispatcher::runtime::DispatcherRuntime runtime(
        make_batch_telemetry_configuration(),
        make_batch_alarm_configuration()
    );

    const auto summary = runtime.process_batch({});

    EXPECT_TRUE(summary.empty());
    EXPECT_EQ(summary.total_count, 0);
    EXPECT_FALSE(summary.successful());

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.telemetry.current_state_size, 0);
    EXPECT_EQ(snapshot.history.store_size, 0);
    EXPECT_EQ(snapshot.alarm.event_store_size, 0);
}

TEST(DispatcherRuntimeProcessBatchTests, BatchProcessesAcceptedTelemetryValues)
{
    dispatcher::runtime::DispatcherRuntime runtime(
        make_batch_telemetry_configuration(),
        make_batch_alarm_configuration()
    );

    const auto summary = runtime.process_batch(
        std::vector<dispatcher::telemetry::TelemetryValue>{
        make_batch_telemetry_value(
            "tag-temperature",
            5.0,
            1
        ),
            make_batch_telemetry_value(
                "tag-pressure",
                101.0,
                2
            )
    }
    );

    EXPECT_EQ(summary.total_count, 2);

    EXPECT_EQ(summary.telemetry_accepted_count, 2);
    EXPECT_EQ(summary.telemetry_stored_count, 2);
    EXPECT_EQ(summary.telemetry_no_change_count, 0);
    EXPECT_EQ(summary.telemetry_rejected_count, 0);

    EXPECT_EQ(summary.history_written_count, 2);
    EXPECT_EQ(summary.history_skipped_count, 0);

    EXPECT_EQ(summary.configured_alarm_count, 2);
    EXPECT_EQ(summary.missing_condition_count, 0);

    EXPECT_EQ(summary.alarm_total_count, 2);
    EXPECT_EQ(summary.alarm_evaluated_count, 2);
    EXPECT_EQ(summary.alarm_skipped_count, 0);

    EXPECT_EQ(summary.alarm_activated_count, 2);
    EXPECT_EQ(summary.alarm_acknowledged_count, 0);
    EXPECT_EQ(summary.alarm_cleared_count, 0);
    EXPECT_EQ(summary.alarm_stored_event_count, 2);

    EXPECT_TRUE(summary.all_telemetry_accepted());
    EXPECT_FALSE(summary.has_telemetry_rejections());
    EXPECT_TRUE(summary.has_history_writes());
    EXPECT_TRUE(summary.has_alarm_evaluations());
    EXPECT_TRUE(summary.has_alarm_events());
    EXPECT_TRUE(summary.has_alarm_transitions());
    EXPECT_FALSE(summary.has_missing_conditions());
    EXPECT_TRUE(summary.successful());

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.telemetry.current_state_size, 2);
    EXPECT_EQ(snapshot.history.store_size, 2);
    EXPECT_EQ(snapshot.alarm.event_store_size, 2);

    EXPECT_EQ(snapshot.alarm_operator.active_alarm_count, 2);
    EXPECT_EQ(snapshot.alarm_operator.unacknowledged_alarm_count, 2);
    EXPECT_TRUE(snapshot.requires_operator_attention());
}

TEST(DispatcherRuntimeProcessBatchTests, BatchAggregatesRejectedTelemetry)
{
    dispatcher::runtime::DispatcherRuntime runtime(
        make_batch_telemetry_configuration(),
        make_batch_alarm_configuration()
    );

    const auto summary = runtime.process_batch(
        std::vector<dispatcher::telemetry::TelemetryValue>{
        make_batch_telemetry_value(
            "tag-temperature",
            5.0,
            1
        ),
            make_batch_telemetry_value(
                "unknown-tag",
                101.0,
                2
            )
    }
    );

    EXPECT_EQ(summary.total_count, 2);

    EXPECT_EQ(summary.telemetry_accepted_count, 1);
    EXPECT_EQ(summary.telemetry_stored_count, 1);
    EXPECT_EQ(summary.telemetry_rejected_count, 1);

    EXPECT_EQ(summary.history_written_count, 1);
    EXPECT_EQ(summary.history_skipped_count, 1);

    EXPECT_EQ(summary.configured_alarm_count, 1);
    EXPECT_EQ(summary.alarm_total_count, 1);
    EXPECT_EQ(summary.alarm_evaluated_count, 1);
    EXPECT_EQ(summary.alarm_activated_count, 1);
    EXPECT_EQ(summary.alarm_stored_event_count, 1);

    EXPECT_FALSE(summary.all_telemetry_accepted());
    EXPECT_TRUE(summary.has_telemetry_rejections());
    EXPECT_TRUE(summary.has_history_writes());
    EXPECT_TRUE(summary.has_alarm_events());
    EXPECT_FALSE(summary.successful());

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.telemetry.current_state_size, 1);
    EXPECT_EQ(snapshot.history.store_size, 1);
    EXPECT_EQ(snapshot.alarm.event_store_size, 1);
}

TEST(DispatcherRuntimeProcessBatchTests, LiveOnlyBatchSkipsHistoryButEvaluatesAlarms)
{
    dispatcher::runtime::DispatcherRuntime runtime(
        make_batch_telemetry_configuration(
            dispatcher::domain::HistoryPolicy::LiveOnly
        ),
        make_batch_alarm_configuration()
    );

    const auto summary = runtime.process_batch(
        std::vector<dispatcher::telemetry::TelemetryValue>{
        make_batch_telemetry_value(
            "tag-temperature",
            5.0,
            1
        ),
            make_batch_telemetry_value(
                "tag-pressure",
                101.0,
                2
            )
    }
    );

    EXPECT_EQ(summary.total_count, 2);

    EXPECT_EQ(summary.telemetry_accepted_count, 2);
    EXPECT_EQ(summary.telemetry_stored_count, 2);
    EXPECT_EQ(summary.telemetry_rejected_count, 0);

    EXPECT_EQ(summary.history_written_count, 0);
    EXPECT_EQ(summary.history_skipped_count, 2);

    EXPECT_EQ(summary.configured_alarm_count, 2);
    EXPECT_EQ(summary.alarm_activated_count, 2);
    EXPECT_EQ(summary.alarm_stored_event_count, 2);

    EXPECT_TRUE(summary.all_telemetry_accepted());
    EXPECT_FALSE(summary.has_telemetry_rejections());
    EXPECT_FALSE(summary.has_history_writes());
    EXPECT_TRUE(summary.has_alarm_events());
    EXPECT_TRUE(summary.successful());

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.telemetry.current_state_size, 2);
    EXPECT_EQ(snapshot.history.store_size, 0);
    EXPECT_EQ(snapshot.alarm.event_store_size, 2);
}