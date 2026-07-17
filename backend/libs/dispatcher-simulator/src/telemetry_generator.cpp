#include <dispatcher/simulator/telemetry_generator.hpp>

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/telemetry/tag_value.hpp>

#include <chrono>
#include <string>

namespace dispatcher::simulator
{
    TelemetryGenerator::TelemetryGenerator(std::size_t tag_count)
        : tag_count_(tag_count)
    {
    }

    std::size_t TelemetryGenerator::tag_count() const noexcept
    {
        return tag_count_;
    }

    std::uint64_t TelemetryGenerator::sequence() const noexcept
    {
        return sequence_;
    }

    std::vector<dispatcher::telemetry::TelemetryValue> TelemetryGenerator::next_batch()
    {
        using dispatcher::domain::Quality;
        using dispatcher::domain::TagId;
        using dispatcher::telemetry::TagValue;
        using dispatcher::telemetry::TelemetryValue;

        ++sequence_;

        std::vector<TelemetryValue> values;
        values.reserve(tag_count_);

        const auto now = TelemetryValue::Clock::now();

        for (std::size_t index = 0; index < tag_count_; ++index)
        {
            values.emplace_back(
                TagId{ make_tag_id(index) },
                TagValue(make_value(index)),
                Quality::Good,
                now,
                now,
                sequence_
            );
        }

        return values;
    }

    std::string TelemetryGenerator::make_tag_id(std::size_t index) const
    {
        return "tag-" + std::to_string(index + 1);
    }

    double TelemetryGenerator::make_value(std::size_t index) const
    {
        return static_cast<double>(index) + static_cast<double>(sequence_) * 0.1;
    }
}