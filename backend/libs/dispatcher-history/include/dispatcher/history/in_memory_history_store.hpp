#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/history/history_sample.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace dispatcher::history
{
    class InMemoryHistoryStore
    {
    public:
        using TimePoint = HistorySample::TimePoint;

        InMemoryHistoryStore() = default;

        explicit InMemoryHistoryStore(std::size_t max_samples);

        void append(HistorySample sample);

        void append_telemetry(
            dispatcher::telemetry::TelemetryValue telemetry_value
        );

        void append_batch(
            std::vector<dispatcher::telemetry::TelemetryValue> telemetry_values
        );

        [[nodiscard]] std::vector<HistorySample> find_by_tag_id(
            const dispatcher::domain::TagId& tag_id
        ) const;

        [[nodiscard]] std::vector<HistorySample> find_by_tag_id_and_source_time_range(
            const dispatcher::domain::TagId& tag_id,
            TimePoint from,
            TimePoint to
        ) const;

        [[nodiscard]] const std::vector<HistorySample>& samples() const noexcept;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] std::optional<std::size_t> max_samples() const noexcept;

        [[nodiscard]] std::uint64_t retained_sample_count() const noexcept;

        void set_max_samples(std::optional<std::size_t> value);

        void clear() noexcept;

        void reset_retained_sample_count() noexcept;

    private:
        void apply_retention();

        std::vector<HistorySample> samples_;
        std::optional<std::size_t> max_samples_;
        std::uint64_t retained_sample_count_{ 0 };
    };
}