#include <dispatcher/telemetry/telemetry_adapter_error.hpp>

#include <utility>

namespace dispatcher::telemetry
{
    TelemetryAdapterError::TelemetryAdapterError(
        TelemetryAdapterStatus status,
        std::string operation,
        std::string adapter_name,
        std::string resource,
        std::string field,
        std::string message
    )
        : status_(status)
        , operation_(std::move(operation))
        , adapter_name_(std::move(adapter_name))
        , resource_(std::move(resource))
        , field_(std::move(field))
        , message_(std::move(message))
    {
    }

    TelemetryAdapterStatus TelemetryAdapterError::status() const noexcept
    {
        return status_;
    }

    const std::string& TelemetryAdapterError::operation() const noexcept
    {
        return operation_;
    }

    const std::string& TelemetryAdapterError::adapter_name() const noexcept
    {
        return adapter_name_;
    }

    const std::string& TelemetryAdapterError::resource() const noexcept
    {
        return resource_;
    }

    const std::string& TelemetryAdapterError::field() const noexcept
    {
        return field_;
    }

    const std::string& TelemetryAdapterError::message() const noexcept
    {
        return message_;
    }

    bool TelemetryAdapterError::has_operation() const noexcept
    {
        return !operation_.empty();
    }

    bool TelemetryAdapterError::has_adapter_name() const noexcept
    {
        return !adapter_name_.empty();
    }

    bool TelemetryAdapterError::has_resource() const noexcept
    {
        return !resource_.empty();
    }

    bool TelemetryAdapterError::has_field() const noexcept
    {
        return !field_.empty();
    }

    bool TelemetryAdapterError::has_message() const noexcept
    {
        return !message_.empty();
    }

    bool TelemetryAdapterError::retryable() const noexcept
    {
        return is_retryable(status_);
    }
}