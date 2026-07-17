#pragma once

#include <dispatcher/api/api_result.hpp>
#include <dispatcher/api/api_status.hpp>
#include <dispatcher/runtime/dispatcher_runtime.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <utility>

namespace dispatcher::api
{
    using UnacknowledgedAlarmList = decltype(
        std::declval<const dispatcher::runtime::DispatcherRuntime&>()
        .unacknowledged_alarms()
        );

    class UnacknowledgedAlarmsApiResult
    {
    public:
        [[nodiscard]] static UnacknowledgedAlarmsApiResult success(
            UnacknowledgedAlarmList alarms
        );

        [[nodiscard]] static UnacknowledgedAlarmsApiResult failure(
            ApiStatus status,
            std::string operation = {},
            std::string resource = {},
            std::string field = {},
            std::string message = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] ApiStatus status() const noexcept;

        [[nodiscard]] const ApiResult& result() const noexcept;

        [[nodiscard]] const ApiError& error() const noexcept;

        [[nodiscard]] bool has_alarms() const noexcept;

        [[nodiscard]] const UnacknowledgedAlarmList& alarms() const;

        [[nodiscard]] std::size_t alarm_count() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

    private:
        UnacknowledgedAlarmsApiResult(
            ApiResult result,
            std::optional<UnacknowledgedAlarmList> alarms
        );

        ApiResult result_;
        std::optional<UnacknowledgedAlarmList> alarms_;
    };
}