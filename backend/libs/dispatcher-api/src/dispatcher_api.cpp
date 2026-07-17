#include <dispatcher/api/dispatcher_api.hpp>

namespace dispatcher::api
{
    DispatcherApi::DispatcherApi(
        dispatcher::runtime::DispatcherRuntime& runtime
    )
        : runtime_(&runtime)
        , runtime_api_(runtime)
        , telemetry_api_(runtime)
        , alarm_api_(runtime)
        , history_api_(runtime)
        , configuration_api_(runtime)
    {
    }

    dispatcher::runtime::DispatcherRuntime& DispatcherApi::runtime()
        noexcept
    {
        return *runtime_;
    }

    const dispatcher::runtime::DispatcherRuntime& DispatcherApi::runtime()
        const noexcept
    {
        return *runtime_;
    }

    RuntimeApi& DispatcherApi::runtime_api() noexcept
    {
        return runtime_api_;
    }

    const RuntimeApi& DispatcherApi::runtime_api() const noexcept
    {
        return runtime_api_;
    }

    TelemetryApi& DispatcherApi::telemetry_api() noexcept
    {
        return telemetry_api_;
    }

    const TelemetryApi& DispatcherApi::telemetry_api() const noexcept
    {
        return telemetry_api_;
    }

    AlarmApi& DispatcherApi::alarm_api() noexcept
    {
        return alarm_api_;
    }

    const AlarmApi& DispatcherApi::alarm_api() const noexcept
    {
        return alarm_api_;
    }

    HistoryApi& DispatcherApi::history_api() noexcept
    {
        return history_api_;
    }

    const HistoryApi& DispatcherApi::history_api() const noexcept
    {
        return history_api_;
    }

    ConfigurationApi& DispatcherApi::configuration_api() noexcept
    {
        return configuration_api_;
    }

    const ConfigurationApi& DispatcherApi::configuration_api() const noexcept
    {
        return configuration_api_;
    }

    DispatcherRuntimeApi& DispatcherApi::dispatcher_runtime_api() noexcept
    {
        return runtime_api_;
    }

    const DispatcherRuntimeApi& DispatcherApi::dispatcher_runtime_api()
        const noexcept
    {
        return runtime_api_;
    }

    DispatcherTelemetryApi& DispatcherApi::dispatcher_telemetry_api()
        noexcept
    {
        return telemetry_api_;
    }

    const DispatcherTelemetryApi& DispatcherApi::dispatcher_telemetry_api()
        const noexcept
    {
        return telemetry_api_;
    }

    DispatcherAlarmApi& DispatcherApi::dispatcher_alarm_api() noexcept
    {
        return alarm_api_;
    }

    const DispatcherAlarmApi& DispatcherApi::dispatcher_alarm_api()
        const noexcept
    {
        return alarm_api_;
    }

    DispatcherHistoryApi& DispatcherApi::dispatcher_history_api() noexcept
    {
        return history_api_;
    }

    const DispatcherHistoryApi& DispatcherApi::dispatcher_history_api()
        const noexcept
    {
        return history_api_;
    }

    DispatcherConfigurationApi& DispatcherApi::dispatcher_configuration_api()
        noexcept
    {
        return configuration_api_;
    }

    const DispatcherConfigurationApi&
        DispatcherApi::dispatcher_configuration_api() const noexcept
    {
        return configuration_api_;
    }
}