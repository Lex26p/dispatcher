#pragma once

#include <dispatcher/core/telemetry_ingest_status.hpp>

#include <string>
#include <utility>

namespace dispatcher::core
{
    class TelemetryIngestResult
    {
    public:
        TelemetryIngestResult(
            TelemetryIngestStatus status,
            std::string message = {}
        )
            : status_(status)
            , message_(std::move(message))
        {
        }

        [[nodiscard]] TelemetryIngestStatus status() const noexcept
        {
            return status_;
        }

        [[nodiscard]] const std::string& message() const noexcept
        {
            return message_;
        }

        [[nodiscard]] bool accepted() const noexcept
        {
            return status_ == TelemetryIngestStatus::Accepted
                || status_ == TelemetryIngestStatus::AcceptedNoChange;
        }

        [[nodiscard]] bool stored() const noexcept
        {
            return status_ == TelemetryIngestStatus::Accepted;
        }

        [[nodiscard]] bool no_change() const noexcept
        {
            return status_ == TelemetryIngestStatus::AcceptedNoChange;
        }

        [[nodiscard]] bool rejected() const noexcept
        {
            return !accepted();
        }

    private:
        TelemetryIngestStatus status_;
        std::string message_;
    };
}