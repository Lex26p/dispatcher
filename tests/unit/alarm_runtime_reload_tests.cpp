#include <dispatcher/alarm/alarm_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_definition.hpp>
#include <dispatcher/alarm/alarm_condition_type.hpp>
#include <dispatcher/alarm/alarm_configuration_metadata.hpp>
#include <dispatcher/alarm/alarm_configuration_snapshot.hpp>
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
            .description("Test alarm")
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

    dispatcher::alarm::AlarmConfigurationSnapshot make_snapshot(
        std::uint64_t config_version,
        dispatcher::domain::ConfigurationStatus status
    )
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(config_version)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "plant-alarms",
                    .description = "Plant alarm configuration",
                    .created_by = "unit-test"
                }
            )
            .status(status)
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

    dispatcher::alarm::AlarmConfigurationSnapshot make_invalid_snapshot()
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(0)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "plant-alarms",
                    .description = "Plant alarm configuration",
                    .created_by = "unit-test"
                }
            )
            .status(dispatcher::domain::ConfigurationStatus::Published)
            .build();
    }

    dispatcher::alarm::AlarmDefinition make_runtime_alarm_definition()
    {
        return make_alarm_definition(
            "alarm-temperature-high",
            "tag-temperature",
            "temperature_high"
        );
    }

    dispatcher::alarm::ThresholdAlarmCondition make_high_condition()
    {
        return dispatcher::alarm::ThresholdAlarmCondition(
            dispatcher::alarm::AlarmConditionType::High,
            80.0
        );
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

TEST(AlarmRuntimeReloadTests, DefaultRuntimeHasPublishedEmptyConfiguration)
{
    const dispatcher::alarm::AlarmRuntime runtime;

    EXPECT_TRUE(runtime.configuration_snapshot().published());
    EXPECT_FALSE(runtime.configuration_snapshot().draft());

    EXPECT_EQ(runtime.configuration_snapshot().config_version(), 1);
    EXPECT_TRUE(runtime.configuration_snapshot().alarm_catalog().empty());
    EXPECT_TRUE(runtime.configuration_snapshot().condition_catalog().empty());
}

TEST(AlarmRuntimeReloadTests, ReloadAcceptsPublishedValidConfiguration)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto result = runtime.reload_configuration(
        make_snapshot(
            7,
            dispatcher::domain::ConfigurationStatus::Published
        )
    );

    EXPECT_TRUE(result.valid());
    EXPECT_FALSE(result.has_errors());

    EXPECT_EQ(runtime.configuration_snapshot().config_version(), 7);
    EXPECT_TRUE(runtime.configuration_snapshot().published());

    EXPECT_EQ(runtime.configuration_snapshot().alarm_catalog().size(), 1);
    EXPECT_EQ(runtime.configuration_snapshot().condition_catalog().size(), 1);

    const auto alarm_definition =
        runtime.configuration_snapshot().find_by_alarm_id(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" }
        );

    ASSERT_TRUE(alarm_definition.has_value());

    EXPECT_EQ(
        alarm_definition->tag_id(),
        dispatcher::domain::TagId{ "tag-temperature" }
    );
}

TEST(AlarmRuntimeReloadTests, ReloadRejectsDraftConfiguration)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto original_version =
        runtime.configuration_snapshot().config_version();

    const auto result = runtime.reload_configuration(
        make_snapshot(
            7,
            dispatcher::domain::ConfigurationStatus::Draft
        )
    );

    EXPECT_FALSE(result.valid());
    EXPECT_TRUE(result.has_errors());

    EXPECT_EQ(result.errors()[0].field, "status");

    EXPECT_EQ(
        runtime.configuration_snapshot().config_version(),
        original_version
    );

    EXPECT_TRUE(runtime.configuration_snapshot().published());
}

TEST(AlarmRuntimeReloadTests, ReloadRejectsInvalidConfiguration)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto result = runtime.reload_configuration(
        make_invalid_snapshot()
    );

    EXPECT_FALSE(result.valid());
    EXPECT_TRUE(result.has_errors());

    bool found_config_version_error = false;

    for (const auto& error : result.errors())
    {
        if (error.field == "config_version")
        {
            found_config_version_error = true;
        }
    }

    EXPECT_TRUE(found_config_version_error);

    EXPECT_EQ(runtime.configuration_snapshot().config_version(), 1);
    EXPECT_TRUE(runtime.configuration_snapshot().alarm_catalog().empty());
}

TEST(AlarmRuntimeReloadTests, ReloadDoesNotClearStateStoreEventStoreOrStatistics)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto definition = make_runtime_alarm_definition();
    const auto condition = make_high_condition();

    ASSERT_TRUE(
        runtime.evaluate(
            definition,
            condition,
            make_telemetry_value(
                81.0,
                1
            )
        ).activated()
    );

    ASSERT_EQ(
        runtime.state_store().state_of(definition.alarm_id()),
        dispatcher::alarm::AlarmState::Active
    );

    ASSERT_EQ(runtime.event_store().size(), 1);
    ASSERT_EQ(runtime.statistics().total_count(), 1);

    const auto result = runtime.reload_configuration(
        make_snapshot(
            8,
            dispatcher::domain::ConfigurationStatus::Published
        )
    );

    ASSERT_TRUE(result.valid());

    EXPECT_EQ(runtime.configuration_snapshot().config_version(), 8);

    EXPECT_EQ(
        runtime.state_store().state_of(definition.alarm_id()),
        dispatcher::alarm::AlarmState::Active
    );

    EXPECT_EQ(runtime.event_store().size(), 1);
    EXPECT_EQ(runtime.statistics().total_count(), 1);
    EXPECT_EQ(runtime.statistics().activated_count(), 1);
}

TEST(AlarmRuntimeReloadTests, FailedReloadDoesNotReplaceExistingConfiguration)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto accepted_result = runtime.reload_configuration(
        make_snapshot(
            10,
            dispatcher::domain::ConfigurationStatus::Published
        )
    );

    ASSERT_TRUE(accepted_result.valid());
    ASSERT_EQ(runtime.configuration_snapshot().config_version(), 10);

    const auto rejected_result = runtime.reload_configuration(
        make_snapshot(
            11,
            dispatcher::domain::ConfigurationStatus::Draft
        )
    );

    ASSERT_FALSE(rejected_result.valid());

    EXPECT_EQ(runtime.configuration_snapshot().config_version(), 10);
    EXPECT_TRUE(runtime.configuration_snapshot().published());
}