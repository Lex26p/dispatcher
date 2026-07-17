#include <dispatcher/alarm/alarm_event_store.hpp>

#include <utility>

namespace dispatcher::alarm
{
    void AlarmEventStore::append(AlarmRuntimeEvent event)
    {
        events_.push_back(std::move(event));
    }

    bool AlarmEventStore::append_if_present(
        const std::optional<AlarmRuntimeEvent>& event
    )
    {
        if (!event.has_value())
        {
            return false;
        }

        append(event.value());

        return true;
    }

    bool AlarmEventStore::append_from_evaluation_result(
        const AlarmEvaluationResult& evaluation_result
    )
    {
        return append_if_present(evaluation_result.event());
    }

    std::vector<AlarmRuntimeEvent> AlarmEventStore::find_by_alarm_id(
        const dispatcher::domain::AlarmId& alarm_id
    ) const
    {
        std::vector<AlarmRuntimeEvent> result;

        for (const auto& event : events_)
        {
            if (event.alarm_id() == alarm_id)
            {
                result.push_back(event);
            }
        }

        return result;
    }

    std::vector<AlarmRuntimeEvent> AlarmEventStore::find_by_tag_id(
        const dispatcher::domain::TagId& tag_id
    ) const
    {
        std::vector<AlarmRuntimeEvent> result;

        for (const auto& event : events_)
        {
            if (event.tag_id() == tag_id)
            {
                result.push_back(event);
            }
        }

        return result;
    }

    const std::vector<AlarmRuntimeEvent>& AlarmEventStore::events() const noexcept
    {
        return events_;
    }

    std::size_t AlarmEventStore::size() const noexcept
    {
        return events_.size();
    }

    bool AlarmEventStore::empty() const noexcept
    {
        return events_.empty();
    }

    void AlarmEventStore::clear() noexcept
    {
        events_.clear();
    }
}