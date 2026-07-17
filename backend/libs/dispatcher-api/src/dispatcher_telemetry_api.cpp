#include <dispatcher/api/dispatcher_telemetry_api.hpp>

#include <dispatcher/api/api_status_mapping.hpp>

namespace dispatcher::api
{
    DispatcherTelemetryApi::DispatcherTelemetryApi(
        dispatcher::runtime::DispatcherRuntime& runtime
    )
        : runtime_(&runtime)
    {
    }

    TelemetryIngestApiResult DispatcherTelemetryApi::ingest(
        const TelemetryIngestRequest& request
    )
    {
        if (runtime_ == nullptr)
        {
            return TelemetryIngestApiResult::failure(
                ApiStatus::InternalError,
                "telemetry.ingest",
                request.tag_id.value(),
                {},
                "runtime instance is not available"
            );
        }

        const auto summary = runtime_->process(
            request.to_telemetry_value()
        );

        const auto status = map_runtime_process_summary_to_api_status(
            summary
        );

        if (status == ApiStatus::Success)
        {
            return TelemetryIngestApiResult::success(summary);
        }

        return TelemetryIngestApiResult::failure(
            status,
            "telemetry.ingest",
            request.tag_id.value(),
            {},
            telemetry_ingest_status_message(summary.telemetry_status)
        );
    }

    dispatcher::runtime::DispatcherRuntime&
        DispatcherTelemetryApi::runtime() noexcept
    {
        return *runtime_;
    }

    const dispatcher::runtime::DispatcherRuntime&
        DispatcherTelemetryApi::runtime() const noexcept
    {
        return *runtime_;
    }
}