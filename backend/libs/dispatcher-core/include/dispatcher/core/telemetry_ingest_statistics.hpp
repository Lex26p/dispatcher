#pragma once

#include <cstdint>

namespace dispatcher::core
{
    class TelemetryIngestStatistics
    {
    public:
        void record_accepted_stored() noexcept
        {
            ++accepted_count_;
            ++stored_count_;
        }

        void record_accepted_no_change() noexcept
        {
            ++accepted_count_;
            ++accepted_no_change_count_;
        }

        void record_unknown_tag() noexcept
        {
            ++unknown_tag_count_;
            ++rejected_count_;
        }

        void record_disabled_tag() noexcept
        {
            ++disabled_tag_count_;
            ++rejected_count_;
        }

        void record_data_type_mismatch() noexcept
        {
            ++data_type_mismatch_count_;
            ++rejected_count_;
        }

        void record_stale_sequence() noexcept
        {
            ++stale_sequence_count_;
            ++rejected_count_;
        }

        void record_future_source_timestamp() noexcept
        {
            ++future_source_timestamp_count_;
            ++rejected_count_;
        }

        void record_bad_quality() noexcept
        {
            ++bad_quality_count_;
            ++rejected_count_;
        }

        [[nodiscard]] std::uint64_t accepted_count() const noexcept
        {
            return accepted_count_;
        }

        [[nodiscard]] std::uint64_t stored_count() const noexcept
        {
            return stored_count_;
        }

        [[nodiscard]] std::uint64_t accepted_no_change_count() const noexcept
        {
            return accepted_no_change_count_;
        }

        [[nodiscard]] std::uint64_t rejected_count() const noexcept
        {
            return rejected_count_;
        }

        [[nodiscard]] std::uint64_t unknown_tag_count() const noexcept
        {
            return unknown_tag_count_;
        }

        [[nodiscard]] std::uint64_t disabled_tag_count() const noexcept
        {
            return disabled_tag_count_;
        }

        [[nodiscard]] std::uint64_t data_type_mismatch_count() const noexcept
        {
            return data_type_mismatch_count_;
        }

        [[nodiscard]] std::uint64_t stale_sequence_count() const noexcept
        {
            return stale_sequence_count_;
        }

        [[nodiscard]] std::uint64_t future_source_timestamp_count() const noexcept
        {
            return future_source_timestamp_count_;
        }

        [[nodiscard]] std::uint64_t bad_quality_count() const noexcept
        {
            return bad_quality_count_;
        }

        [[nodiscard]] std::uint64_t total_count() const noexcept
        {
            return accepted_count_ + rejected_count_;
        }

        void reset() noexcept
        {
            accepted_count_ = 0;
            stored_count_ = 0;
            accepted_no_change_count_ = 0;
            rejected_count_ = 0;
            unknown_tag_count_ = 0;
            disabled_tag_count_ = 0;
            data_type_mismatch_count_ = 0;
            stale_sequence_count_ = 0;
            future_source_timestamp_count_ = 0;
            bad_quality_count_ = 0;
        }

    private:
        std::uint64_t accepted_count_{ 0 };
        std::uint64_t stored_count_{ 0 };
        std::uint64_t accepted_no_change_count_{ 0 };
        std::uint64_t rejected_count_{ 0 };
        std::uint64_t unknown_tag_count_{ 0 };
        std::uint64_t disabled_tag_count_{ 0 };
        std::uint64_t data_type_mismatch_count_{ 0 };
        std::uint64_t stale_sequence_count_{ 0 };
        std::uint64_t future_source_timestamp_count_{ 0 };
        std::uint64_t bad_quality_count_{ 0 };
    };
}