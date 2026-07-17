#include <dispatcher/storage/alarm_event_storage_query.hpp>

namespace dispatcher::storage
{
    bool AlarmEventStorageQuery::has_alarm_id() const noexcept
    {
        return alarm_id.has_value();
    }

    bool AlarmEventStorageQuery::has_tag_id() const noexcept
    {
        return tag_id.has_value();
    }

    bool AlarmEventStorageQuery::has_transition_type() const noexcept
    {
        return transition_type.has_value();
    }

    bool AlarmEventStorageQuery::has_from() const noexcept
    {
        return from.has_value();
    }

    bool AlarmEventStorageQuery::has_to() const noexcept
    {
        return to.has_value();
    }

    bool AlarmEventStorageQuery::has_limit() const noexcept
    {
        return limit > 0;
    }

    bool AlarmEventStorageQuery::has_time_range() const noexcept
    {
        return has_from() || has_to();
    }

    bool AlarmEventStorageQuery::has_bounded_time_range() const noexcept
    {
        return has_from() && has_to();
    }

    bool AlarmEventStorageQuery::requests_latest_only() const noexcept
    {
        return latest_only;
    }
}