#pragma once

#include <dispatcher/alarm/alarm_acknowledgement_record.hpp>
#include <dispatcher/storage/alarm_acknowledgement_query_result.hpp>
#include <dispatcher/storage/alarm_acknowledgement_storage_query.hpp>
#include <dispatcher/storage/storage_result.hpp>

#include <vector>

namespace dispatcher::storage
{
    class AlarmAcknowledgementStorage
    {
    public:
        virtual ~AlarmAcknowledgementStorage() = default;

        [[nodiscard]] virtual StorageResult append(
            const dispatcher::alarm::AlarmAcknowledgementRecord& record
        ) = 0;

        [[nodiscard]] virtual StorageResult append_batch(
            const std::vector<dispatcher::alarm::AlarmAcknowledgementRecord>&
            records
        ) = 0;

        [[nodiscard]] virtual AlarmAcknowledgementQueryResult query(
            const AlarmAcknowledgementStorageQuery& query
        ) const = 0;

        [[nodiscard]] virtual StorageResult remove_by_alarm(
            const dispatcher::domain::AlarmId& alarm_id
        ) = 0;

        [[nodiscard]] virtual StorageResult remove_by_operator(
            const std::string& operator_id
        ) = 0;

        [[nodiscard]] virtual StorageResult clear() = 0;
    };
}