#pragma once

#include <cstdint>
#include <cstddef>

namespace dispatcher::alarm
{
    struct AlarmSuppressionRuntimeSnapshot
    {
        std::size_t store_size{ 0 };
        std::size_t active_count{ 0 };
        std::size_t expired_count{ 0 };

        std::size_t shelved_count{ 0 };
        std::size_t suppressed_count{ 0 };
        std::size_t inhibited_count{ 0 };

        std::size_t operator_controlled_count{ 0 };
        std::size_t system_controlled_count{ 0 };

        std::uint64_t applied_count{ 0 };
        std::uint64_t cleared_count{ 0 };
        std::uint64_t rejected_count{ 0 };
        std::uint64_t expired_removed_count{ 0 };

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool has_records() const noexcept;

        [[nodiscard]] bool has_active_records() const noexcept;

        [[nodiscard]] bool has_expired_records() const noexcept;

        [[nodiscard]] bool has_operator_controlled_records() const noexcept;

        [[nodiscard]] bool has_system_controlled_records() const noexcept;

        [[nodiscard]] std::uint64_t total_command_count() const noexcept;
    };
}