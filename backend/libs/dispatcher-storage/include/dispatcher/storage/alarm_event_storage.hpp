#pragma once

#include <dispatcher/alarm/alarm_runtime_event.hpp>
#include <dispatcher/storage/alarm_event_query_result.hpp>
#include <dispatcher/storage/alarm_event_storage_query.hpp>
#include <dispatcher/storage/storage_result.hpp>

#include <vector>

namespace dispatcher::storage
{
    class AlarmEventStorage
    {
    public:
        virtual ~AlarmEventStorage() = default;

        [[nodiscard]] virtual StorageResult append(
            const dispatcher::alarm::AlarmRuntimeEvent& event
        ) = 0;

        [[nodiscard]] virtual StorageResult append_batch(
            const std::vector<dispatcher::alarm::AlarmRuntimeEvent>& events
        ) = 0;

        [[nodiscard]] virtual AlarmEventQueryResult query(
            const AlarmEventStorageQuery& query
        ) const = 0;

        [[nodiscard]] virtual StorageResult remove_by_alarm(
            const dispatcher::domain::AlarmId& alarm_id
        ) = 0;

        [[nodiscard]] virtual StorageResult remove_by_tag(
            const dispatcher::domain::TagId& tag_id
        ) = 0;

        [[nodiscard]] virtual StorageResult clear() = 0;
    };
}