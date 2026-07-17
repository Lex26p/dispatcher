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
            .description("Reload prune test alarm")
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
                    .name = "two-alarms",
                    .description = "Two alarm configuration",
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
                    .name = "temperature-only",
                    .description = "Temperature only alarm configuration",
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

    dispatcher::alarm::AlarmConfigurationSnapshot make_draft_empty_snapshot()
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(9)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "draft-empty",
                    .description = "Draft empty alarm configuration",
                    .created_by = "unit-test"
                }
            )
            .status(dispatcher::domain::ConfigurationStatus::Draft)
            .build();
    }

    dispatcher::telemetry::TelemetryValue make_telemetry_value(
        std::string tag_id,
        dispatcher::telemetry::TagValue value,
        std::uint64_t sequence
    )
    {
        using dispatcher::domain::Quality;
        using dispatcher::domain::TagId;
        using dispatcher::telemetry::TelemetryValue;

        const auto now = TelemetryValue::Clock::now();

        return TelemetryValue(
            TagId{ std::move(tag_id) },
            std::move(value),
            Quality::Good,
            now,
            now,
            sequence
        );
    }
}

TEST(AlarmRuntimeReloadPruneTests, SuccessfulReloadPrunesRemovedAlarmStates)
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
            dispatcher::telemetry::TagValue(81.0),
            1
        ),
            make_telemetry_value(
                "tag-pressure",
                dispatcher::telemetry::TagValue(11.0),
                2
            )
    }
    );

    ASSERT_EQ(activate_result.batch_result().activated_count(), 2);
    ASSERT_EQ(runtime.state_store().size(), 2);
    ASSERT_EQ(runtime.event_store().size(), 2);
    ASSERT_EQ(runtime.statistics().total_count(), 2);

    ASSERT_TRUE(
        runtime.state_store().has_state(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" }
        )
    );

    ASSERT_TRUE(
        runtime.state_store().has_state(
            dispatcher::domain::AlarmId{ "alarm-pressure-high" }
        )
    );

    const auto reload_result = runtime.reload_configuration(
        make_temperature_only_snapshot()
    );

    ASSERT_TRUE(reload_result.valid());

    EXPECT_EQ(runtime.configuration_snapshot().config_version(), 8);

    EXPECT_TRUE(
        runtime.state_store().has_state(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" }
        )
    );

    EXPECT_FALSE(
        runtime.state_store().has_state(
            dispatcher::domain::AlarmId{ "alarm-pressure-high" }
        )
    );

    EXPECT_EQ(runtime.state_store().size(), 1);

    EXPECT_EQ(runtime.event_store().size(), 2);
    EXPECT_EQ(runtime.statistics().total_count(), 2);
}

TEST(AlarmRuntimeReloadPruneTests, FailedReloadDoesNotPruneStates)
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
            dispatcher::telemetry::TagValue(81.0),
            1
        ),
            make_telemetry_value(
                "tag-pressure",
                dispatcher::telemetry::TagValue(11.0),
                2
            )
    }
    );

    ASSERT_EQ(activate_result.batch_result().activated_count(), 2);
    ASSERT_EQ(runtime.state_store().size(), 2);

    const auto rejected_reload_result = runtime.reload_configuration(
        make_draft_empty_snapshot()
    );

    ASSERT_FALSE(rejected_reload_result.valid());

    EXPECT_EQ(runtime.configuration_snapshot().config_version(), 7);

    EXPECT_TRUE(
        runtime.state_store().has_state(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" }
        )
    );

    EXPECT_TRUE(
        runtime.state_store().has_state(
            dispatcher::domain::AlarmId{ "alarm-pressure-high" }
        )
    );

    EXPECT_EQ(runtime.state_store().size(), 2);
}

TEST(AlarmRuntimeReloadPruneTests, ReloadToEmptyPublishedConfigurationPrunesAllStates)
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
            dispatcher::telemetry::TagValue(81.0),
            1
        ),
            make_telemetry_value(
                "tag-pressure",
                dispatcher::telemetry::TagValue(11.0),
                2
            )
    }
    );

    ASSERT_EQ(activate_result.batch_result().activated_count(), 2);
    ASSERT_EQ(runtime.state_store().size(), 2);

    const auto empty_snapshot =
        dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
        .config_version(10)
        .metadata(
            dispatcher::alarm::AlarmConfigurationMetadata{
                .name = "empty-published",
                .description = "Empty published alarm configuration",
                .created_by = "unit-test"
            }
        )
        .status(dispatcher::domain::ConfigurationStatus::Published)
        .build();

    const auto reload_result = runtime.reload_configuration(empty_snapshot);

    ASSERT_TRUE(reload_result.valid());

    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_EQ(runtime.event_store().size(), 2);
    EXPECT_EQ(runtime.statistics().total_count(), 2);
    EXPECT_EQ(runtime.runtime_snapshot().configured_alarm_count, 0);
    EXPECT_EQ(runtime.runtime_snapshot().indexed_tag_count, 0);
    EXPECT_EQ(runtime.runtime_snapshot().indexed_condition_count, 0);
}