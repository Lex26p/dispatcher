#include <dispatcher/history/in_memory_history_store.hpp>

#include <algorithm>
#include <utility>

namespace dispatcher::history
{
    InMemoryHistoryStore::InMemoryHistoryStore(std::size_t max_samples)
        : max_samples_(max_samples)
    {
    }

    void InMemoryHistoryStore::append(HistorySample sample)
    {
        samples_.push_back(std::move(sample));
        apply_retention();
    }

    void InMemoryHistoryStore::append_telemetry(
        dispatcher::telemetry::TelemetryValue telemetry_value
    )
    {
        append(
            HistorySample::from_telemetry_value(
                std::move(telemetry_value)
            )
        );
    }

    void InMemoryHistoryStore::append_batch(
        std::vector<dispatcher::telemetry::TelemetryValue> telemetry_values
    )
    {
        for (auto& telemetry_value : telemetry_values)
        {
            append_telemetry(std::move(telemetry_value));
        }
    }

    std::vector<HistorySample> InMemoryHistoryStore::find_by_tag_id(
        const dispatcher::domain::TagId& tag_id
    ) const
    {
        std::vector<HistorySample> result;

        for (const auto& sample : samples_)
        {
            if (sample.tag_id() == tag_id)
            {
                result.push_back(sample);
            }
        }

        return result;
    }

    std::vector<HistorySample> InMemoryHistoryStore::find_by_tag_id_and_source_time_range(
        const dispatcher::domain::TagId& tag_id,
        TimePoint from,
        TimePoint to
    ) const
    {
        std::vector<HistorySample> result;

        if (to < from)
        {
            return result;
        }

        for (const auto& sample : samples_)
        {
            if (sample.tag_id() == tag_id
                && sample.source_timestamp() >= from
                && sample.source_timestamp() <= to)
            {
                result.push_back(sample);
            }
        }

        return result;
    }

    const std::vector<HistorySample>& InMemoryHistoryStore::samples() const noexcept
    {
        return samples_;
    }

    std::size_t InMemoryHistoryStore::size() const noexcept
    {
        return samples_.size();
    }

    bool InMemoryHistoryStore::empty() const noexcept
    {
        return samples_.empty();
    }

    std::optional<std::size_t> InMemoryHistoryStore::max_samples() const noexcept
    {
        return max_samples_;
    }

    std::uint64_t InMemoryHistoryStore::retained_sample_count() const noexcept
    {
        return retained_sample_count_;
    }

    void InMemoryHistoryStore::set_max_samples(std::optional<std::size_t> value)
    {
        max_samples_ = value;
        apply_retention();
    }

    void InMemoryHistoryStore::clear() noexcept
    {
        samples_.clear();
    }

    void InMemoryHistoryStore::reset_retained_sample_count() noexcept
    {
        retained_sample_count_ = 0;
    }

    void InMemoryHistoryStore::apply_retention()
    {
        if (!max_samples_.has_value())
        {
            return;
        }

        const auto limit = max_samples_.value();

        if (samples_.size() <= limit)
        {
            return;
        }

        const auto remove_count = samples_.size() - limit;

        samples_.erase(
            samples_.begin(),
            samples_.begin() + static_cast<std::ptrdiff_t>(remove_count)
        );

        retained_sample_count_ += remove_count;
    }
}