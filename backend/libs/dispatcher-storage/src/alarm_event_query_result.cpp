#include <dispatcher/storage/alarm_event_query_result.hpp>

#include <utility>

namespace dispatcher::storage
{
    AlarmEventQueryResult AlarmEventQueryResult::success(
        std::vector<dispatcher::alarm::AlarmRuntimeEvent> events
    )
    {
        return AlarmEventQueryResult(
            StorageResult::success(),
            std::move(events)
        );
    }

    AlarmEventQueryResult AlarmEventQueryResult::failure(
        StorageStatus status,
        std::string operation,
        std::string key,
        std::string message
    )
    {
        return AlarmEventQueryResult(
            StorageResult::failure(
                status,
                std::move(operation),
                std::move(key),
                std::move(message)
            ),
            {}
        );
    }

    bool AlarmEventQueryResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool AlarmEventQueryResult::failed() const noexcept
    {
        return result_.failed();
    }

    StorageStatus AlarmEventQueryResult::status() const noexcept
    {
        return result_.status();
    }

    const StorageResult& AlarmEventQueryResult::result() const noexcept
    {
        return result_;
    }

    const StorageError& AlarmEventQueryResult::error() const noexcept
    {
        return result_.error();
    }

    const std::vector<dispatcher::alarm::AlarmRuntimeEvent>&
        AlarmEventQueryResult::events() const noexcept
    {
        return events_;
    }

    std::size_t AlarmEventQueryResult::event_count() const noexcept
    {
        return events_.size();
    }

    bool AlarmEventQueryResult::empty() const noexcept
    {
        return events_.empty();
    }

    AlarmEventQueryResult::AlarmEventQueryResult(
        StorageResult result,
        std::vector<dispatcher::alarm::AlarmRuntimeEvent> events
    )
        : result_(std::move(result))
        , events_(std::move(events))
    {
    }
}