#pragma once

#include <dispatcher/telemetry/telemetry_adapter_error.hpp>
#include <dispatcher/telemetry/telemetry_adapter_status.hpp>

#include <optional>
#include <string>

namespace dispatcher::telemetry
{
    class TelemetryAdapterResult
    {
    public:
        [[nodiscard]] static TelemetryAdapterResult success();

        [[nodiscard]] static TelemetryAdapterResult failure(
            TelemetryAdapterStatus status,
            std::string operation,
            std::string adapter_name = {},
            std::string resource = {},
            std::string field = {},
            std::string message = {}
        );

        [[nodiscard]] static TelemetryAdapterResult failure(
            TelemetryAdapterError error
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] TelemetryAdapterStatus status() const noexcept;

        [[nodiscard]] bool retryable() const noexcept;

        [[nodiscard]] bool has_error() const noexcept;

        [[nodiscard]] const TelemetryAdapterError& error() const;

    private:
        TelemetryAdapterResult(
            TelemetryAdapterStatus status,
            std::optional<TelemetryAdapterError> error
        );

        TelemetryAdapterStatus status_{ TelemetryAdapterStatus::UnknownError };
        std::optional<TelemetryAdapterError> error_;
    };
}