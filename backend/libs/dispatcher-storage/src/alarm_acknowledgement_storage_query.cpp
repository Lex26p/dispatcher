#include <dispatcher/storage/alarm_acknowledgement_storage_query.hpp>

namespace dispatcher::storage
{
    bool AlarmAcknowledgementStorageQuery::has_alarm_id() const noexcept
    {
        return alarm_id.has_value();
    }

    bool AlarmAcknowledgementStorageQuery::has_operator_id() const noexcept
    {
        return operator_id.has_value() && !operator_id->empty();
    }

    bool AlarmAcknowledgementStorageQuery::has_from() const noexcept
    {
        return from.has_value();
    }

    bool AlarmAcknowledgementStorageQuery::has_to() const noexcept
    {
        return to.has_value();
    }

    bool AlarmAcknowledgementStorageQuery::has_limit() const noexcept
    {
        return limit > 0;
    }

    bool AlarmAcknowledgementStorageQuery::has_time_range() const noexcept
    {
        return has_from() || has_to();
    }

    bool AlarmAcknowledgementStorageQuery::has_bounded_time_range() const noexcept
    {
        return has_from() && has_to();
    }

    bool AlarmAcknowledgementStorageQuery::requests_latest_only() const noexcept
    {
        return latest_only;
    }
}