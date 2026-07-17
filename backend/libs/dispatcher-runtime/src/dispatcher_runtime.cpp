#include <dispatcher/runtime/dispatcher_runtime.hpp>

#include <dispatcher/domain/configuration_snapshot_builder.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/history_policy.hpp>
#include <dispatcher/history/history_write_batch_result.hpp>
#include <dispatcher/history/history_write_candidate.hpp>
#include <dispatcher/storage/storage_result.hpp>

#include <utility>
#include <vector>

namespace
{
    dispatcher::domain::ConfigurationSnapshot make_default_telemetry_configuration()
    {
        return dispatcher::domain::ConfigurationSnapshotBuilder{}
            .status(dispatcher::domain::ConfigurationStatus::Published)
            .build();
    }

    dispatcher::domain::HistoryPolicy find_history_policy(
        const dispatcher::domain::ConfigurationSnapshot& configuration_snapshot,
        const dispatcher::domain::TagId& tag_id
    )
    {
        const auto tag_definition = configuration_snapshot.find_tag_by_id(tag_id);

        if (!tag_definition.has_value())
        {
            return dispatcher::domain::HistoryPolicy::Disabled;
        }

        return tag_definition->history_policy();
    }

    dispatcher::history::HistoryWriteStatus summarize_history_status(
        const dispatcher::history::HistoryWriteBatchResult& batch_result
    )
    {
        if (batch_result.written_count() > 0)
        {
            return dispatcher::history::HistoryWriteStatus::Written;
        }

        if (batch_result.skipped_by_policy_count() > 0)
        {
            return dispatcher::history::HistoryWriteStatus::SkippedByPolicy;
        }

        return dispatcher::history::HistoryWriteStatus::SkippedNotStored;
    }

    dispatcher::storage::StorageResult persist_telemetry_configuration_if_available(
        dispatcher::storage::StorageRepository* storage_repository,
        const dispatcher::domain::ConfigurationSnapshot& snapshot
    )
    {
        if (storage_repository == nullptr)
        {
            return dispatcher::storage::StorageResult::success();
        }

        return storage_repository->configuration_storage().save(snapshot);
    }
}

namespace dispatcher::runtime
{
    DispatcherRuntime::DispatcherRuntime()
        : telemetry_ingestor_(make_default_telemetry_configuration())
        , history_store_()
        , history_writer_(history_store_)
        , alarm_runtime_()
    {
    }

    DispatcherRuntime::DispatcherRuntime(
        dispatcher::storage::StorageRepository& storage_repository
    )
        : telemetry_ingestor_(make_default_telemetry_configuration())
        , history_store_()
        , history_writer_(history_store_)
        , alarm_runtime_()
        , storage_repository_(&storage_repository)
    {
        const auto persist_result =
            persist_telemetry_configuration_if_available(
                storage_repository_,
                telemetry_ingestor_.configuration_snapshot()
            );

        (void)persist_result;
    }

    DispatcherRuntime::DispatcherRuntime(
        dispatcher::domain::ConfigurationSnapshot telemetry_configuration
    )
        : telemetry_ingestor_(std::move(telemetry_configuration))
        , history_store_()
        , history_writer_(history_store_)
        , alarm_runtime_()
    {
    }

    DispatcherRuntime::DispatcherRuntime(
        dispatcher::domain::ConfigurationSnapshot telemetry_configuration,
        dispatcher::storage::StorageRepository& storage_repository
    )
        : telemetry_ingestor_(std::move(telemetry_configuration))
        , history_store_()
        , history_writer_(history_store_)
        , alarm_runtime_()
        , storage_repository_(&storage_repository)
    {
        const auto persist_result =
            persist_telemetry_configuration_if_available(
                storage_repository_,
                telemetry_ingestor_.configuration_snapshot()
            );

        (void)persist_result;
    }

    DispatcherRuntime::DispatcherRuntime(
        dispatcher::domain::ConfigurationSnapshot telemetry_configuration,
        dispatcher::alarm::AlarmConfigurationSnapshot alarm_configuration
    )
        : telemetry_ingestor_(std::move(telemetry_configuration))
        , history_store_()
        , history_writer_(history_store_)
        , alarm_runtime_()
    {
        const auto reload_result = alarm_runtime_.reload_configuration(
            std::move(alarm_configuration)
        );

        (void)reload_result;
    }

    DispatcherRuntime::DispatcherRuntime(
        dispatcher::domain::ConfigurationSnapshot telemetry_configuration,
        dispatcher::alarm::AlarmConfigurationSnapshot alarm_configuration,
        dispatcher::storage::StorageRepository& storage_repository
    )
        : telemetry_ingestor_(std::move(telemetry_configuration))
        , history_store_()
        , history_writer_(history_store_)
        , alarm_runtime_()
        , storage_repository_(&storage_repository)
    {
        const auto persist_result =
            persist_telemetry_configuration_if_available(
                storage_repository_,
                telemetry_ingestor_.configuration_snapshot()
            );

        (void)persist_result;

        const auto reload_result = alarm_runtime_.reload_configuration(
            std::move(alarm_configuration)
        );

        (void)reload_result;
    }

    const dispatcher::core::TelemetryIngestor&
        DispatcherRuntime::telemetry_ingestor() const noexcept
    {
        return telemetry_ingestor_;
    }

    dispatcher::core::TelemetryIngestor&
        DispatcherRuntime::telemetry_ingestor() noexcept
    {
        return telemetry_ingestor_;
    }

    const dispatcher::history::InMemoryHistoryStore&
        DispatcherRuntime::history_store() const noexcept
    {
        return history_store_;
    }

    dispatcher::history::InMemoryHistoryStore&
        DispatcherRuntime::history_store() noexcept
    {
        return history_store_;
    }

    const dispatcher::history::HistoryWriter&
        DispatcherRuntime::history_writer() const noexcept
    {
        return history_writer_;
    }

    dispatcher::history::HistoryWriter&
        DispatcherRuntime::history_writer() noexcept
    {
        return history_writer_;
    }

    const dispatcher::alarm::AlarmRuntime& DispatcherRuntime::alarm_runtime()
        const noexcept
    {
        return alarm_runtime_;
    }

    dispatcher::alarm::AlarmRuntime& DispatcherRuntime::alarm_runtime() noexcept
    {
        return alarm_runtime_;
    }

    bool DispatcherRuntime::has_storage_repository() const noexcept
    {
        return storage_repository_ != nullptr;
    }

    dispatcher::storage::StorageRepository*
        DispatcherRuntime::storage_repository() noexcept
    {
        return storage_repository_;
    }

    const dispatcher::storage::StorageRepository*
        DispatcherRuntime::storage_repository() const noexcept
    {
        return storage_repository_;
    }

    DispatcherRuntimeSnapshot DispatcherRuntime::runtime_snapshot()
        const noexcept
    {
        return DispatcherRuntimeSnapshot{
            .telemetry = telemetry_ingestor_.runtime_snapshot(),
            .history = history_writer_.runtime_snapshot(),
            .alarm = alarm_runtime_.runtime_snapshot(),
            .alarm_operator = alarm_runtime_.operator_snapshot()
        };
    }

    DispatcherRuntimeProcessSummary DispatcherRuntime::process(
        dispatcher::telemetry::TelemetryValue telemetry_value
    )
    {
        DispatcherRuntimeProcessSummary summary;

        const auto ingest_result = telemetry_ingestor_.ingest(telemetry_value);

        summary.telemetry_status = ingest_result.status();

        const auto history_policy = find_history_policy(
            telemetry_ingestor_.configuration_snapshot(),
            telemetry_value.tag_id()
        );

        const auto history_batch_result = history_writer_.write_batch_if_stored(
            std::vector<dispatcher::history::HistoryWriteCandidate>{
            dispatcher::history::HistoryWriteCandidate{
                ingest_result,
                telemetry_value,
                history_policy
            }
        }
        );

        summary.history_status = summarize_history_status(history_batch_result);

        if (!ingest_result.stored())
        {
            return summary;
        }

        const auto alarm_result = alarm_runtime_.evaluate_configured(
            telemetry_value
        );

        summary.configured_alarm_count =
            alarm_result.configured_alarm_count();

        summary.missing_condition_count =
            alarm_result.missing_condition_count();

        summary.alarm_total_count =
            alarm_result.batch_result().total_count();

        summary.alarm_evaluated_count =
            alarm_result.batch_result().evaluated_count();

        summary.alarm_skipped_count =
            alarm_result.batch_result().skipped_count();

        summary.alarm_activated_count =
            alarm_result.batch_result().activated_count();

        summary.alarm_acknowledged_count =
            alarm_result.batch_result().acknowledged_count();

        summary.alarm_cleared_count =
            alarm_result.batch_result().cleared_count();

        summary.alarm_stored_event_count =
            alarm_result.batch_result().stored_event_count();

        return summary;
    }

    DispatcherRuntimeBatchSummary DispatcherRuntime::process_batch(
        std::vector<dispatcher::telemetry::TelemetryValue> telemetry_values
    )
    {
        DispatcherRuntimeBatchSummary batch_summary;

        for (auto& telemetry_value : telemetry_values)
        {
            const auto summary = process(std::move(telemetry_value));

            batch_summary.record(summary);
        }

        return batch_summary;
    }

    dispatcher::alarm::AlarmOperatorSnapshot
        DispatcherRuntime::alarm_operator_snapshot() const noexcept
    {
        return alarm_runtime_.operator_snapshot();
    }

    decltype(
        std::declval<const dispatcher::alarm::AlarmRuntime&>()
        .unacknowledged_alarms()
        )
        DispatcherRuntime::unacknowledged_alarms() const
    {
        return alarm_runtime_.unacknowledged_alarms();
    }

    dispatcher::alarm::AlarmAcknowledgementResult
        DispatcherRuntime::acknowledge_alarm(
            dispatcher::alarm::AlarmAcknowledgementCommand command
        )
    {
        return alarm_runtime_.acknowledge(std::move(command));
    }

    dispatcher::common::ValidationResult
        DispatcherRuntime::reload_telemetry_configuration(
            dispatcher::domain::ConfigurationSnapshot snapshot
        )
    {
        const auto reload_result =
            telemetry_ingestor_.reload_configuration(std::move(snapshot));

        if (reload_result.has_errors())
        {
            return reload_result;
        }

        const auto persist_result =
            persist_telemetry_configuration_if_available(
                storage_repository_,
                telemetry_ingestor_.configuration_snapshot()
            );

        (void)persist_result;

        return reload_result;
    }

    dispatcher::common::ValidationResult
        DispatcherRuntime::reload_alarm_configuration(
            dispatcher::alarm::AlarmConfigurationSnapshot snapshot
        )
    {
        return alarm_runtime_.reload_configuration(std::move(snapshot));
    }
}