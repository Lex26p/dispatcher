#pragma once

namespace dispatcher::telemetry
{
    enum class TelemetryAdapterStatus
    {
        Success,

        NotConnected,
        ConnectionFailed,
        Timeout,

        InvalidConfiguration,
        InvalidRequest,
        UnsupportedOperation,

        UnknownTag,
        BadPayload,
        DecodeError,

        Backpressure,
        IoError,
        UnknownError
    };

    [[nodiscard]] const char* to_string(
        TelemetryAdapterStatus status
    ) noexcept;

    [[nodiscard]] bool is_success(
        TelemetryAdapterStatus status
    ) noexcept;

    [[nodiscard]] bool is_failure(
        TelemetryAdapterStatus status
    ) noexcept;

    [[nodiscard]] bool is_retryable(
        TelemetryAdapterStatus status
    ) noexcept;
}