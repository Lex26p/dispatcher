#pragma once

#include <cstdint>

namespace dispatcher::history
{
    class HistoryStatistics
    {
    public:
        void record_written() noexcept
        {
            ++written_count_;
        }

        void record_skipped_not_stored() noexcept
        {
            ++skipped_not_stored_count_;
        }

        void record_skipped_by_policy() noexcept
        {
            ++skipped_by_policy_count_;
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

        [[nodiscard]] std::uint64_t total_count() const noexcept
        {
            return written_count_ + skipped_count();
        }

        void reset() noexcept
        {
            written_count_ = 0;
            skipped_not_stored_count_ = 0;
            skipped_by_policy_count_ = 0;
        }

    private:
        std::uint64_t written_count_{ 0 };
        std::uint64_t skipped_not_stored_count_{ 0 };
        std::uint64_t skipped_by_policy_count_{ 0 };
    };
}