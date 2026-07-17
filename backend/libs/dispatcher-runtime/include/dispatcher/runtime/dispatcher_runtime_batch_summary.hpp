#pragma once

#include <dispatcher/runtime/dispatcher_runtime_process_summary.hpp>

#include <cstdint>

namespace dispatcher::runtime
{
    struct DispatcherRuntimeBatchSummary
    {
        std::uint64_t total_count{ 0 };

        std::uint64_t telemetry_accepted_count{ 0 };
        std::uint64_t telemetry_stored_count{ 0 };
        std::uint64_t telemetry_no_change_count{ 0 };
        std::uint64_t telemetry_rejected_count{ 0 };

        std::uint64_t history_written_count{ 0 };
        std::uint64_t history_skipped_count{ 0 };

        std::uint64_t configured_alarm_count{ 0 };
        std::uint64_t missing_condition_count{ 0 };

        std::uint64_t alarm_total_count{ 0 };
        std::uint64_t alarm_evaluated_count{ 0 };
        std::uint64_t alarm_skipped_count{ 0 };

        std::uint64_t alarm_activated_count{ 0 };
        std::uint64_t alarm_acknowledged_count{ 0 };
        std::uint64_t alarm_cleared_count{ 0 };
        std::uint64_t alarm_stored_event_count{ 0 };

        void record(const DispatcherRuntimeProcessSummary& summary) noexcept
        {
            ++total_count;

            if (summary.telemetry_accepted())
            {
                ++telemetry_accepted_count;
            }

            if (summary.telemetry_stored())
            {
                ++telemetry_stored_count;
            }

            if (summary.telemetry_no_change())
            {
                ++telemetry_no_change_count;
            }

            if (summary.telemetry_rejected())
            {
                ++telemetry_rejected_count;
            }

            if (summary.history_written())
            {
                ++history_written_count;
            }
            else
            {
                ++history_skipped_count;
            }

            configured_alarm_count += summary.configured_alarm_count;
            missing_condition_count += summary.missing_condition_count;

            alarm_total_count += summary.alarm_total_count;
            alarm_evaluated_count += summary.alarm_evaluated_count;
            alarm_skipped_count += summary.alarm_skipped_count;

            alarm_activated_count += summary.alarm_activated_count;
            alarm_acknowledged_count += summary.alarm_acknowledged_count;
            alarm_cleared_count += summary.alarm_cleared_count;
            alarm_stored_event_count += summary.alarm_stored_event_count;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return total_count == 0;
        }

        [[nodiscard]] bool all_telemetry_accepted() const noexcept
        {
            return total_count > 0 && telemetry_rejected_count == 0;
        }

        [[nodiscard]] bool has_telemetry_rejections() const noexcept
        {
            return telemetry_rejected_count > 0;
        }

        [[nodiscard]] bool has_history_writes() const noexcept
        {
            return history_written_count > 0;
        }

        [[nodiscard]] bool has_alarm_evaluations() const noexcept
        {
            return alarm_evaluated_count > 0;
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

        [[nodiscard]] bool has_missing_conditions() const noexcept
        {
            return missing_condition_count > 0;
        }

        [[nodiscard]] bool successful() const noexcept
        {
            return total_count > 0
                && all_telemetry_accepted()
                && !has_missing_conditions();
        }
    };
}