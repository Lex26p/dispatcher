#pragma once

#include <dispatcher/alarm/alarm_runtime_event.hpp>
#include <dispatcher/storage/storage_result.hpp>
#include <dispatcher/storage/storage_status.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace dispatcher::storage
{
    class AlarmEventQueryResult
    {
    public:
        [[nodiscard]] static AlarmEventQueryResult success(
            std::vector<dispatcher::alarm::AlarmRuntimeEvent> events
        );

        [[nodiscard]] static AlarmEventQueryResult failure(
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

        [[nodiscard]] const std::vector<dispatcher::alarm::AlarmRuntimeEvent>&
            events() const noexcept;

        [[nodiscard]] std::size_t event_count() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

    private:
        AlarmEventQueryResult(
            StorageResult result,
            std::vector<dispatcher::alarm::AlarmRuntimeEvent> events
        );

        StorageResult result_;
        std::vector<dispatcher::alarm::AlarmRuntimeEvent> events_;
    };
}