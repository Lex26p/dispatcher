#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>

namespace dispatcher::core
{
    class CurrentStateStore
    {
    public:
        using TelemetryValue = dispatcher::telemetry::TelemetryValue;
        using TagId = dispatcher::domain::TagId;

        CurrentStateStore() = default;

        void update(TelemetryValue value);

        [[nodiscard]] std::optional<TelemetryValue> find(const TagId& tag_id) const;

        [[nodiscard]] bool contains(const TagId& tag_id) const;

        [[nodiscard]] std::size_t size() const noexcept;

        void clear();

    private:
        std::unordered_map<std::string, TelemetryValue> values_;
    };
}