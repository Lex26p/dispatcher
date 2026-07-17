#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <cstdint>

namespace dispatcher::history
{
    class HistorySample
    {
    public:
        using Clock = dispatcher::telemetry::TelemetryValue::Clock;
        using TimePoint = dispatcher::telemetry::TelemetryValue::TimePoint;

        explicit HistorySample(
            dispatcher::telemetry::TelemetryValue telemetry_value
        );

        [[nodiscard]] static HistorySample from_telemetry_value(
            dispatcher::telemetry::TelemetryValue telemetry_value
        );

        [[nodiscard]] const dispatcher::telemetry::TelemetryValue& telemetry_value()
            const noexcept;

        [[nodiscard]] const dispatcher::domain::TagId& tag_id() const noexcept;

        [[nodiscard]] const dispatcher::telemetry::TagValue& value() const noexcept;

        [[nodiscard]] dispatcher::domain::Quality quality() const noexcept;

        [[nodiscard]] TimePoint source_timestamp() const noexcept;

        [[nodiscard]] TimePoint ingest_timestamp() const noexcept;

        [[nodiscard]] std::uint64_t sequence() const noexcept;

    private:
        dispatcher::telemetry::TelemetryValue telemetry_value_;
    };
}