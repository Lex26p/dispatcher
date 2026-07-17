#pragma once

#include <dispatcher/core/telemetry_ingest_status.hpp>

#include <cstdint>

namespace dispatcher::core
{
    class TelemetryIngestBatchResult
    {
    public:
        void record(TelemetryIngestStatus status) noexcept
        {
            ++total_count_;

            switch (status)
            {
            case TelemetryIngestStatus::Accepted:
                ++accepted_count_;
                ++stored_count_;
                break;

            case TelemetryIngestStatus::AcceptedNoChange:
                ++accepted_count_;
                ++accepted_no_change_count_;
                break;

            case TelemetryIngestStatus::UnknownTag:
                ++rejected_count_;
                ++unknown_tag_count_;
                break;

            case TelemetryIngestStatus::DisabledTag:
                ++rejected_count_;
                ++disabled_tag_count_;
                break;

            case TelemetryIngestStatus::DataTypeMismatch:
                ++rejected_count_;
                ++data_type_mismatch_count_;
                break;

            case TelemetryIngestStatus::StaleSequence:
                ++rejected_count_;
                ++stale_sequence_count_;
                break;

            case TelemetryIngestStatus::FutureSourceTimestamp:
                ++rejected_count_;
                ++future_source_timestamp_count_;
                break;

            case TelemetryIngestStatus::BadQuality:
                ++rejected_count_;
                ++bad_quality_count_;
                break;
            }
        }

        [[nodiscard]] std::uint64_t total_count() const noexcept
        {
            return total_count_;
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

        [[nodiscard]] bool all_accepted() const noexcept
        {
            return total_count_ > 0 && rejected_count_ == 0;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return total_count_ == 0;
        }

        [[nodiscard]] bool has_rejections() const noexcept
        {
            return rejected_count_ > 0;
        }

    private:
        std::uint64_t total_count_{ 0 };
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