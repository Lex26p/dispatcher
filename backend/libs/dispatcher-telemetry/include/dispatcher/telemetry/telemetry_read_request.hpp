#pragma once

#include <dispatcher/domain/id_types.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace dispatcher::telemetry
{
    class TelemetryReadRequest
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

        TelemetryReadRequest() = default;

        explicit TelemetryReadRequest(
            dispatcher::domain::TagId tag_id,
            std::string source = {}
        );

        TelemetryReadRequest(
            std::vector<dispatcher::domain::TagId> tag_ids,
            std::string source = {}
        );

        [[nodiscard]] static TelemetryReadRequest single(
            dispatcher::domain::TagId tag_id,
            std::string source = {}
        );

        [[nodiscard]] static TelemetryReadRequest batch(
            std::vector<dispatcher::domain::TagId> tag_ids,
            std::string source = {}
        );

        [[nodiscard]] const std::vector<dispatcher::domain::TagId>& tag_ids()
            const noexcept;

        [[nodiscard]] const std::string& source() const noexcept;

        [[nodiscard]] TimePoint requested_at() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool single() const noexcept;

        [[nodiscard]] bool batch() const noexcept;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool has_source() const noexcept;

        [[nodiscard]] bool contains(
            const dispatcher::domain::TagId& tag_id
        ) const noexcept;

    private:
        std::vector<dispatcher::domain::TagId> tag_ids_;
        std::string source_;
        TimePoint requested_at_{ Clock::now() };
    };
}