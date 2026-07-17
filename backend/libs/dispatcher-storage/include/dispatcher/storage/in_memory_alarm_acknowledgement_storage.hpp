#pragma once

#include <dispatcher/storage/alarm_acknowledgement_storage.hpp>
#include <dispatcher/storage/storage_result.hpp>
#include <dispatcher/storage/storage_status.hpp>

#include <cstddef>
#include <vector>

namespace dispatcher::storage
{
    class InMemoryAlarmAcknowledgementStorage final
        : public AlarmAcknowledgementStorage
    {
    public:
        [[nodiscard]] StorageResult append(
            const dispatcher::alarm::AlarmAcknowledgementRecord& record
        ) override
        {
            records_.push_back(record);

            return StorageResult::success();
        }

        [[nodiscard]] StorageResult append_batch(
            const std::vector<dispatcher::alarm::AlarmAcknowledgementRecord>&
            records
        ) override
        {
            records_.insert(
                records_.end(),
                records.begin(),
                records.end()
            );

            return StorageResult::success();
        }

        [[nodiscard]] AlarmAcknowledgementQueryResult query(
            const AlarmAcknowledgementStorageQuery& query
        ) const override
        {
            auto selected_records = records_;

            if (query.requests_latest_only())
            {
                if (selected_records.empty())
                {
                    return AlarmAcknowledgementQueryResult::success({});
                }

                return AlarmAcknowledgementQueryResult::success(
                    std::vector<
                    dispatcher::alarm::AlarmAcknowledgementRecord
                    >{
                    selected_records.back()
                }
                );
            }

            if (
                query.has_limit()
                && selected_records.size() > query.limit
                )
            {
                selected_records.erase(
                    selected_records.begin(),
                    selected_records.end()
                    - static_cast<std::vector<
                    dispatcher::alarm::AlarmAcknowledgementRecord
                    >::difference_type>(query.limit)
                );
            }

            return AlarmAcknowledgementQueryResult::success(
                std::move(selected_records)
            );
        }

        [[nodiscard]] StorageResult remove_by_alarm(
            const dispatcher::domain::AlarmId& alarm_id
        ) override
        {
            return StorageResult::failure(
                StorageStatus::UnsupportedOperation,
                "alarm_acknowledgement.remove_by_alarm",
                alarm_id.value(),
                "remove_by_alarm is not implemented by baseline in-memory acknowledgement storage"
            );
        }

        [[nodiscard]] StorageResult remove_by_operator(
            const std::string& operator_id
        ) override
        {
            return StorageResult::failure(
                StorageStatus::UnsupportedOperation,
                "alarm_acknowledgement.remove_by_operator",
                operator_id,
                "remove_by_operator is not implemented by baseline in-memory acknowledgement storage"
            );
        }

        [[nodiscard]] StorageResult clear() override
        {
            records_.clear();

            return StorageResult::success();
        }

        [[nodiscard]] const std::vector<
            dispatcher::alarm::AlarmAcknowledgementRecord
        >& records() const noexcept
        {
            return records_;
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return records_.size();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return records_.empty();
        }

    private:
        std::vector<dispatcher::alarm::AlarmAcknowledgementRecord> records_;
    };
}