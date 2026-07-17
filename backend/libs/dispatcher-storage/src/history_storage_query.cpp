#include <dispatcher/storage/history_storage_query.hpp>

namespace dispatcher::storage
{
    bool HistoryStorageQuery::has_tag() const noexcept
    {
        return tag_id.has_value();
    }

    bool HistoryStorageQuery::has_from() const noexcept
    {
        return from.has_value();
    }

    bool HistoryStorageQuery::has_to() const noexcept
    {
        return to.has_value();
    }

    bool HistoryStorageQuery::has_limit() const noexcept
    {
        return limit > 0;
    }

    bool HistoryStorageQuery::has_time_range() const noexcept
    {
        return has_from() || has_to();
    }

    bool HistoryStorageQuery::has_bounded_time_range() const noexcept
    {
        return has_from() && has_to();
    }

    bool HistoryStorageQuery::requests_latest_only() const noexcept
    {
        return latest_only;
    }
}