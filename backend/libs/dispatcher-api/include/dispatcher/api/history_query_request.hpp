#pragma once

#include <dispatcher/api/api_page_request.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/storage/history_storage_query.hpp>

#include <optional>

namespace dispatcher::api
{
    struct HistoryQueryRequest
    {
        using TimePoint = dispatcher::storage::HistoryStorageQuery::TimePoint;

        std::optional<dispatcher::domain::TagId> tag_id;
        std::optional<TimePoint> from;
        std::optional<TimePoint> to;

        ApiPageRequest page;
        bool latest_only{ false };

        [[nodiscard]] bool has_tag_id() const noexcept;

        [[nodiscard]] bool has_from() const noexcept;

        [[nodiscard]] bool has_to() const noexcept;

        [[nodiscard]] bool has_time_range() const noexcept;

        [[nodiscard]] bool has_bounded_time_range() const noexcept;

        [[nodiscard]] bool requests_latest_only() const noexcept;

        [[nodiscard]] dispatcher::storage::HistoryStorageQuery
            to_storage_query() const;
    };
}