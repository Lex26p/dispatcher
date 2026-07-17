#pragma once

#include <dispatcher/common/validation_result.hpp>
#include <dispatcher/core/current_state_store.hpp>
#include <dispatcher/core/telemetry_ingest_batch_result.hpp>
#include <dispatcher/core/telemetry_ingest_result.hpp>
#include <dispatcher/core/telemetry_ingest_statistics.hpp>
#include <dispatcher/core/telemetry_runtime_snapshot.hpp>
#include <dispatcher/domain/configuration_snapshot.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace dispatcher::core
{
    class TelemetryIngestor
    {
    public:
        explicit TelemetryIngestor(
            dispatcher::domain::ConfigurationSnapshot configuration_snapshot
        );

        TelemetryIngestor(
            dispatcher::domain::ConfigurationSnapshot configuration_snapshot,
            std::chrono::milliseconds max_future_source_timestamp_skew
        );

        [[nodiscard]] const dispatcher::domain::ConfigurationSnapshot& configuration_snapshot()
            const noexcept;

        [[nodiscard]] TelemetryRuntimeSnapshot runtime_snapshot() const noexcept;

        [[nodiscard]] const CurrentStateStore& current_state() const noexcept;

        [[nodiscard]] const TelemetryIngestStatistics& statistics() const noexcept;

        [[nodiscard]] std::chrono::milliseconds max_future_source_timestamp_skew()
            const noexcept;

        [[nodiscard]] std::optional<std::uint64_t> last_sequence(
            const dispatcher::domain::TagId& tag_id
        ) const;

        [[nodiscard]] dispatcher::common::ValidationResult reload_configuration(
            dispatcher::domain::ConfigurationSnapshot configuration_snapshot
        );

        void reset_statistics() noexcept;

        [[nodiscard]] TelemetryIngestResult ingest(
            dispatcher::telemetry::TelemetryValue value
        );

        [[nodiscard]] TelemetryIngestBatchResult ingest_batch(
            std::vector<dispatcher::telemetry::TelemetryValue> values
        );

    private:
        [[nodiscard]] bool is_future_source_timestamp(
            const dispatcher::telemetry::TelemetryValue& value
        ) const;

        void record_last_sequence(
            const dispatcher::domain::TagId& tag_id,
            std::uint64_t sequence
        );

        dispatcher::domain::ConfigurationSnapshot configuration_snapshot_;
        CurrentStateStore current_state_;
        TelemetryIngestStatistics statistics_;
        std::chrono::milliseconds max_future_source_timestamp_skew_{ 5000 };
        std::unordered_map<std::string, std::uint64_t> last_sequence_by_tag_id_;
    };
}