#include <dispatcher/alarm/alarm_acknowledgement_command.hpp>
#include <dispatcher/alarm/alarm_acknowledgement_result.hpp>
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
    dispatcher::domain::DeviceDefinition make_ack_device()
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

    dispatcher::domain::TagDefinition make_ack_tag()
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
            .history_policy(dispatcher::domain::HistoryPolicy::EveryPoll)
            .enabled(true)
            .build();
    }

    dispatcher::domain::ConfigurationSnapshot make_ack_telemetry_configuration()
    {
        dispatcher::domain::ConfigurationSnapshotBuilder builder;

        builder.config_version(7);
        builder.status(dispatcher::domain::ConfigurationStatus::Published);

        const auto device_result = builder.add_device(make_ack_device());

        if (device_result.has_errors())
        {
            throw std::runtime_error(
                "failed to add device to telemetry configuration: "
                + device_result.errors().front().field
                + " - "
                + device_result.errors().front().message
            );
        }

        const auto tag_result = builder.add_tag(make_ack_tag());

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

    dispatcher::alarm::AlarmDefinition make_ack_alarm_definition()
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

    dispatcher::alarm::AlarmConditionDefinition make_ack_alarm_condition_definition()
    {
        return dispatcher::alarm::AlarmConditionDefinition(
            dispatcher::domain::AlarmId{ "alarm-temperature-low" },
            dispatcher::alarm::ThresholdAlarmCondition(
                dispatcher::alarm::AlarmConditionType::Low,
                10.0
            )
        );
    }

    dispatcher::alarm::AlarmConfigurationSnapshot make_ack_alarm_configuration()
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(7)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "runtime-ack-alarms",
                    .description = "Runtime acknowledgement alarm configuration",
                    .created_by = "unit-test"
                }
            )
            .status(dispatcher::domain::ConfigurationStatus::Published)
            .alarm_catalog(
                dispatcher::alarm::AlarmCatalog(
                    std::vector<dispatcher::alarm::AlarmDefinition>{
            make_ack_alarm_definition()
        }
                )
            )
            .condition_catalog(
                dispatcher::alarm::AlarmConditionCatalog(
                    std::vector<dispatcher::alarm::AlarmConditionDefinition>{
            make_ack_alarm_condition_definition()
        }
                )
            )
            .build();
    }

    dispatcher::telemetry::TelemetryValue make_ack_telemetry_value(
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
            TagValue(static_cast<double>(value)),
            Quality::Good,
            now,
            now,
            sequence
        );
    }
}

TEST(DispatcherRuntimeAcknowledgementTests, RuntimeExposesOperatorSnapshot)
{
    dispatcher::runtime::DispatcherRuntime runtime(
        make_ack_telemetry_configuration(),
        make_ack_alarm_configuration()
    );

    const auto initial_operator_snapshot = runtime.alarm_operator_snapshot();

    EXPECT_EQ(initial_operator_snapshot.configured_alarm_count, 1);
    EXPECT_EQ(initial_operator_snapshot.normal_alarm_count, 1);
    EXPECT_EQ(initial_operator_snapshot.acknowledged_alarm_count, 0);
    EXPECT_EQ(initial_operator_snapshot.unacknowledged_alarm_count, 0);
    EXPECT_FALSE(initial_operator_snapshot.requires_operator_attention());

    const auto summary = runtime.process(
        make_ack_telemetry_value(
            5.0,
            1
        )
    );

    ASSERT_EQ(
        summary.telemetry_status,
        dispatcher::core::TelemetryIngestStatus::Accepted
    ) << "Actual telemetry status: "
        << dispatcher::core::to_string(summary.telemetry_status);

    ASSERT_EQ(summary.alarm_activated_count, 1);

    const auto active_operator_snapshot = runtime.alarm_operator_snapshot();

    EXPECT_EQ(active_operator_snapshot.configured_alarm_count, 1);
    EXPECT_EQ(active_operator_snapshot.normal_alarm_count, 0);
    EXPECT_EQ(active_operator_snapshot.active_alarm_count, 1);
    EXPECT_EQ(active_operator_snapshot.acknowledged_alarm_count, 0);
    EXPECT_EQ(active_operator_snapshot.unacknowledged_alarm_count, 1);
    EXPECT_TRUE(active_operator_snapshot.requires_operator_attention());
}

TEST(DispatcherRuntimeAcknowledgementTests, RuntimeReturnsUnacknowledgedAlarms)
{
    dispatcher::runtime::DispatcherRuntime runtime(
        make_ack_telemetry_configuration(),
        make_ack_alarm_configuration()
    );

    EXPECT_TRUE(runtime.unacknowledged_alarms().empty());

    const auto summary = runtime.process(
        make_ack_telemetry_value(
            5.0,
            1
        )
    );

    ASSERT_EQ(
        summary.telemetry_status,
        dispatcher::core::TelemetryIngestStatus::Accepted
    ) << "Actual telemetry status: "
        << dispatcher::core::to_string(summary.telemetry_status);

    const auto unacknowledged_alarms = runtime.unacknowledged_alarms();

    ASSERT_EQ(unacknowledged_alarms.size(), 1);
    EXPECT_EQ(
        unacknowledged_alarms.front().alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-low" }
    );
}

TEST(DispatcherRuntimeAcknowledgementTests, RuntimeAcknowledgesAlarmWithAudit)
{
    dispatcher::runtime::DispatcherRuntime runtime(
        make_ack_telemetry_configuration(),
        make_ack_alarm_configuration()
    );

    const auto summary = runtime.process(
        make_ack_telemetry_value(
            5.0,
            1
        )
    );

    ASSERT_EQ(
        summary.telemetry_status,
        dispatcher::core::TelemetryIngestStatus::Accepted
    ) << "Actual telemetry status: "
        << dispatcher::core::to_string(summary.telemetry_status);

    ASSERT_EQ(summary.alarm_activated_count, 1);

    const auto unacknowledged_alarms = runtime.unacknowledged_alarms();

    ASSERT_EQ(unacknowledged_alarms.size(), 1);

    const dispatcher::alarm::AlarmAcknowledgementCommand command(
        unacknowledged_alarms.front().alarm_id(),
        "operator-runtime-test",
        "acknowledged through dispatcher runtime facade"
    );

    const auto acknowledgement_result = runtime.acknowledge_alarm(command);

    ASSERT_TRUE(acknowledgement_result.acknowledged());
    EXPECT_EQ(
        acknowledgement_result.status(),
        dispatcher::alarm::AlarmAcknowledgementStatus::Acknowledged
    );

    const auto operator_snapshot = runtime.alarm_operator_snapshot();

    EXPECT_EQ(operator_snapshot.active_alarm_count, 0);
    EXPECT_EQ(operator_snapshot.acknowledged_alarm_count, 1);
    EXPECT_EQ(operator_snapshot.unacknowledged_alarm_count, 0);
    EXPECT_FALSE(operator_snapshot.requires_operator_attention());

    const auto runtime_snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(runtime_snapshot.alarm.acknowledgement_store_size, 1);
    EXPECT_EQ(runtime_snapshot.alarm.event_store_size, 2);
    EXPECT_EQ(runtime_snapshot.alarm.acknowledged_count, 1);

    EXPECT_TRUE(runtime.unacknowledged_alarms().empty());
}

TEST(DispatcherRuntimeAcknowledgementTests, RuntimeRejectsInvalidAcknowledgementCommand)
{
    dispatcher::runtime::DispatcherRuntime runtime(
        make_ack_telemetry_configuration(),
        make_ack_alarm_configuration()
    );

    const dispatcher::alarm::AlarmAcknowledgementCommand invalid_command(
        dispatcher::domain::AlarmId{ "" },
        "operator-runtime-test",
        "invalid acknowledgement"
    );

    const auto acknowledgement_result =
        runtime.acknowledge_alarm(invalid_command);

    EXPECT_FALSE(acknowledgement_result.acknowledged());
    EXPECT_EQ(
        acknowledgement_result.status(),
        dispatcher::alarm::AlarmAcknowledgementStatus::InvalidCommand
    );

    const auto runtime_snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(runtime_snapshot.alarm.acknowledgement_store_size, 1);
    EXPECT_EQ(runtime_snapshot.alarm.event_store_size, 0);
    EXPECT_EQ(runtime_snapshot.alarm.acknowledged_count, 0);
}