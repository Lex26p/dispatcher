#include <dispatcher/api/history_query_request.hpp>

namespace dispatcher::api
{
    bool HistoryQueryRequest::has_tag_id() const noexcept
    {
        return tag_id.has_value();
    }

    bool HistoryQueryRequest::has_from() const noexcept
    {
        return from.has_value();
    }

    bool HistoryQueryRequest::has_to() const noexcept
    {
        return to.has_value();
    }

    bool HistoryQueryRequest::has_time_range() const noexcept
    {
        return has_from() || has_to();
    }

    bool HistoryQueryRequest::has_bounded_time_range() const noexcept
    {
        return has_from() && has_to();
    }

    bool HistoryQueryRequest::requests_latest_only() const noexcept
    {
        return latest_only;
    }

    dispatcher::storage::HistoryStorageQuery
        HistoryQueryRequest::to_storage_query() const
    {
        return dispatcher::storage::HistoryStorageQuery{
            .tag_id = tag_id,
            .from = from,
            .to = to,
            .limit = page.limit,
            .latest_only = latest_only
        };
    }
}