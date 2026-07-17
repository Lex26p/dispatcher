#pragma once

#include <string_view>

namespace dispatcher::history
{
    enum class HistoryWriteStatus
    {
        Written,
        SkippedNotStored,
        SkippedByPolicy
    };

    constexpr std::string_view to_string(HistoryWriteStatus status)
    {
        switch (status)
        {
        case HistoryWriteStatus::Written:
            return "written";
        case HistoryWriteStatus::SkippedNotStored:
            return "skipped_not_stored";
        case HistoryWriteStatus::SkippedByPolicy:
            return "skipped_by_policy";
        }

        return "unknown";
    }

    class HistoryWriteResult
    {
    public:
        explicit HistoryWriteResult(HistoryWriteStatus status)
            : status_(status)
        {
        }

        [[nodiscard]] HistoryWriteStatus status() const noexcept
        {
            return status_;
        }

        [[nodiscard]] bool written() const noexcept
        {
            return status_ == HistoryWriteStatus::Written;
        }

        [[nodiscard]] bool skipped() const noexcept
        {
            return !written();
        }

        [[nodiscard]] bool skipped_by_policy() const noexcept
        {
            return status_ == HistoryWriteStatus::SkippedByPolicy;
        }

    private:
        HistoryWriteStatus status_;
    };
}