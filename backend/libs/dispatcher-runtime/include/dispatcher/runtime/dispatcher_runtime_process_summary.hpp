#pragma once

#include <dispatcher/core/telemetry_ingest_status.hpp>
#include <dispatcher/history/history_write_result.hpp>

#include <cstdint>

namespace dispatcher::runtime
{
    struct DispatcherRuntimeProcessSummary
    {
        dispatcher::core::TelemetryIngestStatus telemetry_status{
            dispatcher::core::TelemetryIngestStatus::UnknownTag
        };

        dispatcher::history::HistoryWriteStatus history_status{
            dispatcher::history::HistoryWriteStatus::SkippedNotStored
        };

        std::uint64_t configured_alarm_count{ 0 };
        std::uint64_t missing_condition_count{ 0 };

        std::uint64_t alarm_total_count{ 0 };
        std::uint64_t alarm_evaluated_count{ 0 };
        std::uint64_t alarm_skipped_count{ 0 };

        std::uint64_t alarm_activated_count{ 0 };
        std::uint64_t alarm_acknowledged_count{ 0 };
        std::uint64_t alarm_cleared_count{ 0 };
        std::uint64_t alarm_stored_event_count{ 0 };

        [[nodiscard]] bool telemetry_accepted() const noexcept
        {
            return telemetry_status
                == dispatcher::core::TelemetryIngestStatus::Accepted
                || telemetry_status
                == dispatcher::core::TelemetryIngestStatus::AcceptedNoChange;
        }

        [[nodiscard]] bool telemetry_stored() const noexcept
        {
            return telemetry_status
                == dispatcher::core::TelemetryIngestStatus::Accepted;
        }

        [[nodiscard]] bool telemetry_no_change() const noexcept
        {
            return telemetry_status
                == dispatcher::core::TelemetryIngestStatus::AcceptedNoChange;
        }

        [[nodiscard]] bool telemetry_rejected() const noexcept
        {
            return !telemetry_accepted();
        }

        [[nodiscard]] bool history_written() const noexcept
        {
            return history_status
                == dispatcher::history::HistoryWriteStatus::Written;
        }

        [[nodiscard]] bool history_skipped() const noexcept
        {
            return !history_written();
        }

        [[nodiscard]] bool has_configured_alarms() const noexcept
        {
            return configured_alarm_count > 0;
        }

        [[nodiscard]] bool has_missing_conditions() const noexcept
        {
            return missing_condition_count > 0;
        }

        [[nodiscard]] bool alarm_evaluated() const noexcept
        {
            return alarm_evaluated_count > 0;
        }

        [[nodiscard]] bool alarm_skipped() const noexcept
        {
            return alarm_skipped_count > 0;
        }

        [[nodiscard]] bool has_alarm_events() const noexcept
        {
            return alarm_stored_event_count > 0;
        }

        [[nodiscard]] bool has_alarm_transitions() const noexcept
        {
            return alarm_activated_count > 0
                || alarm_acknowledged_count > 0
                || alarm_cleared_count > 0;
        }

        [[nodiscard]] bool successful() const noexcept
        {
            return telemetry_accepted()
                && !has_missing_conditions();
        }
    };
}