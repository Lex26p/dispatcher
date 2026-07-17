#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <cstdint>
#include <optional>

namespace dispatcher::api
{
    struct TelemetryIngestRequest
    {
        dispatcher::domain::TagId tag_id;
        dispatcher::telemetry::TagValue value;
        dispatcher::domain::Quality quality{ dispatcher::domain::Quality::Good };
        std::uint64_t sequence{ 0 };

        std::optional<dispatcher::telemetry::TelemetryValue::TimePoint>
            source_timestamp;
        std::optional<dispatcher::telemetry::TelemetryValue::TimePoint>
            ingest_timestamp;

        [[nodiscard]] bool has_source_timestamp() const noexcept;

        [[nodiscard]] bool has_ingest_timestamp() const noexcept;

        [[nodiscard]] dispatcher::telemetry::TelemetryValue
            to_telemetry_value() const;
    };
}