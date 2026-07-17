#pragma once

#include <dispatcher/history/history_write_result.hpp>

#include <cstdint>

namespace dispatcher::history
{
    class HistoryWriteBatchResult
    {
    public:
        void record(HistoryWriteStatus status) noexcept
        {
            ++total_count_;

            switch (status)
            {
            case HistoryWriteStatus::Written:
                ++written_count_;
                break;

            case HistoryWriteStatus::SkippedNotStored:
                ++skipped_not_stored_count_;
                break;

            case HistoryWriteStatus::SkippedByPolicy:
                ++skipped_by_policy_count_;
                break;
            }
        }

        [[nodiscard]] std::uint64_t total_count() const noexcept
        {
            return total_count_;
        }

        [[nodiscard]] std::uint64_t written_count() const noexcept
        {
            return written_count_;
        }

        [[nodiscard]] std::uint64_t skipped_not_stored_count() const noexcept
        {
            return skipped_not_stored_count_;
        }

        [[nodiscard]] std::uint64_t skipped_by_policy_count() const noexcept
        {
            return skipped_by_policy_count_;
        }

        [[nodiscard]] std::uint64_t skipped_count() const noexcept
        {
            return skipped_not_stored_count_ + skipped_by_policy_count_;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return total_count_ == 0;
        }

        [[nodiscard]] bool all_written() const noexcept
        {
            return total_count_ > 0 && skipped_count() == 0;
        }

        [[nodiscard]] bool has_skipped() const noexcept
        {
            return skipped_count() > 0;
        }

    private:
        std::uint64_t total_count_{ 0 };
        std::uint64_t written_count_{ 0 };
        std::uint64_t skipped_not_stored_count_{ 0 };
        std::uint64_t skipped_by_policy_count_{ 0 };
    };
}