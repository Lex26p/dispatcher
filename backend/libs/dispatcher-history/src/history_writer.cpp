#include <dispatcher/history/history_writer.hpp>

#include <utility>

namespace dispatcher::history
{
    HistoryWriter::HistoryWriter(InMemoryHistoryStore& store)
        : store_(store)
    {
    }

    bool HistoryWriter::should_write_history(
        dispatcher::domain::HistoryPolicy history_policy
    ) noexcept
    {
        using dispatcher::domain::HistoryPolicy;

        switch (history_policy)
        {
        case HistoryPolicy::Disabled:
        case HistoryPolicy::LiveOnly:
            return false;

        case HistoryPolicy::OnChange:
        case HistoryPolicy::OnChangeWithForcedSample:
        case HistoryPolicy::EveryPoll:
        case HistoryPolicy::CriticalLossless:
        case HistoryPolicy::DiagnosticBestEffort:
            return true;
        }

        return false;
    }

    HistoryWriteResult HistoryWriter::write_if_stored(
        const dispatcher::core::TelemetryIngestResult& ingest_result,
        dispatcher::telemetry::TelemetryValue telemetry_value,
        dispatcher::domain::HistoryPolicy history_policy
    )
    {
        if (!ingest_result.stored())
        {
            statistics_.record_skipped_not_stored();

            return HistoryWriteResult(
                HistoryWriteStatus::SkippedNotStored
            );
        }

        if (!should_write_history(history_policy))
        {
            statistics_.record_skipped_by_policy();

            return HistoryWriteResult(
                HistoryWriteStatus::SkippedByPolicy
            );
        }

        store_.append_telemetry(std::move(telemetry_value));
        statistics_.record_written();

        return HistoryWriteResult(
            HistoryWriteStatus::Written
        );
    }

    HistoryWriteBatchResult HistoryWriter::write_batch_if_stored(
        std::vector<HistoryWriteCandidate> candidates
    )
    {
        HistoryWriteBatchResult batch_result;

        for (auto& candidate : candidates)
        {
            const auto result = write_if_stored(
                candidate.ingest_result,
                std::move(candidate.telemetry_value),
                candidate.history_policy
            );

            batch_result.record(result.status());
        }

        return batch_result;
    }

    const InMemoryHistoryStore& HistoryWriter::store() const noexcept
    {
        return store_;
    }

    const HistoryStatistics& HistoryWriter::statistics() const noexcept
    {
        return statistics_;
    }

    HistoryRuntimeSnapshot HistoryWriter::runtime_snapshot() const noexcept
    {
        const auto max_samples = store_.max_samples();

        return HistoryRuntimeSnapshot{
            .store_size = store_.size(),

            .max_samples_enabled = max_samples.has_value(),
            .max_samples = max_samples.value_or(0),

            .retained_sample_count = store_.retained_sample_count(),

            .written_count = statistics_.written_count(),
            .skipped_not_stored_count = statistics_.skipped_not_stored_count(),
            .skipped_by_policy_count = statistics_.skipped_by_policy_count(),
            .skipped_count = statistics_.skipped_count(),
            .total_write_count = statistics_.total_count()
        };
    }

    void HistoryWriter::reset_statistics() noexcept
    {
        statistics_.reset();
    }
}