#include <dispatcher/alarm/alarm_acknowledgement_store.hpp>

#include <utility>

namespace dispatcher::alarm
{
    void AlarmAcknowledgementStore::append(
        AlarmAcknowledgementRecord record
    )
    {
        records_.push_back(std::move(record));
    }

    std::vector<AlarmAcknowledgementRecord>
        AlarmAcknowledgementStore::find_by_alarm_id(
            const dispatcher::domain::AlarmId& alarm_id
        ) const
    {
        std::vector<AlarmAcknowledgementRecord> result;

        for (const auto& record : records_)
        {
            if (record.alarm_id() == alarm_id)
            {
                result.push_back(record);
            }
        }

        return result;
    }

    std::vector<AlarmAcknowledgementRecord>
        AlarmAcknowledgementStore::find_by_operator_id(
            const std::string& operator_id
        ) const
    {
        std::vector<AlarmAcknowledgementRecord> result;

        for (const auto& record : records_)
        {
            if (record.operator_id() == operator_id)
            {
                result.push_back(record);
            }
        }

        return result;
    }

    const std::vector<AlarmAcknowledgementRecord>&
        AlarmAcknowledgementStore::records() const noexcept
    {
        return records_;
    }

    std::size_t AlarmAcknowledgementStore::size() const noexcept
    {
        return records_.size();
    }

    bool AlarmAcknowledgementStore::empty() const noexcept
    {
        return records_.empty();
    }

    void AlarmAcknowledgementStore::clear() noexcept
    {
        records_.clear();
    }
}