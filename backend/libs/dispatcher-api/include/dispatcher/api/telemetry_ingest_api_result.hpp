#pragma once

#include <dispatcher/api/api_result.hpp>
#include <dispatcher/api/api_status.hpp>
#include <dispatcher/runtime/dispatcher_runtime_process_summary.hpp>

#include <optional>
#include <string>

namespace dispatcher::api
{
    class TelemetryIngestApiResult
    {
    public:
        [[nodiscard]] static TelemetryIngestApiResult success(
            dispatcher::runtime::DispatcherRuntimeProcessSummary summary
        );

        [[nodiscard]] static TelemetryIngestApiResult failure(
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

        [[nodiscard]] bool has_summary() const noexcept;

        [[nodiscard]] const dispatcher::runtime::DispatcherRuntimeProcessSummary&
            summary() const;

    private:
        TelemetryIngestApiResult(
            ApiResult result,
            std::optional<dispatcher::runtime::DispatcherRuntimeProcessSummary>
            summary
        );

        ApiResult result_;
        std::optional<dispatcher::runtime::DispatcherRuntimeProcessSummary>
            summary_;
    };
}