#pragma once

#include <dispatcher/domain/id_types.hpp>

#include <chrono>
#include <cstddef>
#include <optional>

namespace dispatcher::storage
{
    struct HistoryStorageQuery
    {
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

        std::optional<dispatcher::domain::TagId> tag_id;
        std::optional<TimePoint> from;
        std::optional<TimePoint> to;

        std::size_t limit{ 0 };
        bool latest_only{ false };

        [[nodiscard]] bool has_tag() const noexcept;

        [[nodiscard]] bool has_from() const noexcept;

        [[nodiscard]] bool has_to() const noexcept;

        [[nodiscard]] bool has_limit() const noexcept;

        [[nodiscard]] bool has_time_range() const noexcept;

        [[nodiscard]] bool has_bounded_time_range() const noexcept;

        [[nodiscard]] bool requests_latest_only() const noexcept;
    };
}