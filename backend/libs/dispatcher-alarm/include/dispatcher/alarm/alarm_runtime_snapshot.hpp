#pragma once

#include <dispatcher/alarm/alarm_suppression_runtime_snapshot.hpp>

#include <cstddef>
#include <cstdint>

namespace dispatcher::alarm
{
    struct AlarmRuntimeSnapshot
    {
        std::size_t state_store_size{ 0 };
        std::size_t event_store_size{ 0 };
        std::size_t acknowledgement_store_size{ 0 };

        std::uint64_t total_count{ 0 };
        std::uint64_t evaluated_count{ 0 };
        std::uint64_t skipped_count{ 0 };

        std::uint64_t disabled_alarm_count{ 0 };
        std::uint64_t tag_mismatch_count{ 0 };
        std::uint64_t unsupported_value_type_count{ 0 };

        std::uint64_t activated_count{ 0 };
        std::uint64_t acknowledged_count{ 0 };
        std::uint64_t cleared_count{ 0 };
        std::uint64_t no_transition_count{ 0 };
        std::uint64_t stored_event_count{ 0 };

        std::uint64_t configuration_version{ 0 };
        std::size_t configured_alarm_count{ 0 };
        std::size_t indexed_tag_count{ 0 };
        std::size_t indexed_condition_count{ 0 };

        AlarmSuppressionRuntimeSnapshot suppression;
    };
}