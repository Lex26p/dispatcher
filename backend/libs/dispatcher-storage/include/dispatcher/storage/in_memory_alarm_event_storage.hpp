#pragma once

#include <dispatcher/storage/alarm_event_storage.hpp>
#include <dispatcher/storage/storage_result.hpp>
#include <dispatcher/storage/storage_status.hpp>

#include <cstddef>
#include <vector>

namespace dispatcher::storage
{
    class InMemoryAlarmEventStorage final : public AlarmEventStorage
    {
    public:
        [[nodiscard]] StorageResult append(
            const dispatcher::alarm::AlarmRuntimeEvent& event
        ) override
        {
            events_.push_back(event);

            return StorageResult::success();
        }

        [[nodiscard]] StorageResult append_batch(
            const std::vector<dispatcher::alarm::AlarmRuntimeEvent>& events
        ) override
        {
            events_.insert(
                events_.end(),
                events.begin(),
                events.end()
            );

            return StorageResult::success();
        }

        [[nodiscard]] AlarmEventQueryResult query(
            const AlarmEventStorageQuery& query
        ) const override
        {
            auto selected_events = events_;

            if (query.requests_latest_only())
            {
                if (selected_events.empty())
                {
                    return AlarmEventQueryResult::success({});
                }

                return AlarmEventQueryResult::success(
                    std::vector<dispatcher::alarm::AlarmRuntimeEvent>{
                    selected_events.back()
                }
                );
            }

            if (
                query.has_limit()
                && selected_events.size() > query.limit
                )
            {
                selected_events.erase(
                    selected_events.begin(),
                    selected_events.end()
                    - static_cast<std::vector<
                    dispatcher::alarm::AlarmRuntimeEvent
                    >::difference_type>(query.limit)
                );
            }

            return AlarmEventQueryResult::success(
                std::move(selected_events)
            );
        }

        [[nodiscard]] StorageResult remove_by_alarm(
            const dispatcher::domain::AlarmId& alarm_id
        ) override
        {
            return StorageResult::failure(
                StorageStatus::UnsupportedOperation,
                "alarm_event.remove_by_alarm",
                alarm_id.value(),
                "remove_by_alarm is not implemented by baseline in-memory alarm event storage"
            );
        }

        [[nodiscard]] StorageResult remove_by_tag(
            const dispatcher::domain::TagId& tag_id
        ) override
        {
            return StorageResult::failure(
                StorageStatus::UnsupportedOperation,
                "alarm_event.remove_by_tag",
                tag_id.value(),
                "remove_by_tag is not implemented by baseline in-memory alarm event storage"
            );
        }

        [[nodiscard]] StorageResult clear() override
        {
            events_.clear();

            return StorageResult::success();
        }

        [[nodiscard]] const std::vector<dispatcher::alarm::AlarmRuntimeEvent>&
            events() const noexcept
        {
            return events_;
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return events_.size();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return events_.empty();
        }

    private:
        std::vector<dispatcher::alarm::AlarmRuntimeEvent> events_;
    };
}