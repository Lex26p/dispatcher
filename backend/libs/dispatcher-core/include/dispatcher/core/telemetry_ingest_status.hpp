#pragma once

#include <string_view>

namespace dispatcher::core
{
    enum class TelemetryIngestStatus
    {
        Accepted,
        AcceptedNoChange,
        UnknownTag,
        DisabledTag,
        DataTypeMismatch,
        StaleSequence,
        FutureSourceTimestamp,
        BadQuality
    };

    constexpr std::string_view to_string(TelemetryIngestStatus status)
    {
        switch (status)
        {
        case TelemetryIngestStatus::Accepted:
            return "accepted";
        case TelemetryIngestStatus::AcceptedNoChange:
            return "accepted_no_change";
        case TelemetryIngestStatus::UnknownTag:
            return "unknown_tag";
        case TelemetryIngestStatus::DisabledTag:
            return "disabled_tag";
        case TelemetryIngestStatus::DataTypeMismatch:
            return "data_type_mismatch";
        case TelemetryIngestStatus::StaleSequence:
            return "stale_sequence";
        case TelemetryIngestStatus::FutureSourceTimestamp:
            return "future_source_timestamp";
        case TelemetryIngestStatus::BadQuality:
            return "bad_quality";
        }

        return "unknown";
    }
}