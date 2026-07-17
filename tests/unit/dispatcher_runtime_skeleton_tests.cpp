#include <dispatcher/alarm/alarm_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_definition.hpp>
#include <dispatcher/alarm/alarm_condition_type.hpp>
#include <dispatcher/alarm/alarm_configuration_metadata.hpp>
#include <dispatcher/alarm/alarm_configuration_snapshot_builder.hpp>
#include <dispatcher/alarm/alarm_definition_builder.hpp>
#include <dispatcher/alarm/alarm_severity.hpp>
#include <dispatcher/alarm/threshold_alarm_condition.hpp>
#include <dispatcher/domain/configuration_snapshot_builder.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/device_definition_builder.hpp>
#include <dispatcher/domain/history_policy.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/tag_definition_builder.hpp>
#include <dispatcher/runtime/dispatcher_runtime.hpp>

#include <gtest/gtest.h>

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

    dispatcher::domain::TagDefinition make_tag()
    {
        return dispatcher::domain::TagDefinitionBuilder{}
            .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
            .device_id(dispatcher::domain::DeviceId{ "device-1" })
            .local_name("temperature")
            .display_name("Temperature")
            .data_type(dispatcher::domain::DataType::Float64)
            .history_policy(dispatcher::domain::HistoryPolicy::EveryPoll)
            .enabled(true)
            .build();
    }

    dispatcher::domain::ConfigurationSnapshot make_telemetry_configuration(
        dispatcher::domain::ConfigurationStatus status =
        dispatcher::domain::ConfigurationStatus::Published
    )
    {
        dispatcher::domain::ConfigurationSnapshotBuilder builder;

        const auto device_result = builder.add_device(make_device());
        (void)device_result;

        const auto tag_result = builder.add_tag(make_tag());
        (void)tag_result;

        return builder
            .config_version(7)
            .status(status)
            .build();
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

    dispatcher::alarm::AlarmConfigurationSnapshot make_alarm_configuration(
        dispatcher::domain::ConfigurationStatus status =
        dispatcher::domain::ConfigurationStatus::Published
    )
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(7)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "runtime-alarms",
                    .description = "Runtime alarm configuration",
                    .created_by = "unit-test"
                }
            )
            .status(status)
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
}

TEST(DispatcherRuntimeSkeletonTests, DefaultRuntimeHasEmptySnapshots)
{
    const dispatcher::runtime::DispatcherRuntime runtime;

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.telemetry.configuration_version, 1);
    EXPECT_EQ(snapshot.telemetry.current_state_size, 0);

    EXPECT_EQ(snapshot.history.store_size, 0);

    EXPECT_EQ(snapshot.alarm.configuration_version, 1);
    EXPECT_EQ(snapshot.alarm.configured_alarm_count, 0);

    EXPECT_EQ(snapshot.alarm_operator.configuration_version, 1);
    EXPECT_EQ(snapshot.alarm_operator.configured_alarm_count, 0);

    EXPECT_FALSE(snapshot.has_current_state());
    EXPECT_FALSE(snapshot.has_history_samples());
    EXPECT_FALSE(snapshot.has_alarm_events());
    EXPECT_FALSE(snapshot.has_acknowledgement_audit());
    EXPECT_FALSE(snapshot.requires_operator_attention());
}

TEST(DispatcherRuntimeSkeletonTests, RuntimeCanBeConstructedWithTelemetryConfiguration)
{
    const dispatcher::runtime::DispatcherRuntime runtime(
        make_telemetry_configuration()
    );

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.telemetry.configuration_version, 7);
    EXPECT_EQ(snapshot.telemetry.current_state_size, 0);

    EXPECT_EQ(snapshot.alarm.configuration_version, 1);
    EXPECT_EQ(snapshot.alarm.configured_alarm_count, 0);
}

TEST(DispatcherRuntimeSkeletonTests, RuntimeCanBeConstructedWithTelemetryAndAlarmConfiguration)
{
    const dispatcher::runtime::DispatcherRuntime runtime(
        make_telemetry_configuration(),
        make_alarm_configuration()
    );

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.telemetry.configuration_version, 7);
    EXPECT_EQ(snapshot.alarm.configuration_version, 7);
    EXPECT_EQ(snapshot.alarm.configured_alarm_count, 1);
    EXPECT_EQ(snapshot.alarm.indexed_tag_count, 1);
    EXPECT_EQ(snapshot.alarm.indexed_condition_count, 1);

    EXPECT_EQ(snapshot.alarm_operator.configuration_version, 7);
    EXPECT_EQ(snapshot.alarm_operator.configured_alarm_count, 1);
    EXPECT_EQ(snapshot.alarm_operator.normal_alarm_count, 1);
}

TEST(DispatcherRuntimeSkeletonTests, TelemetryConfigurationCanBeReloaded)
{
    dispatcher::runtime::DispatcherRuntime runtime;

    const auto result = runtime.reload_telemetry_configuration(
        make_telemetry_configuration()
    );

    ASSERT_TRUE(result.valid());

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.telemetry.configuration_version, 7);
}

TEST(DispatcherRuntimeSkeletonTests, DraftTelemetryConfigurationIsRejected)
{
    dispatcher::runtime::DispatcherRuntime runtime;

    const auto result = runtime.reload_telemetry_configuration(
        make_telemetry_configuration(
            dispatcher::domain::ConfigurationStatus::Draft
        )
    );

    ASSERT_FALSE(result.valid());

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.telemetry.configuration_version, 1);
}

TEST(DispatcherRuntimeSkeletonTests, AlarmConfigurationCanBeReloaded)
{
    dispatcher::runtime::DispatcherRuntime runtime;

    const auto result = runtime.reload_alarm_configuration(
        make_alarm_configuration()
    );

    ASSERT_TRUE(result.valid());

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.alarm.configuration_version, 7);
    EXPECT_EQ(snapshot.alarm.configured_alarm_count, 1);

    EXPECT_EQ(snapshot.alarm_operator.configuration_version, 7);
    EXPECT_EQ(snapshot.alarm_operator.configured_alarm_count, 1);
}

TEST(DispatcherRuntimeSkeletonTests, DraftAlarmConfigurationIsRejected)
{
    dispatcher::runtime::DispatcherRuntime runtime;

    const auto result = runtime.reload_alarm_configuration(
        make_alarm_configuration(
            dispatcher::domain::ConfigurationStatus::Draft
        )
    );

    ASSERT_FALSE(result.valid());

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.alarm.configuration_version, 1);
    EXPECT_EQ(snapshot.alarm.configured_alarm_count, 0);
}