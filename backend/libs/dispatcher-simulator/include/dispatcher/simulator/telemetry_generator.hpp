#pragma once

#include <dispatcher/telemetry/telemetry_value.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace dispatcher::simulator
{
    class TelemetryGenerator
    {
    public:
        explicit TelemetryGenerator(std::size_t tag_count);

        [[nodiscard]] std::size_t tag_count() const noexcept;

        [[nodiscard]] std::uint64_t sequence() const noexcept;

        [[nodiscard]] std::vector<dispatcher::telemetry::TelemetryValue> next_batch();

    private:
        std::size_t tag_count_;
        std::uint64_t sequence_{ 0 };

        [[nodiscard]] std::string make_tag_id(std::size_t index) const;
        [[nodiscard]] double make_value(std::size_t index) const;
    };
}