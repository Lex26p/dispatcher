#include <dispatcher/api/telemetry_ingest_api_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::api
{
    TelemetryIngestApiResult TelemetryIngestApiResult::success(
        dispatcher::runtime::DispatcherRuntimeProcessSummary summary
    )
    {
        return TelemetryIngestApiResult(
            ApiResult::success(),
            summary
        );
    }

    TelemetryIngestApiResult TelemetryIngestApiResult::failure(
        ApiStatus status,
        std::string operation,
        std::string resource,
        std::string field,
        std::string message
    )
    {
        return TelemetryIngestApiResult(
            ApiResult::failure(
                status,
                std::move(operation),
                std::move(resource),
                std::move(field),
                std::move(message)
            ),
            std::nullopt
        );
    }

    bool TelemetryIngestApiResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool TelemetryIngestApiResult::failed() const noexcept
    {
        return result_.failed();
    }

    ApiStatus TelemetryIngestApiResult::status() const noexcept
    {
        return result_.status();
    }

    const ApiResult& TelemetryIngestApiResult::result() const noexcept
    {
        return result_;
    }

    const ApiError& TelemetryIngestApiResult::error() const noexcept
    {
        return result_.error();
    }

    bool TelemetryIngestApiResult::has_summary() const noexcept
    {
        return summary_.has_value();
    }

    const dispatcher::runtime::DispatcherRuntimeProcessSummary&
        TelemetryIngestApiResult::summary() const
    {
        if (!summary_.has_value())
        {
            throw std::logic_error(
                "TelemetryIngestApiResult does not contain a summary"
            );
        }

        return summary_.value();
    }

    TelemetryIngestApiResult::TelemetryIngestApiResult(
        ApiResult result,
        std::optional<dispatcher::runtime::DispatcherRuntimeProcessSummary>
        summary
    )
        : result_(std::move(result))
        , summary_(std::move(summary))
    {
    }
}