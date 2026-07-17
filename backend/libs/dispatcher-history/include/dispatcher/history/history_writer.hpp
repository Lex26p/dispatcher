#pragma once

#include <dispatcher/core/telemetry_ingest_result.hpp>
#include <dispatcher/domain/history_policy.hpp>
#include <dispatcher/history/history_runtime_snapshot.hpp>
#include <dispatcher/history/history_statistics.hpp>
#include <dispatcher/history/history_write_batch_result.hpp>
#include <dispatcher/history/history_write_candidate.hpp>
#include <dispatcher/history/history_write_result.hpp>
#include <dispatcher/history/in_memory_history_store.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <vector>

namespace dispatcher::history
{
    class HistoryWriter
    {
    public:
        explicit HistoryWriter(InMemoryHistoryStore& store);

        [[nodiscard]] HistoryWriteResult write_if_stored(
            const dispatcher::core::TelemetryIngestResult& ingest_result,
            dispatcher::telemetry::TelemetryValue telemetry_value,
            dispatcher::domain::HistoryPolicy history_policy
        );

        [[nodiscard]] HistoryWriteBatchResult write_batch_if_stored(
            std::vector<HistoryWriteCandidate> candidates
        );

        [[nodiscard]] const InMemoryHistoryStore& store() const noexcept;

        [[nodiscard]] const HistoryStatistics& statistics() const noexcept;

        [[nodiscard]] HistoryRuntimeSnapshot runtime_snapshot() const noexcept;

        void reset_statistics() noexcept;

    private:
        [[nodiscard]] static bool should_write_history(
            dispatcher::domain::HistoryPolicy history_policy
        ) noexcept;

        InMemoryHistoryStore& store_;
        HistoryStatistics statistics_;
    };
}