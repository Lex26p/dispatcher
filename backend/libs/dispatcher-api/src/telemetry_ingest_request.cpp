#include <dispatcher/api/telemetry_ingest_request.hpp>

namespace dispatcher::api
{
    bool TelemetryIngestRequest::has_source_timestamp() const noexcept
    {
        return source_timestamp.has_value();
    }

    bool TelemetryIngestRequest::has_ingest_timestamp() const noexcept
    {
        return ingest_timestamp.has_value();
    }

    dispatcher::telemetry::TelemetryValue
        TelemetryIngestRequest::to_telemetry_value() const
    {
        const auto now = dispatcher::telemetry::TelemetryValue::Clock::now();

        return dispatcher::telemetry::TelemetryValue(
            tag_id,
            value,
            quality,
            source_timestamp.value_or(now),
            ingest_timestamp.value_or(now),
            sequence
        );
    }
}