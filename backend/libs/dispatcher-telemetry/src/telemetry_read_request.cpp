#include <dispatcher/telemetry/telemetry_read_request.hpp>

#include <algorithm>
#include <utility>

namespace dispatcher::telemetry
{
    TelemetryReadRequest::TelemetryReadRequest(
        dispatcher::domain::TagId tag_id,
        std::string source
    )
        : tag_ids_{ std::move(tag_id) }
        , source_(std::move(source))
        , requested_at_(Clock::now())
    {
    }

    TelemetryReadRequest::TelemetryReadRequest(
        std::vector<dispatcher::domain::TagId> tag_ids,
        std::string source
    )
        : tag_ids_(std::move(tag_ids))
        , source_(std::move(source))
        , requested_at_(Clock::now())
    {
    }

    TelemetryReadRequest TelemetryReadRequest::single(
        dispatcher::domain::TagId tag_id,
        std::string source
    )
    {
        return TelemetryReadRequest(
            std::move(tag_id),
            std::move(source)
        );
    }

    TelemetryReadRequest TelemetryReadRequest::batch(
        std::vector<dispatcher::domain::TagId> tag_ids,
        std::string source
    )
    {
        return TelemetryReadRequest(
            std::move(tag_ids),
            std::move(source)
        );
    }

    const std::vector<dispatcher::domain::TagId>&
        TelemetryReadRequest::tag_ids() const noexcept
    {
        return tag_ids_;
    }

    const std::string& TelemetryReadRequest::source() const noexcept
    {
        return source_;
    }

    TelemetryReadRequest::TimePoint TelemetryReadRequest::requested_at()
        const noexcept
    {
        return requested_at_;
    }

    bool TelemetryReadRequest::empty() const noexcept
    {
        return tag_ids_.empty();
    }

    bool TelemetryReadRequest::single() const noexcept
    {
        return tag_ids_.size() == 1;
    }

    bool TelemetryReadRequest::batch() const noexcept
    {
        return tag_ids_.size() > 1;
    }

    std::size_t TelemetryReadRequest::size() const noexcept
    {
        return tag_ids_.size();
    }

    bool TelemetryReadRequest::has_source() const noexcept
    {
        return !source_.empty();
    }

    bool TelemetryReadRequest::contains(
        const dispatcher::domain::TagId& tag_id
    ) const noexcept
    {
        return std::find(
            tag_ids_.begin(),
            tag_ids_.end(),
            tag_id
        ) != tag_ids_.end();
    }
}