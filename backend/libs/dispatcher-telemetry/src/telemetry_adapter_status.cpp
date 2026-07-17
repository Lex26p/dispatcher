#include <dispatcher/telemetry/telemetry_adapter_status.hpp>

namespace dispatcher::telemetry
{
    const char* to_string(TelemetryAdapterStatus status) noexcept
    {
        switch (status)
        {
        case TelemetryAdapterStatus::Success:
            return "success";

        case TelemetryAdapterStatus::NotConnected:
            return "not_connected";

        case TelemetryAdapterStatus::ConnectionFailed:
            return "connection_failed";

        case TelemetryAdapterStatus::Timeout:
            return "timeout";

        case TelemetryAdapterStatus::InvalidConfiguration:
            return "invalid_configuration";

        case TelemetryAdapterStatus::InvalidRequest:
            return "invalid_request";

        case TelemetryAdapterStatus::UnsupportedOperation:
            return "unsupported_operation";

        case TelemetryAdapterStatus::UnknownTag:
            return "unknown_tag";

        case TelemetryAdapterStatus::BadPayload:
            return "bad_payload";

        case TelemetryAdapterStatus::DecodeError:
            return "decode_error";

        case TelemetryAdapterStatus::Backpressure:
            return "backpressure";

        case TelemetryAdapterStatus::IoError:
            return "io_error";

        case TelemetryAdapterStatus::UnknownError:
            return "unknown_error";
        }

        return "unknown_error";
    }

    bool is_success(TelemetryAdapterStatus status) noexcept
    {
        return status == TelemetryAdapterStatus::Success;
    }

    bool is_failure(TelemetryAdapterStatus status) noexcept
    {
        return !is_success(status);
    }

    bool is_retryable(TelemetryAdapterStatus status) noexcept
    {
        switch (status)
        {
        case TelemetryAdapterStatus::NotConnected:
        case TelemetryAdapterStatus::ConnectionFailed:
        case TelemetryAdapterStatus::Timeout:
        case TelemetryAdapterStatus::Backpressure:
        case TelemetryAdapterStatus::IoError:
        case TelemetryAdapterStatus::UnknownError:
            return true;

        case TelemetryAdapterStatus::Success:
        case TelemetryAdapterStatus::InvalidConfiguration:
        case TelemetryAdapterStatus::InvalidRequest:
        case TelemetryAdapterStatus::UnsupportedOperation:
        case TelemetryAdapterStatus::UnknownTag:
        case TelemetryAdapterStatus::BadPayload:
        case TelemetryAdapterStatus::DecodeError:
            return false;
        }

        return true;
    }
}