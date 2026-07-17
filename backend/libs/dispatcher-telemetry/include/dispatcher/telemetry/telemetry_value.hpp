#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/telemetry/tag_value.hpp>

#include <chrono>
#include <cstdint>

namespace dispatcher::telemetry
{
    class TelemetryValue
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

        TelemetryValue(
            dispatcher::domain::TagId tag_id,
            TagValue value,
            dispatcher::domain::Quality quality,
            TimePoint source_timestamp,
            TimePoint ingest_timestamp,
            std::uint64_t sequence
        );

        [[nodiscard]] const dispatcher::domain::TagId& tag_id() const noexcept;
        [[nodiscard]] const TagValue& value() const noexcept;
        [[nodiscard]] dispatcher::domain::Quality quality() const noexcept;
        [[nodiscard]] TimePoint source_timestamp() const noexcept;
        [[nodiscard]] TimePoint ingest_timestamp() const noexcept;
        [[nodiscard]] std::uint64_t sequence() const noexcept;

    private:
        dispatcher::domain::TagId tag_id_;
        TagValue value_;
        dispatcher::domain::Quality quality_;
        TimePoint source_timestamp_;
        TimePoint ingest_timestamp_;
        std::uint64_t sequence_;
    };
}