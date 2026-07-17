#include <dispatcher/telemetry/telemetry_value.hpp>

#include <utility>

namespace dispatcher::telemetry
{
    TelemetryValue::TelemetryValue(
        dispatcher::domain::TagId tag_id,
        TagValue value,
        dispatcher::domain::Quality quality,
        TimePoint source_timestamp,
        TimePoint ingest_timestamp,
        std::uint64_t sequence
    )
        : tag_id_(std::move(tag_id))
        , value_(std::move(value))
        , quality_(quality)
        , source_timestamp_(source_timestamp)
        , ingest_timestamp_(ingest_timestamp)
        , sequence_(sequence)
    {
    }

    const dispatcher::domain::TagId& TelemetryValue::tag_id() const noexcept
    {
        return tag_id_;
    }

    const TagValue& TelemetryValue::value() const noexcept
    {
        return value_;
    }

    dispatcher::domain::Quality TelemetryValue::quality() const noexcept
    {
        return quality_;
    }

    TelemetryValue::TimePoint TelemetryValue::source_timestamp() const noexcept
    {
        return source_timestamp_;
    }

    TelemetryValue::TimePoint TelemetryValue::ingest_timestamp() const noexcept
    {
        return ingest_timestamp_;
    }

    std::uint64_t TelemetryValue::sequence() const noexcept
    {
        return sequence_;
    }
}