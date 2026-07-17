#include <dispatcher/alarm/alarm_acknowledgement_command.hpp>
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
#include <dispatcher/domain/configuration_snapshot_validation.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/device_definition_builder.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/domain/tag_definition_builder.hpp>

#include <dispatcher/runtime/dispatcher_runtime.hpp>

#include <dispatcher/simulator/telemetry_generator.hpp>

#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <spdlog/spdlog.h>

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <string>
#include <utility>
#include <vector>

namespace
{
    std::size_t parse_tag_count(int argc, char** argv)
    {
        if (argc < 2)
        {
            return 10;
        }

        try
        {
            const auto value = std::stoull(argv[1]);

            if (value == 0)
            {
                return 10;
            }

            return static_cast<std::size_t>(value);
        }
        catch (const std::exception&)
        {
            return 10;
        }
    }

    dispatcher::domain::DeviceDefinition make_simulated_device()
    {
        using namespace dispatcher::domain;

        return DeviceDefinitionBuilder{}
            .organization_id(OrganizationId{ "org-1" })
            .site_id(SiteId{ "site-1" })
            .area_id(AreaId{ "area-1" })
            .device_id(DeviceId{ "device-1" })
            .local_name("simulator-device-1")
            .display_name("Simulator Device 1")
            .description("Synthetic telemetry generator device")
            .enabled(true)
            .config_version(1)
            .build();
    }

    dispatcher::domain::TagDefinition make_simulated_tag(std::size_t index)
    {
        using namespace dispatcher::domain;

        const auto tag_number = index + 1;
        const auto tag_id = "tag-" + std::to_string(tag_number);
        const auto local_name = "tag-" + std::to_string(tag_number);

        return TagDefinitionBuilder{}
            .organization_id(OrganizationId{ "org-1" })
            .site_id(SiteId{ "site-1" })
            .area_id(AreaId{ "area-1" })
            .device_id(DeviceId{ "device-1" })
            .tag_id(TagId{ tag_id })
            .local_name(local_name)
            .display_name("Simulated Tag " + std::to_string(tag_number))
            .description("Synthetic Float64 telemetry tag")
            .data_type(DataType::Float64)
            .engineering_unit("unit")
            .history_policy(HistoryPolicy::OnChange)
            .enabled(true)
            .config_version(1)
            .build();
    }

    dispatcher::alarm::AlarmDefinition make_simulated_alarm_definition(
        const dispatcher::domain::TagId& tag_id,
        std::uint64_t config_version
    )
    {
        const auto alarm_id = dispatcher::domain::AlarmId{
            "alarm-" + tag_id.value() + "-low"
        };

        return dispatcher::alarm::AlarmDefinitionBuilder{}
            .alarm_id(alarm_id)
            .tag_id(tag_id)
            .name("alarm_" + tag_id.value() + "_low")
            .description("Simulated low threshold alarm")
            .severity(dispatcher::alarm::AlarmSeverity::Warning)
            .enabled(true)
            .config_version(config_version)
            .build();
    }

    dispatcher::alarm::AlarmConditionDefinition
        make_simulated_alarm_condition_definition(
            const dispatcher::domain::AlarmId& alarm_id
        )
    {
        return dispatcher::alarm::AlarmConditionDefinition(
            alarm_id,
            dispatcher::alarm::ThresholdAlarmCondition(
                dispatcher::alarm::AlarmConditionType::Low,
                1'000'000'000.0
            )
        );
    }

    dispatcher::telemetry::TelemetryValue make_clearing_telemetry_value(
        const dispatcher::alarm::AlarmDefinition& alarm_definition,
        std::uint64_t sequence
    )
    {
        using dispatcher::domain::Quality;
        using dispatcher::telemetry::TagValue;
        using dispatcher::telemetry::TelemetryValue;

        const auto now = TelemetryValue::Clock::now();

        return TelemetryValue(
            alarm_definition.tag_id(),
            TagValue(2'000'000'000.0),
            Quality::Good,
            now,
            now,
            sequence
        );
    }

    dispatcher::domain::ConfigurationSnapshot make_configuration_snapshot(
        std::size_t tag_count
    )
    {
        dispatcher::domain::ConfigurationSnapshotBuilder builder;

        const auto device_result = builder.add_device(make_simulated_device());

        if (device_result.has_errors())
        {
            spdlog::error("Failed to add simulated device to configuration");
        }

        for (std::size_t index = 0; index < tag_count; ++index)
        {
            const auto tag_result = builder.add_tag(make_simulated_tag(index));

            if (tag_result.has_errors())
            {
                spdlog::error(
                    "Failed to add simulated tag {} to configuration",
                    index + 1
                );
            }
        }

        return builder
            .config_version(1)
            .published()
            .description("E2E smoke generated configuration")
            .build();
    }

    dispatcher::alarm::AlarmConfigurationSnapshot
        make_simulated_alarm_configuration_snapshot(
            std::size_t tag_count,
            std::uint64_t config_version
        )
    {
        std::vector<dispatcher::alarm::AlarmDefinition> alarm_definitions;
        std::vector<dispatcher::alarm::AlarmConditionDefinition>
            condition_definitions;

        alarm_definitions.reserve(tag_count);
        condition_definitions.reserve(tag_count);

        for (std::size_t index = 0; index < tag_count; ++index)
        {
            const auto tag_number = index + 1;
            const auto tag_id = dispatcher::domain::TagId{
                "tag-" + std::to_string(tag_number)
            };

            const auto alarm_definition = make_simulated_alarm_definition(
                tag_id,
                config_version
            );

            condition_definitions.push_back(
                make_simulated_alarm_condition_definition(
                    alarm_definition.alarm_id()
                )
            );

            alarm_definitions.push_back(alarm_definition);
        }

        return dispatcher::alarm::AlarmConfigurationSnapshotBuilder{}
            .config_version(config_version)
            .metadata(
                dispatcher::alarm::AlarmConfigurationMetadata{
                    .name = "simulated-alarms",
                    .description = "Simulated alarm configuration",
                    .created_by = "dispatcher-e2e-smoke"
                }
            )
            .status(dispatcher::domain::ConfigurationStatus::Published)
            .alarm_catalog(
                dispatcher::alarm::AlarmCatalog(
                    std::move(alarm_definitions)
                )
            )
            .condition_catalog(
                dispatcher::alarm::AlarmConditionCatalog(
                    std::move(condition_definitions)
                )
            )
            .build();
    }
}

int main(int argc, char** argv)
{
    const auto tag_count = parse_tag_count(argc, argv);

    spdlog::info("Dispatcher E2E smoke application");
    spdlog::info("Requested tag count: {}", tag_count);

    const auto configuration_snapshot = make_configuration_snapshot(tag_count);
    const auto validation_result =
        dispatcher::domain::validate_configuration_snapshot(
            configuration_snapshot
        );

    if (validation_result.has_errors())
    {
        spdlog::error("Configuration snapshot validation failed");

        for (const auto& error : validation_result.errors())
        {
            spdlog::error("{}: {}", error.field, error.message);
        }

        return EXIT_FAILURE;
    }

    spdlog::info(
        "Configuration snapshot is valid. Version: {}, status: {}",
        configuration_snapshot.config_version(),
        dispatcher::domain::to_string(configuration_snapshot.status())
    );

    dispatcher::simulator::TelemetryGenerator generator(tag_count);

    dispatcher::runtime::DispatcherRuntime runtime(configuration_snapshot);

    const auto alarm_configuration_snapshot =
        make_simulated_alarm_configuration_snapshot(
            tag_count,
            configuration_snapshot.config_version()
        );

    const auto alarm_reload_result =
        runtime.reload_alarm_configuration(alarm_configuration_snapshot);

    if (alarm_reload_result.has_errors())
    {
        for (const auto& error : alarm_reload_result.errors())
        {
            spdlog::error(
                "Alarm configuration reload failed. Field: {}, Error: {}",
                error.field,
                error.message
            );
        }

        return EXIT_FAILURE;
    }

    spdlog::info(
        "Dispatcher runtime loaded. Telemetry version: {}, alarm version: {}, "
        "alarms: {}, conditions: {}",
        runtime.telemetry_ingestor().configuration_snapshot().config_version(),
        runtime.alarm_runtime().configuration_snapshot().config_version(),
        runtime.alarm_runtime().configuration_snapshot().alarm_catalog().size(),
        runtime.alarm_runtime()
        .configuration_snapshot()
        .condition_catalog()
        .size()
    );

    const auto batch = generator.next_batch();

    const auto runtime_batch_summary = runtime.process_batch(batch);
    const auto runtime_snapshot_after_batch = runtime.runtime_snapshot();

    spdlog::info("Generated telemetry values: {}", batch.size());

    spdlog::info(
        "Runtime batch summary: total={}, telemetry_accepted={}, "
        "telemetry_stored={}, telemetry_no_change={}, telemetry_rejected={}, "
        "history_written={}, history_skipped={}, configured_alarms={}, "
        "missing_conditions={}, alarm_total={}, alarm_evaluated={}, "
        "alarm_skipped={}, alarm_activated={}, alarm_acknowledged={}, "
        "alarm_cleared={}, alarm_events={}",
        runtime_batch_summary.total_count,
        runtime_batch_summary.telemetry_accepted_count,
        runtime_batch_summary.telemetry_stored_count,
        runtime_batch_summary.telemetry_no_change_count,
        runtime_batch_summary.telemetry_rejected_count,
        runtime_batch_summary.history_written_count,
        runtime_batch_summary.history_skipped_count,
        runtime_batch_summary.configured_alarm_count,
        runtime_batch_summary.missing_condition_count,
        runtime_batch_summary.alarm_total_count,
        runtime_batch_summary.alarm_evaluated_count,
        runtime_batch_summary.alarm_skipped_count,
        runtime_batch_summary.alarm_activated_count,
        runtime_batch_summary.alarm_acknowledged_count,
        runtime_batch_summary.alarm_cleared_count,
        runtime_batch_summary.alarm_stored_event_count
    );

    spdlog::info(
        "Runtime snapshot after batch: telemetry_config_version={}, "
        "telemetry_current_state={}, telemetry_total={}, history_samples={}, "
        "history_written={}, history_skipped_not_stored={}, "
        "history_skipped_by_policy={}, history_skipped={}, history_total={}, "
        "alarm_states={}, alarm_events={}, acknowledgement_records={}, "
        "alarm_total={}, alarm_evaluated={}, alarm_skipped={}, "
        "alarm_activated={}, alarm_acknowledged={}, alarm_cleared={}, "
        "alarm_no_transition={}, alarm_stored_events={}, operator_active={}, "
        "operator_acknowledged={}, operator_unacknowledged={}, "
        "requires_operator_attention={}",
        runtime_snapshot_after_batch.telemetry.configuration_version,
        runtime_snapshot_after_batch.telemetry.current_state_size,
        runtime_snapshot_after_batch.telemetry.total_count,
        runtime_snapshot_after_batch.history.store_size,
        runtime_snapshot_after_batch.history.written_count,
        runtime_snapshot_after_batch.history.skipped_not_stored_count,
        runtime_snapshot_after_batch.history.skipped_by_policy_count,
        runtime_snapshot_after_batch.history.skipped_count,
        runtime_snapshot_after_batch.history.total_write_count,
        runtime_snapshot_after_batch.alarm.state_store_size,
        runtime_snapshot_after_batch.alarm.event_store_size,
        runtime_snapshot_after_batch.alarm.acknowledgement_store_size,
        runtime_snapshot_after_batch.alarm.total_count,
        runtime_snapshot_after_batch.alarm.evaluated_count,
        runtime_snapshot_after_batch.alarm.skipped_count,
        runtime_snapshot_after_batch.alarm.activated_count,
        runtime_snapshot_after_batch.alarm.acknowledged_count,
        runtime_snapshot_after_batch.alarm.cleared_count,
        runtime_snapshot_after_batch.alarm.no_transition_count,
        runtime_snapshot_after_batch.alarm.stored_event_count,
        runtime_snapshot_after_batch.alarm_operator.active_alarm_count,
        runtime_snapshot_after_batch.alarm_operator.acknowledged_alarm_count,
        runtime_snapshot_after_batch.alarm_operator.unacknowledged_alarm_count,
        runtime_snapshot_after_batch.requires_operator_attention()
    );

    spdlog::info("Generator sequence: {}", generator.sequence());

    if (runtime_batch_summary.total_count != tag_count)
    {
        spdlog::error(
            "Runtime batch total count mismatch. Expected: {}, Actual: {}",
            tag_count,
            runtime_batch_summary.total_count
        );

        return EXIT_FAILURE;
    }

    if (runtime_batch_summary.telemetry_accepted_count != tag_count)
    {
        spdlog::error(
            "Accepted telemetry count mismatch. Expected: {}, Actual: {}",
            tag_count,
            runtime_batch_summary.telemetry_accepted_count
        );

        return EXIT_FAILURE;
    }

    if (runtime_batch_summary.telemetry_rejected_count != 0)
    {
        spdlog::error(
            "Unexpected rejected telemetry values. Rejected: {}",
            runtime_batch_summary.telemetry_rejected_count
        );

        return EXIT_FAILURE;
    }

    if (runtime_batch_summary.telemetry_stored_count != tag_count)
    {
        spdlog::error(
            "Stored telemetry count mismatch. Expected: {}, Actual: {}",
            tag_count,
            runtime_batch_summary.telemetry_stored_count
        );

        return EXIT_FAILURE;
    }

    if (runtime_batch_summary.history_written_count != tag_count)
    {
        spdlog::error(
            "History written count mismatch. Expected: {}, Actual: {}",
            tag_count,
            runtime_batch_summary.history_written_count
        );

        return EXIT_FAILURE;
    }

    if (runtime_batch_summary.history_skipped_count != 0)
    {
        spdlog::error(
            "Unexpected skipped history values. Skipped: {}",
            runtime_batch_summary.history_skipped_count
        );

        return EXIT_FAILURE;
    }

    if (runtime_batch_summary.configured_alarm_count != tag_count)
    {
        spdlog::error(
            "Alarm configured count mismatch. Expected: {}, Actual: {}",
            tag_count,
            runtime_batch_summary.configured_alarm_count
        );

        return EXIT_FAILURE;
    }

    if (runtime_batch_summary.missing_condition_count != 0)
    {
        spdlog::error(
            "Unexpected alarm missing condition count. Count: {}",
            runtime_batch_summary.missing_condition_count
        );

        return EXIT_FAILURE;
    }

    if (runtime_batch_summary.alarm_total_count != tag_count)
    {
        spdlog::error(
            "Alarm batch total count mismatch. Expected: {}, Actual: {}",
            tag_count,
            runtime_batch_summary.alarm_total_count
        );

        return EXIT_FAILURE;
    }

    if (runtime_batch_summary.alarm_evaluated_count != tag_count)
    {
        spdlog::error(
            "Alarm evaluated count mismatch. Expected: {}, Actual: {}",
            tag_count,
            runtime_batch_summary.alarm_evaluated_count
        );

        return EXIT_FAILURE;
    }

    if (runtime_batch_summary.alarm_skipped_count != 0)
    {
        spdlog::error(
            "Unexpected skipped alarm evaluations. Skipped: {}",
            runtime_batch_summary.alarm_skipped_count
        );

        return EXIT_FAILURE;
    }

    if (runtime_batch_summary.alarm_activated_count != tag_count)
    {
        spdlog::error(
            "Alarm activated count mismatch. Expected: {}, Actual: {}",
            tag_count,
            runtime_batch_summary.alarm_activated_count
        );

        return EXIT_FAILURE;
    }

    if (runtime_batch_summary.alarm_stored_event_count != tag_count)
    {
        spdlog::error(
            "Alarm stored event count mismatch. Expected: {}, Actual: {}",
            tag_count,
            runtime_batch_summary.alarm_stored_event_count
        );

        return EXIT_FAILURE;
    }

    if (runtime_snapshot_after_batch.telemetry.current_state_size != tag_count)
    {
        spdlog::error(
            "Current state size mismatch. Expected: {}, Actual: {}",
            tag_count,
            runtime_snapshot_after_batch.telemetry.current_state_size
        );

        return EXIT_FAILURE;
    }

    if (runtime_snapshot_after_batch.history.store_size != tag_count)
    {
        spdlog::error(
            "History store size mismatch. Expected: {}, Actual: {}",
            tag_count,
            runtime_snapshot_after_batch.history.store_size
        );

        return EXIT_FAILURE;
    }

    if (runtime_snapshot_after_batch.alarm.state_store_size != tag_count)
    {
        spdlog::error(
            "Alarm state store size mismatch. Expected: {}, Actual: {}",
            tag_count,
            runtime_snapshot_after_batch.alarm.state_store_size
        );

        return EXIT_FAILURE;
    }

    if (runtime_snapshot_after_batch.alarm.event_store_size != tag_count)
    {
        spdlog::error(
            "Alarm event store size mismatch. Expected: {}, Actual: {}",
            tag_count,
            runtime_snapshot_after_batch.alarm.event_store_size
        );

        return EXIT_FAILURE;
    }

    const auto operator_snapshot_after_activation =
        runtime.alarm_operator_snapshot();

    if (operator_snapshot_after_activation.configured_alarm_count != tag_count)
    {
        spdlog::error(
            "Expected operator snapshot configured alarm count {}, got {}",
            tag_count,
            operator_snapshot_after_activation.configured_alarm_count
        );

        return EXIT_FAILURE;
    }

    if (operator_snapshot_after_activation.active_alarm_count != tag_count)
    {
        spdlog::error(
            "Expected operator snapshot active alarm count {}, got {}",
            tag_count,
            operator_snapshot_after_activation.active_alarm_count
        );

        return EXIT_FAILURE;
    }

    if (operator_snapshot_after_activation.unacknowledged_alarm_count
        != tag_count)
    {
        spdlog::error(
            "Expected operator snapshot unacknowledged alarm count {}, got {}",
            tag_count,
            operator_snapshot_after_activation.unacknowledged_alarm_count
        );

        return EXIT_FAILURE;
    }

    if (!operator_snapshot_after_activation.requires_operator_attention())
    {
        spdlog::error(
            "Expected operator snapshot to require attention after activation"
        );

        return EXIT_FAILURE;
    }

    const auto unacknowledged_alarms =
        runtime.unacknowledged_alarms();

    if (unacknowledged_alarms.empty())
    {
        spdlog::error(
            "Expected at least one unacknowledged alarm after activation"
        );

        return EXIT_FAILURE;
    }

    const auto alarm_to_acknowledge = unacknowledged_alarms.front();

    const dispatcher::alarm::AlarmAcknowledgementCommand acknowledgement_command(
        alarm_to_acknowledge.alarm_id(),
        "operator-e2e",
        "E2E operator acknowledgement"
    );

    const auto acknowledgement_result =
        runtime.acknowledge_alarm(acknowledgement_command);

    if (!acknowledgement_result.acknowledged())
    {
        spdlog::error(
            "Expected acknowledgement to succeed, got status={}",
            dispatcher::alarm::to_string(
                acknowledgement_result.status()
            )
        );

        return EXIT_FAILURE;
    }

    if (runtime.alarm_runtime().acknowledgement_store().size() != 1)
    {
        spdlog::error(
            "Expected acknowledgement audit store size 1, got {}",
            runtime.alarm_runtime().acknowledgement_store().size()
        );

        return EXIT_FAILURE;
    }

    const auto operator_snapshot_after_acknowledgement =
        runtime.alarm_operator_snapshot();

    const auto expected_unacknowledged_after_acknowledgement =
        tag_count - 1;

    if (operator_snapshot_after_acknowledgement.acknowledged_alarm_count != 1)
    {
        spdlog::error(
            "Expected acknowledged alarm count 1 after acknowledgement, got {}",
            operator_snapshot_after_acknowledgement.acknowledged_alarm_count
        );

        return EXIT_FAILURE;
    }

    if (operator_snapshot_after_acknowledgement.unacknowledged_alarm_count
        != expected_unacknowledged_after_acknowledgement)
    {
        spdlog::error(
            "Expected unacknowledged alarm count {} after acknowledgement, "
            "got {}",
            expected_unacknowledged_after_acknowledgement,
            operator_snapshot_after_acknowledgement.unacknowledged_alarm_count
        );

        return EXIT_FAILURE;
    }

    if (runtime.alarm_runtime().event_store().size() != tag_count + 1)
    {
        spdlog::error(
            "Expected alarm event store size {} after acknowledgement, got {}",
            tag_count + 1,
            runtime.alarm_runtime().event_store().size()
        );

        return EXIT_FAILURE;
    }

    const auto clearing_telemetry_value = make_clearing_telemetry_value(
        alarm_to_acknowledge,
        tag_count + 1
    );

    const auto clearing_summary = runtime.process(clearing_telemetry_value);

    if (!clearing_summary.telemetry_stored())
    {
        spdlog::error(
            "Expected clearing telemetry value to be stored, got status={}",
            dispatcher::core::to_string(clearing_summary.telemetry_status)
        );

        return EXIT_FAILURE;
    }

    if (!clearing_summary.history_written())
    {
        spdlog::error(
            "Expected clearing telemetry value to be written to history, "
            "got history_status={}",
            static_cast<int>(clearing_summary.history_status)
        );

        return EXIT_FAILURE;
    }

    if (clearing_summary.alarm_cleared_count != 1)
    {
        spdlog::error(
            "Expected clearing alarm result cleared_count 1, got {}",
            clearing_summary.alarm_cleared_count
        );

        return EXIT_FAILURE;
    }

    const auto operator_snapshot_after_clear =
        runtime.alarm_operator_snapshot();

    if (operator_snapshot_after_clear.normal_alarm_count != 1)
    {
        spdlog::error(
            "Expected normal alarm count 1 after clear, got {}",
            operator_snapshot_after_clear.normal_alarm_count
        );

        return EXIT_FAILURE;
    }

    if (operator_snapshot_after_clear.acknowledged_alarm_count != 0)
    {
        spdlog::error(
            "Expected acknowledged alarm count 0 after clear, got {}",
            operator_snapshot_after_clear.acknowledged_alarm_count
        );

        return EXIT_FAILURE;
    }

    if (operator_snapshot_after_clear.unacknowledged_alarm_count
        != expected_unacknowledged_after_acknowledgement)
    {
        spdlog::error(
            "Expected unacknowledged alarm count {} after clear, got {}",
            expected_unacknowledged_after_acknowledgement,
            operator_snapshot_after_clear.unacknowledged_alarm_count
        );

        return EXIT_FAILURE;
    }

    if (runtime.alarm_runtime().event_store().size() != tag_count + 2)
    {
        spdlog::error(
            "Expected alarm event store size {} after clear, got {}",
            tag_count + 2,
            runtime.alarm_runtime().event_store().size()
        );

        return EXIT_FAILURE;
    }

    if (runtime.alarm_runtime().statistics().acknowledged_count() != 1)
    {
        spdlog::error(
            "Expected acknowledged statistics count 1, got {}",
            runtime.alarm_runtime().statistics().acknowledged_count()
        );

        return EXIT_FAILURE;
    }

    if (runtime.alarm_runtime().statistics().cleared_count() != 1)
    {
        spdlog::error(
            "Expected cleared statistics count 1, got {}",
            runtime.alarm_runtime().statistics().cleared_count()
        );

        return EXIT_FAILURE;
    }

    const auto final_runtime_snapshot = runtime.runtime_snapshot();

    if (final_runtime_snapshot.history.store_size != tag_count + 1)
    {
        spdlog::error(
            "Expected final history store size {} after clear, got {}",
            tag_count + 1,
            final_runtime_snapshot.history.store_size
        );

        return EXIT_FAILURE;
    }

    if (final_runtime_snapshot.alarm.event_store_size != tag_count + 2)
    {
        spdlog::error(
            "Expected final alarm event store size {} after clear, got {}",
            tag_count + 2,
            final_runtime_snapshot.alarm.event_store_size
        );

        return EXIT_FAILURE;
    }

    if (final_runtime_snapshot.alarm.acknowledgement_store_size != 1)
    {
        spdlog::error(
            "Expected final acknowledgement audit store size 1, got {}",
            final_runtime_snapshot.alarm.acknowledgement_store_size
        );

        return EXIT_FAILURE;
    }

    spdlog::info(
        "Alarm operator workflow check passed. "
        "acknowledged_alarm_id={}, audit_records={}, events={}",
        alarm_to_acknowledge.alarm_id().value(),
        runtime.alarm_runtime().acknowledgement_store().size(),
        runtime.alarm_runtime().event_store().size()
    );

    spdlog::info("E2E ingest smoke check passed");

    return EXIT_SUCCESS;
}