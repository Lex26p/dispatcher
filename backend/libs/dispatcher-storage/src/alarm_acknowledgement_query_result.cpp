#include <dispatcher/storage/alarm_acknowledgement_query_result.hpp>

#include <utility>

namespace dispatcher::storage
{
    AlarmAcknowledgementQueryResult AlarmAcknowledgementQueryResult::success(
        std::vector<dispatcher::alarm::AlarmAcknowledgementRecord> records
    )
    {
        return AlarmAcknowledgementQueryResult(
            StorageResult::success(),
            std::move(records)
        );
    }

    AlarmAcknowledgementQueryResult AlarmAcknowledgementQueryResult::failure(
        StorageStatus status,
        std::string operation,
        std::string key,
        std::string message
    )
    {
        return AlarmAcknowledgementQueryResult(
            StorageResult::failure(
                status,
                std::move(operation),
                std::move(key),
                std::move(message)
            ),
            {}
        );
    }

    bool AlarmAcknowledgementQueryResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool AlarmAcknowledgementQueryResult::failed() const noexcept
    {
        return result_.failed();
    }

    StorageStatus AlarmAcknowledgementQueryResult::status() const noexcept
    {
        return result_.status();
    }

    const StorageResult& AlarmAcknowledgementQueryResult::result() const noexcept
    {
        return result_;
    }

    const StorageError& AlarmAcknowledgementQueryResult::error() const noexcept
    {
        return result_.error();
    }

    const std::vector<dispatcher::alarm::AlarmAcknowledgementRecord>&
        AlarmAcknowledgementQueryResult::records() const noexcept
    {
        return records_;
    }

    std::size_t AlarmAcknowledgementQueryResult::record_count() const noexcept
    {
        return records_.size();
    }

    bool AlarmAcknowledgementQueryResult::empty() const noexcept
    {
        return records_.empty();
    }

    AlarmAcknowledgementQueryResult::AlarmAcknowledgementQueryResult(
        StorageResult result,
        std::vector<dispatcher::alarm::AlarmAcknowledgementRecord> records
    )
        : result_(std::move(result))
        , records_(std::move(records))
    {
    }
}