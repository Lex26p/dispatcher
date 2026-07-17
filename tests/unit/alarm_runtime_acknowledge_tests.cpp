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
#include <dispatcher/alarm/alarm_transition_type.hpp>
#include <dispatcher/alarm/threshold_alarm_condition.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
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
            .description("Runtime acknowledgement test alarm")
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

    dispatcher::alarm::AlarmConfigurationSnapshot make_snapshot()
    {
        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(7)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "acknowledgement-alarms",
                    .description = "Acknowledgement alarm configuration",
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

TEST(AlarmRuntimeAcknowledgeTests, AcknowledgementResultHelpersWork)
{
    const dispatcher::alarm::AlarmAcknowledgementResult result(
        dispatcher::alarm::AlarmAcknowledgementStatus::UnknownAlarm,
        dispatcher::alarm::AlarmState::Normal,
        dispatcher::alarm::AlarmState::Normal,
        std::nullopt
    );

    EXPECT_FALSE(result.acknowledged());
    EXPECT_TRUE(result.skipped());
    EXPECT_TRUE(result.unknown_alarm());
    EXPECT_FALSE(result.not_active());
    EXPECT_FALSE(result.already_acknowledged());

    EXPECT_EQ(
        dispatcher::alarm::to_string(result.status()),
        "unknown_alarm"
    );
}

TEST(AlarmRuntimeAcknowledgeTests, UnknownAlarmIsRejected)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(runtime.reload_configuration(make_snapshot()).valid());

    const auto result = runtime.acknowledge(
        dispatcher::domain::AlarmId{ "unknown-alarm" }
    );

    EXPECT_TRUE(result.skipped());
    EXPECT_TRUE(result.unknown_alarm());

    EXPECT_EQ(result.previous_state(), dispatcher::alarm::AlarmState::Normal);
    EXPECT_EQ(result.new_state(), dispatcher::alarm::AlarmState::Normal);
    EXPECT_FALSE(result.event().has_value());

    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_TRUE(runtime.event_store().empty());
    EXPECT_EQ(runtime.statistics().acknowledged_count(), 0);
}

TEST(AlarmRuntimeAcknowledgeTests, NormalAlarmIsNotActive)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(runtime.reload_configuration(make_snapshot()).valid());

    const auto result = runtime.acknowledge(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_TRUE(result.skipped());
    EXPECT_TRUE(result.not_active());

    EXPECT_EQ(result.previous_state(), dispatcher::alarm::AlarmState::Normal);
    EXPECT_EQ(result.new_state(), dispatcher::alarm::AlarmState::Normal);
    EXPECT_FALSE(result.event().has_value());

    EXPECT_TRUE(runtime.state_store().empty());
    EXPECT_TRUE(runtime.event_store().empty());
    EXPECT_EQ(runtime.statistics().acknowledged_count(), 0);
}

TEST(AlarmRuntimeAcknowledgeTests, ActiveAlarmCanBeAcknowledged)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(runtime.reload_configuration(make_snapshot()).valid());

    const auto activate_result = runtime.evaluate_configured(
        make_telemetry_value(81.0, 10)
    );

    ASSERT_EQ(activate_result.batch_result().activated_count(), 1);
    ASSERT_EQ(runtime.event_store().size(), 1);

    const auto result = runtime.acknowledge(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_TRUE(result.acknowledged());
    EXPECT_FALSE(result.skipped());

    EXPECT_EQ(result.previous_state(), dispatcher::alarm::AlarmState::Active);
    EXPECT_EQ(result.new_state(), dispatcher::alarm::AlarmState::Acknowledged);

    ASSERT_TRUE(result.event().has_value());

    EXPECT_EQ(
        result.event()->transition_type(),
        dispatcher::alarm::AlarmTransitionType::Acknowledged
    );

    EXPECT_EQ(
        result.event()->previous_state(),
        dispatcher::alarm::AlarmState::Active
    );

    EXPECT_EQ(
        result.event()->new_state(),
        dispatcher::alarm::AlarmState::Acknowledged
    );

    EXPECT_EQ(
        result.event()->alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_EQ(
        result.event()->tag_id(),
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    EXPECT_EQ(result.event()->sequence(), 1);

    EXPECT_EQ(
        runtime.state_store().state_of(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" }
        ),
        dispatcher::alarm::AlarmState::Acknowledged
    );

    ASSERT_EQ(runtime.event_store().size(), 2);

    EXPECT_EQ(
        runtime.event_store().events()[1].transition_type(),
        dispatcher::alarm::AlarmTransitionType::Acknowledged
    );

    EXPECT_EQ(runtime.statistics().acknowledged_count(), 1);
    EXPECT_EQ(runtime.statistics().stored_event_count(), 2);
    EXPECT_EQ(runtime.runtime_snapshot().acknowledged_count, 1);
}

TEST(AlarmRuntimeAcknowledgeTests, AlreadyAcknowledgedAlarmIsNotAcknowledgedAgain)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(runtime.reload_configuration(make_snapshot()).valid());

    ASSERT_EQ(
        runtime.evaluate_configured(
            make_telemetry_value(81.0, 10)
        ).batch_result().activated_count(),
        1
    );

    ASSERT_TRUE(
        runtime.acknowledge(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" }
        ).acknowledged()
    );

    ASSERT_EQ(runtime.event_store().size(), 2);

    const auto result = runtime.acknowledge(
        dispatcher::domain::AlarmId{ "alarm-temperature-high" }
    );

    EXPECT_TRUE(result.skipped());
    EXPECT_TRUE(result.already_acknowledged());

    EXPECT_EQ(
        result.previous_state(),
        dispatcher::alarm::AlarmState::Acknowledged
    );

    EXPECT_EQ(
        result.new_state(),
        dispatcher::alarm::AlarmState::Acknowledged
    );

    EXPECT_FALSE(result.event().has_value());

    EXPECT_EQ(runtime.event_store().size(), 2);
    EXPECT_EQ(runtime.statistics().acknowledged_count(), 1);
}

TEST(AlarmRuntimeAcknowledgeTests, AcknowledgedAlarmClearsWhenConditionReturnsNormal)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(runtime.reload_configuration(make_snapshot()).valid());

    ASSERT_EQ(
        runtime.evaluate_configured(
            make_telemetry_value(81.0, 10)
        ).batch_result().activated_count(),
        1
    );

    ASSERT_TRUE(
        runtime.acknowledge(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" }
        ).acknowledged()
    );

    ASSERT_EQ(
        runtime.state_store().state_of(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" }
        ),
        dispatcher::alarm::AlarmState::Acknowledged
    );

    const auto clear_result = runtime.evaluate_configured(
        make_telemetry_value(79.0, 11)
    );

    EXPECT_EQ(clear_result.batch_result().cleared_count(), 1);

    EXPECT_EQ(
        runtime.state_store().state_of(
            dispatcher::domain::AlarmId{ "alarm-temperature-high" }
        ),
        dispatcher::alarm::AlarmState::Normal
    );

    ASSERT_EQ(runtime.event_store().size(), 3);

    EXPECT_EQ(
        runtime.event_store().events()[2].transition_type(),
        dispatcher::alarm::AlarmTransitionType::Cleared
    );

    EXPECT_EQ(runtime.statistics().activated_count(), 1);
    EXPECT_EQ(runtime.statistics().acknowledged_count(), 1);
    EXPECT_EQ(runtime.statistics().cleared_count(), 1);
}