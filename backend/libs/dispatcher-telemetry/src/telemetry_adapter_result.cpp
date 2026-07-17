#include <dispatcher/telemetry/telemetry_adapter_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::telemetry
{
    TelemetryAdapterResult TelemetryAdapterResult::success()
    {
        return TelemetryAdapterResult(
            TelemetryAdapterStatus::Success,
            std::nullopt
        );
    }

    TelemetryAdapterResult TelemetryAdapterResult::failure(
        TelemetryAdapterStatus status,
        std::string operation,
        std::string adapter_name,
        std::string resource,
        std::string field,
        std::string message
    )
    {
        if (is_success(status))
        {
            status = TelemetryAdapterStatus::UnknownError;
        }

        return TelemetryAdapterResult(
            status,
            TelemetryAdapterError(
                status,
                std::move(operation),
                std::move(adapter_name),
                std::move(resource),
                std::move(field),
                std::move(message)
            )
        );
    }

    TelemetryAdapterResult TelemetryAdapterResult::failure(
        TelemetryAdapterError error
    )
    {
        auto status = error.status();

        if (is_success(status))
        {
            status = TelemetryAdapterStatus::UnknownError;

            return failure(
                status,
                error.operation(),
                error.adapter_name(),
                error.resource(),
                error.field(),
                error.message()
            );
        }

        return TelemetryAdapterResult(
            status,
            std::move(error)
        );
    }

    bool TelemetryAdapterResult::ok() const noexcept
    {
        return is_success(status_);
    }

    bool TelemetryAdapterResult::failed() const noexcept
    {
        return !ok();
    }

    TelemetryAdapterStatus TelemetryAdapterResult::status() const noexcept
    {
        return status_;
    }

    bool TelemetryAdapterResult::retryable() const noexcept
    {
        return is_retryable(status_);
    }

    bool TelemetryAdapterResult::has_error() const noexcept
    {
        return error_.has_value();
    }

    const TelemetryAdapterError& TelemetryAdapterResult::error() const
    {
        if (!error_.has_value())
        {
            throw std::logic_error(
                "TelemetryAdapterResult does not contain an error"
            );
        }

        return error_.value();
    }

    TelemetryAdapterResult::TelemetryAdapterResult(
        TelemetryAdapterStatus status,
        std::optional<TelemetryAdapterError> error
    )
        : status_(status)
        , error_(std::move(error))
    {
    }
}