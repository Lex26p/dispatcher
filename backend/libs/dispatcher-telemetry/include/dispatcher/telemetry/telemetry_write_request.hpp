#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace dispatcher::telemetry
{
    class TelemetryWriteRequest
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

        TelemetryWriteRequest() = default;

        explicit TelemetryWriteRequest(
            TelemetryValue value,
            std::string source = {}
        );

        TelemetryWriteRequest(
            std::vector<TelemetryValue> values,
            std::string source = {}
        );

        [[nodiscard]] static TelemetryWriteRequest single(
            TelemetryValue value,
            std::string source = {}
        );

        [[nodiscard]] static TelemetryWriteRequest batch(
            std::vector<TelemetryValue> values,
            std::string source = {}
        );

        [[nodiscard]] const std::vector<TelemetryValue>& values()
            const noexcept;

        [[nodiscard]] const std::string& source() const noexcept;

        [[nodiscard]] TimePoint requested_at() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool single() const noexcept;

        [[nodiscard]] bool batch() const noexcept;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool has_source() const noexcept;

        [[nodiscard]] bool contains_tag(
            const dispatcher::domain::TagId& tag_id
        ) const noexcept;

    private:
        std::vector<TelemetryValue> values_;
        std::string source_;
        TimePoint requested_at_{ Clock::now() };
    };
}