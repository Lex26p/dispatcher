#include <dispatcher/telemetry/telemetry_read_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::telemetry
{
    TelemetryReadResult TelemetryReadResult::success(
        std::vector<TelemetryValue> values
    )
    {
        return TelemetryReadResult(
            TelemetryAdapterStatus::Success,
            std::move(values),
            std::nullopt
        );
    }

    TelemetryReadResult TelemetryReadResult::success(
        TelemetryValue value
    )
    {
        std::vector<TelemetryValue> values;

        values.push_back(std::move(value));

        return success(std::move(values));
    }

    TelemetryReadResult TelemetryReadResult::failure(
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

        return TelemetryReadResult(
            status,
            {},
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

    TelemetryReadResult TelemetryReadResult::failure(
        TelemetryAdapterError error
    )
    {
        const auto status = error.status();

        if (is_success(status))
        {
            return failure(
                TelemetryAdapterStatus::UnknownError,
                error.operation(),
                error.adapter_name(),
                error.resource(),
                error.field(),
                error.message()
            );
        }

        return TelemetryReadResult(
            status,
            {},
            std::move(error)
        );
    }

    TelemetryReadResult TelemetryReadResult::from_adapter_result(
        const TelemetryAdapterResult& result
    )
    {
        if (result.ok())
        {
            return success(std::vector<TelemetryValue>{});
        }

        if (result.has_error())
        {
            return failure(result.error());
        }

        return failure(
            result.status(),
            "adapter.read",
            {},
            {},
            {},
            "adapter read failed without detailed error"
        );
    }

    bool TelemetryReadResult::ok() const noexcept
    {
        return is_success(status_);
    }

    bool TelemetryReadResult::failed() const noexcept
    {
        return !ok();
    }

    TelemetryAdapterStatus TelemetryReadResult::status() const noexcept
    {
        return status_;
    }

    bool TelemetryReadResult::retryable() const noexcept
    {
        return is_retryable(status_);
    }

    bool TelemetryReadResult::has_error() const noexcept
    {
        return error_.has_value();
    }

    const TelemetryAdapterError& TelemetryReadResult::error() const
    {
        if (!error_.has_value())
        {
            throw std::logic_error(
                "TelemetryReadResult does not contain an error"
            );
        }

        return error_.value();
    }

    const std::vector<TelemetryValue>& TelemetryReadResult::values()
        const noexcept
    {
        return values_;
    }

    bool TelemetryReadResult::empty() const noexcept
    {
        return values_.empty();
    }

    bool TelemetryReadResult::single() const noexcept
    {
        return values_.size() == 1;
    }

    bool TelemetryReadResult::batch() const noexcept
    {
        return values_.size() > 1;
    }

    std::size_t TelemetryReadResult::size() const noexcept
    {
        return values_.size();
    }

    const TelemetryValue& TelemetryReadResult::value() const
    {
        if (values_.empty())
        {
            throw std::logic_error(
                "TelemetryReadResult does not contain a value"
            );
        }

        return values_.front();
    }

    TelemetryReadResult::TelemetryReadResult(
        TelemetryAdapterStatus status,
        std::vector<TelemetryValue> values,
        std::optional<TelemetryAdapterError> error
    )
        : status_(status)
        , values_(std::move(values))
        , error_(std::move(error))
    {
    }
}