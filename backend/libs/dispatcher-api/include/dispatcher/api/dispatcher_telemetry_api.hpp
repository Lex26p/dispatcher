#pragma once

#include <dispatcher/api/telemetry_api.hpp>
#include <dispatcher/runtime/dispatcher_runtime.hpp>

namespace dispatcher::api
{
    class DispatcherTelemetryApi final : public TelemetryApi
    {
    public:
        explicit DispatcherTelemetryApi(
            dispatcher::runtime::DispatcherRuntime& runtime
        );

        [[nodiscard]] TelemetryIngestApiResult ingest(
            const TelemetryIngestRequest& request
        ) override;

        [[nodiscard]] dispatcher::runtime::DispatcherRuntime& runtime()
            noexcept;

        [[nodiscard]] const dispatcher::runtime::DispatcherRuntime& runtime()
            const noexcept;

    private:
        dispatcher::runtime::DispatcherRuntime* runtime_{ nullptr };
    };
}