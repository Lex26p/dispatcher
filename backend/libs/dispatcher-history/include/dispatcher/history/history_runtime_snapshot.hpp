#pragma once

#include <cstddef>
#include <cstdint>

namespace dispatcher::history
{
    struct HistoryRuntimeSnapshot
    {
        std::size_t store_size{ 0 };

        bool max_samples_enabled{ false };
        std::size_t max_samples{ 0 };

        std::uint64_t retained_sample_count{ 0 };

        std::uint64_t written_count{ 0 };
        std::uint64_t skipped_not_stored_count{ 0 };
        std::uint64_t skipped_by_policy_count{ 0 };
        std::uint64_t skipped_count{ 0 };
        std::uint64_t total_write_count{ 0 };
    };
}