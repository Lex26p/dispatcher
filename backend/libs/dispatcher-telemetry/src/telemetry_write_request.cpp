#include <dispatcher/telemetry/telemetry_write_request.hpp>

#include <algorithm>
#include <utility>

namespace dispatcher::telemetry
{
    TelemetryWriteRequest::TelemetryWriteRequest(
        TelemetryValue value,
        std::string source
    )
        : values_{ std::move(value) }
        , source_(std::move(source))
        , requested_at_(Clock::now())
    {
    }

    TelemetryWriteRequest::TelemetryWriteRequest(
        std::vector<TelemetryValue> values,
        std::string source
    )
        : values_(std::move(values))
        , source_(std::move(source))
        , requested_at_(Clock::now())
    {
    }

    TelemetryWriteRequest TelemetryWriteRequest::single(
        TelemetryValue value,
        std::string source
    )
    {
        return TelemetryWriteRequest(
            std::move(value),
            std::move(source)
        );
    }

    TelemetryWriteRequest TelemetryWriteRequest::batch(
        std::vector<TelemetryValue> values,
        std::string source
    )
    {
        return TelemetryWriteRequest(
            std::move(values),
            std::move(source)
        );
    }

    const std::vector<TelemetryValue>& TelemetryWriteRequest::values()
        const noexcept
    {
        return values_;
    }

    const std::string& TelemetryWriteRequest::source() const noexcept
    {
        return source_;
    }

    TelemetryWriteRequest::TimePoint TelemetryWriteRequest::requested_at()
        const noexcept
    {
        return requested_at_;
    }

    bool TelemetryWriteRequest::empty() const noexcept
    {
        return values_.empty();
    }

    bool TelemetryWriteRequest::single() const noexcept
    {
        return values_.size() == 1;
    }

    bool TelemetryWriteRequest::batch() const noexcept
    {
        return values_.size() > 1;
    }

    std::size_t TelemetryWriteRequest::size() const noexcept
    {
        return values_.size();
    }

    bool TelemetryWriteRequest::has_source() const noexcept
    {
        return !source_.empty();
    }

    bool TelemetryWriteRequest::contains_tag(
        const dispatcher::domain::TagId& tag_id
    ) const noexcept
    {
        return std::any_of(
            values_.begin(),
            values_.end(),
            [&tag_id](const TelemetryValue& value)
            {
                return value.tag_id() == tag_id;
            }
        );
    }
}