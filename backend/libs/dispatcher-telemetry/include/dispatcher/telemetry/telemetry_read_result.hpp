#pragma once

#include <dispatcher/telemetry/telemetry_adapter_error.hpp>
#include <dispatcher/telemetry/telemetry_adapter_result.hpp>
#include <dispatcher/telemetry/telemetry_adapter_status.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace dispatcher::telemetry
{
    class TelemetryReadResult
    {
    public:
        [[nodiscard]] static TelemetryReadResult success(
            std::vector<TelemetryValue> values
        );

        [[nodiscard]] static TelemetryReadResult success(
            TelemetryValue value
        );

        [[nodiscard]] static TelemetryReadResult failure(
            TelemetryAdapterStatus status,
            std::string operation,
            std::string adapter_name = {},
            std::string resource = {},
            std::string field = {},
            std::string message = {}
        );

        [[nodiscard]] static TelemetryReadResult failure(
            TelemetryAdapterError error
        );

        [[nodiscard]] static TelemetryReadResult from_adapter_result(
            const TelemetryAdapterResult& result
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] TelemetryAdapterStatus status() const noexcept;

        [[nodiscard]] bool retryable() const noexcept;

        [[nodiscard]] bool has_error() const noexcept;

        [[nodiscard]] const TelemetryAdapterError& error() const;

        [[nodiscard]] const std::vector<TelemetryValue>& values()
            const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool single() const noexcept;

        [[nodiscard]] bool batch() const noexcept;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] const TelemetryValue& value() const;

    private:
        TelemetryReadResult(
            TelemetryAdapterStatus status,
            std::vector<TelemetryValue> values,
            std::optional<TelemetryAdapterError> error
        );

        TelemetryAdapterStatus status_{ TelemetryAdapterStatus::UnknownError };
        std::vector<TelemetryValue> values_;
        std::optional<TelemetryAdapterError> error_;
    };
}