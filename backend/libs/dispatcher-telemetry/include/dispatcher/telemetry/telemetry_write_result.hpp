#pragma once

#include <dispatcher/telemetry/telemetry_adapter_error.hpp>
#include <dispatcher/telemetry/telemetry_adapter_result.hpp>
#include <dispatcher/telemetry/telemetry_adapter_status.hpp>

#include <cstddef>
#include <optional>
#include <string>

namespace dispatcher::telemetry
{
    class TelemetryWriteResult
    {
    public:
        [[nodiscard]] static TelemetryWriteResult success(
            std::size_t written_count = 0
        );

        [[nodiscard]] static TelemetryWriteResult failure(
            TelemetryAdapterStatus status,
            std::string operation,
            std::string adapter_name = {},
            std::string resource = {},
            std::string field = {},
            std::string message = {},
            std::size_t written_count = 0
        );

        [[nodiscard]] static TelemetryWriteResult failure(
            TelemetryAdapterError error,
            std::size_t written_count = 0
        );

        [[nodiscard]] static TelemetryWriteResult from_adapter_result(
            const TelemetryAdapterResult& result,
            std::size_t written_count = 0
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] TelemetryAdapterStatus status() const noexcept;

        [[nodiscard]] bool retryable() const noexcept;

        [[nodiscard]] bool has_error() const noexcept;

        [[nodiscard]] const TelemetryAdapterError& error() const;

        [[nodiscard]] std::size_t written_count() const noexcept;

        [[nodiscard]] bool wrote_any() const noexcept;

        [[nodiscard]] bool wrote_expected(
            std::size_t expected_count
        ) const noexcept;

    private:
        TelemetryWriteResult(
            TelemetryAdapterStatus status,
            std::size_t written_count,
            std::optional<TelemetryAdapterError> error
        );

        TelemetryAdapterStatus status_{ TelemetryAdapterStatus::UnknownError };
        std::size_t written_count_{ 0 };
        std::optional<TelemetryAdapterError> error_;
    };
}