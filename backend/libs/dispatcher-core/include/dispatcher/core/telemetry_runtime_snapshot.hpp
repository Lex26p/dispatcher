#pragma once

#include <cstdint>
#include <cstddef>

namespace dispatcher::core
{
    struct TelemetryRuntimeSnapshot
    {
        std::uint64_t configuration_version{ 0 };

        std::size_t current_state_size{ 0 };

        std::uint64_t accepted_count{ 0 };
        std::uint64_t stored_count{ 0 };
        std::uint64_t accepted_no_change_count{ 0 };

        std::uint64_t rejected_count{ 0 };
        std::uint64_t unknown_tag_count{ 0 };
        std::uint64_t disabled_tag_count{ 0 };
        std::uint64_t data_type_mismatch_count{ 0 };
        std::uint64_t stale_sequence_count{ 0 };
        std::uint64_t future_source_timestamp_count{ 0 };
        std::uint64_t bad_quality_count{ 0 };

        std::uint64_t total_count{ 0 };
    };
}