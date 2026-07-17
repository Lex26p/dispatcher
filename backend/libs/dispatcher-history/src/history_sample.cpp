#include <dispatcher/history/history_sample.hpp>

#include <utility>

namespace dispatcher::history
{
    HistorySample::HistorySample(
        dispatcher::telemetry::TelemetryValue telemetry_value
    )
        : telemetry_value_(std::move(telemetry_value))
    {
    }

    HistorySample HistorySample::from_telemetry_value(
        dispatcher::telemetry::TelemetryValue telemetry_value
    )
    {
        return HistorySample(std::move(telemetry_value));
    }

    const dispatcher::telemetry::TelemetryValue& HistorySample::telemetry_value()
        const noexcept
    {
        return telemetry_value_;
    }

    const dispatcher::domain::TagId& HistorySample::tag_id() const noexcept
    {
        return telemetry_value_.tag_id();
    }

    const dispatcher::telemetry::TagValue& HistorySample::value() const noexcept
    {
        return telemetry_value_.value();
    }

    dispatcher::domain::Quality HistorySample::quality() const noexcept
    {
        return telemetry_value_.quality();
    }

    HistorySample::TimePoint HistorySample::source_timestamp() const noexcept
    {
        return telemetry_value_.source_timestamp();
    }

    HistorySample::TimePoint HistorySample::ingest_timestamp() const noexcept
    {
        return telemetry_value_.ingest_timestamp();
    }

    std::uint64_t HistorySample::sequence() const noexcept
    {
        return telemetry_value_.sequence();
    }
}