#pragma once

#include <dispatcher/alarm/alarm_operator_snapshot.hpp>
#include <dispatcher/alarm/alarm_runtime_snapshot.hpp>
#include <dispatcher/core/telemetry_runtime_snapshot.hpp>
#include <dispatcher/history/history_runtime_snapshot.hpp>

namespace dispatcher::runtime
{
    struct DispatcherRuntimeSnapshot
    {
        dispatcher::core::TelemetryRuntimeSnapshot telemetry;
        dispatcher::history::HistoryRuntimeSnapshot history;
        dispatcher::alarm::AlarmRuntimeSnapshot alarm;
        dispatcher::alarm::AlarmOperatorSnapshot alarm_operator;

        [[nodiscard]] bool has_current_state() const noexcept
        {
            return telemetry.current_state_size > 0;
        }

        [[nodiscard]] bool has_history_samples() const noexcept
        {
            return history.store_size > 0;
        }

        [[nodiscard]] bool has_alarm_events() const noexcept
        {
            return alarm.event_store_size > 0;
        }

        [[nodiscard]] bool has_acknowledgement_audit() const noexcept
        {
            return alarm.acknowledgement_store_size > 0;
        }

        [[nodiscard]] bool requires_operator_attention() const noexcept
        {
            return alarm_operator.requires_operator_attention();
        }
    };
}