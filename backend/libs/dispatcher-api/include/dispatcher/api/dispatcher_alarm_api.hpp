#pragma once

#include <dispatcher/api/alarm_api.hpp>
#include <dispatcher/runtime/dispatcher_runtime.hpp>

namespace dispatcher::api
{
    class DispatcherAlarmApi final : public AlarmApi
    {
    public:
        explicit DispatcherAlarmApi(
            dispatcher::runtime::DispatcherRuntime& runtime
        );

        [[nodiscard]] AlarmOperatorSnapshotApiResult operator_snapshot()
            const override;

        [[nodiscard]] UnacknowledgedAlarmsApiResult unacknowledged_alarms()
            const override;

        [[nodiscard]] AlarmAcknowledgementApiResult acknowledge(
            const AlarmAcknowledgementRequest& request
        ) override;

        [[nodiscard]] dispatcher::runtime::DispatcherRuntime& runtime()
            noexcept;

        [[nodiscard]] const dispatcher::runtime::DispatcherRuntime& runtime()
            const noexcept;

    private:
        dispatcher::runtime::DispatcherRuntime* runtime_{ nullptr };
    };
}