#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/telemetry/telemetry_adapter_capability.hpp>
#include <dispatcher/telemetry/telemetry_adapter_result.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <string>
#include <vector>

namespace dispatcher::telemetry
{
    class TelemetryAdapter
    {
    public:
        virtual ~TelemetryAdapter() = default;

        [[nodiscard]] virtual std::string name() const = 0;

        [[nodiscard]] virtual TelemetryAdapterCapabilities capabilities()
            const noexcept = 0;

        [[nodiscard]] virtual bool connected() const noexcept = 0;

        [[nodiscard]] virtual TelemetryAdapterResult connect() = 0;

        [[nodiscard]] virtual TelemetryAdapterResult disconnect() = 0;

        [[nodiscard]] virtual TelemetryAdapterResult health_check() = 0;

        [[nodiscard]] virtual TelemetryAdapterResult read_current(
            const dispatcher::domain::TagId& tag_id,
            TelemetryValue& value
        ) = 0;

        [[nodiscard]] virtual TelemetryAdapterResult read_current_batch(
            const std::vector<dispatcher::domain::TagId>& tag_ids,
            std::vector<TelemetryValue>& values
        ) = 0;

        [[nodiscard]] virtual TelemetryAdapterResult write_current(
            const dispatcher::domain::TagId& tag_id,
            const TelemetryValue& value
        ) = 0;
    };
}