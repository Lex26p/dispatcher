#pragma once

#include <dispatcher/api/alarm_api.hpp>
#include <dispatcher/api/configuration_api.hpp>
#include <dispatcher/api/dispatcher_alarm_api.hpp>
#include <dispatcher/api/dispatcher_configuration_api.hpp>
#include <dispatcher/api/dispatcher_history_api.hpp>
#include <dispatcher/api/dispatcher_runtime_api.hpp>
#include <dispatcher/api/dispatcher_telemetry_api.hpp>
#include <dispatcher/api/history_api.hpp>
#include <dispatcher/api/runtime_api.hpp>
#include <dispatcher/api/telemetry_api.hpp>
#include <dispatcher/runtime/dispatcher_runtime.hpp>

namespace dispatcher::api
{
    class DispatcherApi final
    {
    public:
        explicit DispatcherApi(
            dispatcher::runtime::DispatcherRuntime& runtime
        );

        [[nodiscard]] dispatcher::runtime::DispatcherRuntime& runtime()
            noexcept;

        [[nodiscard]] const dispatcher::runtime::DispatcherRuntime& runtime()
            const noexcept;

        [[nodiscard]] RuntimeApi& runtime_api() noexcept;

        [[nodiscard]] const RuntimeApi& runtime_api() const noexcept;

        [[nodiscard]] TelemetryApi& telemetry_api() noexcept;

        [[nodiscard]] const TelemetryApi& telemetry_api() const noexcept;

        [[nodiscard]] AlarmApi& alarm_api() noexcept;

        [[nodiscard]] const AlarmApi& alarm_api() const noexcept;

        [[nodiscard]] HistoryApi& history_api() noexcept;

        [[nodiscard]] const HistoryApi& history_api() const noexcept;

        [[nodiscard]] ConfigurationApi& configuration_api() noexcept;

        [[nodiscard]] const ConfigurationApi& configuration_api()
            const noexcept;

        [[nodiscard]] DispatcherRuntimeApi& dispatcher_runtime_api()
            noexcept;

        [[nodiscard]] const DispatcherRuntimeApi& dispatcher_runtime_api()
            const noexcept;

        [[nodiscard]] DispatcherTelemetryApi& dispatcher_telemetry_api()
            noexcept;

        [[nodiscard]] const DispatcherTelemetryApi& dispatcher_telemetry_api()
            const noexcept;

        [[nodiscard]] DispatcherAlarmApi& dispatcher_alarm_api() noexcept;

        [[nodiscard]] const DispatcherAlarmApi& dispatcher_alarm_api()
            const noexcept;

        [[nodiscard]] DispatcherHistoryApi& dispatcher_history_api()
            noexcept;

        [[nodiscard]] const DispatcherHistoryApi& dispatcher_history_api()
            const noexcept;

        [[nodiscard]] DispatcherConfigurationApi&
            dispatcher_configuration_api() noexcept;

        [[nodiscard]] const DispatcherConfigurationApi&
            dispatcher_configuration_api() const noexcept;

    private:
        dispatcher::runtime::DispatcherRuntime* runtime_{ nullptr };

        DispatcherRuntimeApi runtime_api_;
        DispatcherTelemetryApi telemetry_api_;
        DispatcherAlarmApi alarm_api_;
        DispatcherHistoryApi history_api_;
        DispatcherConfigurationApi configuration_api_;
    };
}