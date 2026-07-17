#pragma once

#include <dispatcher/telemetry/telemetry_adapter.hpp>
#include <dispatcher/telemetry/telemetry_read_request.hpp>
#include <dispatcher/telemetry/telemetry_read_result.hpp>
#include <dispatcher/telemetry/telemetry_write_request.hpp>
#include <dispatcher/telemetry/telemetry_write_result.hpp>

namespace dispatcher::telemetry
{
    [[nodiscard]] TelemetryReadResult read_from_adapter(
        TelemetryAdapter* adapter,
        const TelemetryReadRequest& request
    );

    [[nodiscard]] TelemetryReadResult read_from_adapter(
        TelemetryAdapter& adapter,
        const TelemetryReadRequest& request
    );

    [[nodiscard]] TelemetryWriteResult write_to_adapter(
        TelemetryAdapter* adapter,
        const TelemetryWriteRequest& request
    );

    [[nodiscard]] TelemetryWriteResult write_to_adapter(
        TelemetryAdapter& adapter,
        const TelemetryWriteRequest& request
    );
}