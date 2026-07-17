#pragma once

#include <dispatcher/alarm/alarm_acknowledgement_record.hpp>
#include <dispatcher/storage/storage_result.hpp>
#include <dispatcher/storage/storage_status.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace dispatcher::storage
{
    class AlarmAcknowledgementQueryResult
    {
    public:
        [[nodiscard]] static AlarmAcknowledgementQueryResult success(
            std::vector<dispatcher::alarm::AlarmAcknowledgementRecord> records
        );

        [[nodiscard]] static AlarmAcknowledgementQueryResult failure(
            StorageStatus status,
            std::string operation = {},
            std::string key = {},
            std::string message = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] StorageStatus status() const noexcept;

        [[nodiscard]] const StorageResult& result() const noexcept;

        [[nodiscard]] const StorageError& error() const noexcept;

        [[nodiscard]] const std::vector<dispatcher::alarm::AlarmAcknowledgementRecord>&
            records() const noexcept;

        [[nodiscard]] std::size_t record_count() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

    private:
        AlarmAcknowledgementQueryResult(
            StorageResult result,
            std::vector<dispatcher::alarm::AlarmAcknowledgementRecord> records
        );

        StorageResult result_;
        std::vector<dispatcher::alarm::AlarmAcknowledgementRecord> records_;
    };
}