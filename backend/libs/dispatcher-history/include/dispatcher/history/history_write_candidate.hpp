#pragma once

#include <dispatcher/core/telemetry_ingest_result.hpp>
#include <dispatcher/domain/history_policy.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

namespace dispatcher::history
{
    struct HistoryWriteCandidate
    {
        dispatcher::core::TelemetryIngestResult ingest_result;
        dispatcher::telemetry::TelemetryValue telemetry_value;
        dispatcher::domain::HistoryPolicy history_policy{
            dispatcher::domain::HistoryPolicy::Disabled
        };
    };
}