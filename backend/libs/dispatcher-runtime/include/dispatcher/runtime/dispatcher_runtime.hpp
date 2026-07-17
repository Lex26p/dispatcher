#pragma once

#include <dispatcher/alarm/alarm_configuration_snapshot.hpp>
#include <dispatcher/alarm/alarm_runtime.hpp>
#include <dispatcher/common/validation_result.hpp>
#include <dispatcher/core/telemetry_ingestor.hpp>
#include <dispatcher/domain/configuration_snapshot.hpp>
#include <dispatcher/history/history_writer.hpp>
#include <dispatcher/history/in_memory_history_store.hpp>
#include <dispatcher/runtime/dispatcher_runtime_batch_summary.hpp>
#include <dispatcher/runtime/dispatcher_runtime_process_summary.hpp>
#include <dispatcher/runtime/dispatcher_runtime_snapshot.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>
#include <dispatcher/alarm/alarm_acknowledgement_command.hpp>
#include <dispatcher/alarm/alarm_acknowledgement_result.hpp>
#include <dispatcher/alarm/alarm_operator_snapshot.hpp>
#include <dispatcher/storage/storage_repository.hpp>

#include <utility>
#include <vector>

namespace dispatcher::runtime
{
    class DispatcherRuntime
    {
    public:
        DispatcherRuntime();

        explicit DispatcherRuntime(
            dispatcher::storage::StorageRepository& storage_repository
        );

        explicit DispatcherRuntime(
            dispatcher::domain::ConfigurationSnapshot telemetry_configuration
        );

        DispatcherRuntime(
            dispatcher::domain::ConfigurationSnapshot telemetry_configuration,
            dispatcher::storage::StorageRepository& storage_repository
        );

        DispatcherRuntime(
            dispatcher::domain::ConfigurationSnapshot telemetry_configuration,
            dispatcher::alarm::AlarmConfigurationSnapshot alarm_configuration
        );

        DispatcherRuntime(
            dispatcher::domain::ConfigurationSnapshot telemetry_configuration,
            dispatcher::alarm::AlarmConfigurationSnapshot alarm_configuration,
            dispatcher::storage::StorageRepository& storage_repository
        );

        [[nodiscard]] const dispatcher::core::TelemetryIngestor&
            telemetry_ingestor() const noexcept;

        [[nodiscard]] dispatcher::core::TelemetryIngestor&
            telemetry_ingestor() noexcept;

        [[nodiscard]] const dispatcher::history::InMemoryHistoryStore&
            history_store() const noexcept;

        [[nodiscard]] dispatcher::history::InMemoryHistoryStore&
            history_store() noexcept;

        [[nodiscard]] const dispatcher::history::HistoryWriter&
            history_writer() const noexcept;

        [[nodiscard]] dispatcher::history::HistoryWriter&
            history_writer() noexcept;

        [[nodiscard]] const dispatcher::alarm::AlarmRuntime& alarm_runtime()
            const noexcept;

        [[nodiscard]] dispatcher::alarm::AlarmRuntime& alarm_runtime() noexcept;

        [[nodiscard]] bool has_storage_repository() const noexcept;

        [[nodiscard]] dispatcher::storage::StorageRepository*
            storage_repository() noexcept;

        [[nodiscard]] const dispatcher::storage::StorageRepository*
            storage_repository() const noexcept;

        [[nodiscard]] DispatcherRuntimeSnapshot runtime_snapshot() const noexcept;

        [[nodiscard]] DispatcherRuntimeProcessSummary process(
            dispatcher::telemetry::TelemetryValue telemetry_value
        );

        [[nodiscard]] DispatcherRuntimeBatchSummary process_batch(
            std::vector<dispatcher::telemetry::TelemetryValue> telemetry_values
        );

        [[nodiscard]] dispatcher::alarm::AlarmOperatorSnapshot
            alarm_operator_snapshot() const noexcept;

        [[nodiscard]] decltype(
            std::declval<const dispatcher::alarm::AlarmRuntime&>()
            .unacknowledged_alarms()
            )
            unacknowledged_alarms() const;

        [[nodiscard]] dispatcher::alarm::AlarmAcknowledgementResult
            acknowledge_alarm(
                dispatcher::alarm::AlarmAcknowledgementCommand command
            );

        [[nodiscard]] dispatcher::common::ValidationResult reload_telemetry_configuration(
            dispatcher::domain::ConfigurationSnapshot snapshot
        );

        [[nodiscard]] dispatcher::common::ValidationResult reload_alarm_configuration(
            dispatcher::alarm::AlarmConfigurationSnapshot snapshot
        );

    private:
        dispatcher::core::TelemetryIngestor telemetry_ingestor_;
        dispatcher::history::InMemoryHistoryStore history_store_;
        dispatcher::history::HistoryWriter history_writer_;
        dispatcher::alarm::AlarmRuntime alarm_runtime_;
        dispatcher::storage::StorageRepository* storage_repository_{ nullptr };
    };
}