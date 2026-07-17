#include <dispatcher/telemetry/telemetry_write_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::telemetry
{
    TelemetryWriteResult TelemetryWriteResult::success(
        std::size_t written_count
    )
    {
        return TelemetryWriteResult(
            TelemetryAdapterStatus::Success,
            written_count,
            std::nullopt
        );
    }

    TelemetryWriteResult TelemetryWriteResult::failure(
        TelemetryAdapterStatus status,
        std::string operation,
        std::string adapter_name,
        std::string resource,
        std::string field,
        std::string message,
        std::size_t written_count
    )
    {
        if (is_success(status))
        {
            status = TelemetryAdapterStatus::UnknownError;
        }

        return TelemetryWriteResult(
            status,
            written_count,
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

    TelemetryWriteResult TelemetryWriteResult::failure(
        TelemetryAdapterError error,
        std::size_t written_count
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
                error.message(),
                written_count
            );
        }

        return TelemetryWriteResult(
            status,
            written_count,
            std::move(error)
        );
    }

    TelemetryWriteResult TelemetryWriteResult::from_adapter_result(
        const TelemetryAdapterResult& result,
        std::size_t written_count
    )
    {
        if (result.ok())
        {
            return success(written_count);
        }

        if (result.has_error())
        {
            return failure(
                result.error(),
                written_count
            );
        }

        return failure(
            result.status(),
            "adapter.write",
            {},
            {},
            {},
            "adapter write failed without detailed error",
            written_count
        );
    }

    bool TelemetryWriteResult::ok() const noexcept
    {
        return is_success(status_);
    }

    bool TelemetryWriteResult::failed() const noexcept
    {
        return !ok();
    }

    TelemetryAdapterStatus TelemetryWriteResult::status() const noexcept
    {
        return status_;
    }

    bool TelemetryWriteResult::retryable() const noexcept
    {
        return is_retryable(status_);
    }

    bool TelemetryWriteResult::has_error() const noexcept
    {
        return error_.has_value();
    }

    const TelemetryAdapterError& TelemetryWriteResult::error() const
    {
        if (!error_.has_value())
        {
            throw std::logic_error(
                "TelemetryWriteResult does not contain an error"
            );
        }

        return error_.value();
    }

    std::size_t TelemetryWriteResult::written_count() const noexcept
    {
        return written_count_;
    }

    bool TelemetryWriteResult::wrote_any() const noexcept
    {
        return written_count_ > 0;
    }

    bool TelemetryWriteResult::wrote_expected(
        std::size_t expected_count
    ) const noexcept
    {
        return written_count_ == expected_count;
    }

    TelemetryWriteResult::TelemetryWriteResult(
        TelemetryAdapterStatus status,
        std::size_t written_count,
        std::optional<TelemetryAdapterError> error
    )
        : status_(status)
        , written_count_(written_count)
        , error_(std::move(error))
    {
    }
}