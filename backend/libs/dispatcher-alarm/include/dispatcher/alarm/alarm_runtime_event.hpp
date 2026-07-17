#pragma once

#include <dispatcher/alarm/alarm_severity.hpp>
#include <dispatcher/alarm/alarm_state.hpp>
#include <dispatcher/alarm/alarm_transition_type.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <cstdint>

namespace dispatcher::alarm
{
    class AlarmRuntimeEvent
    {
    public:
        using Clock = dispatcher::telemetry::TelemetryValue::Clock;
        using TimePoint = dispatcher::telemetry::TelemetryValue::TimePoint;

        AlarmRuntimeEvent(
            dispatcher::domain::AlarmId alarm_id,
            dispatcher::domain::TagId tag_id,
            AlarmSeverity severity,
            AlarmTransitionType transition_type,
            AlarmState previous_state,
            AlarmState new_state,
            TimePoint source_timestamp,
            TimePoint event_timestamp,
            std::uint64_t sequence
        );

        [[nodiscard]] const dispatcher::domain::AlarmId& alarm_id() const noexcept;
        [[nodiscard]] const dispatcher::domain::TagId& tag_id() const noexcept;

        [[nodiscard]] AlarmSeverity severity() const noexcept;
        [[nodiscard]] AlarmTransitionType transition_type() const noexcept;

        [[nodiscard]] AlarmState previous_state() const noexcept;
        [[nodiscard]] AlarmState new_state() const noexcept;

        [[nodiscard]] TimePoint source_timestamp() const noexcept;
        [[nodiscard]] TimePoint event_timestamp() const noexcept;

        [[nodiscard]] std::uint64_t sequence() const noexcept;

    private:
        dispatcher::domain::AlarmId alarm_id_;
        dispatcher::domain::TagId tag_id_;

        AlarmSeverity severity_;
        AlarmTransitionType transition_type_;

        AlarmState previous_state_;
        AlarmState new_state_;

        TimePoint source_timestamp_;
        TimePoint event_timestamp_;

        std::uint64_t sequence_;
    };
}