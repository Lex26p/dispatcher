#pragma once

#include <dispatcher/alarm/alarm_definition.hpp>
#include <dispatcher/alarm/alarm_runtime_event.hpp>
#include <dispatcher/alarm/alarm_state.hpp>
#include <dispatcher/alarm/alarm_transition_type.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <optional>

namespace dispatcher::alarm
{
    class AlarmStateTransitionResult
    {
    public:
        AlarmStateTransitionResult(
            AlarmTransitionType transition_type,
            AlarmState previous_state,
            AlarmState new_state,
            std::optional<AlarmRuntimeEvent> event
        );

        [[nodiscard]] AlarmTransitionType transition_type() const noexcept;

        [[nodiscard]] AlarmState previous_state() const noexcept;
        [[nodiscard]] AlarmState new_state() const noexcept;

        [[nodiscard]] bool transitioned() const noexcept;
        [[nodiscard]] bool activated() const noexcept;
        [[nodiscard]] bool cleared() const noexcept;

        [[nodiscard]] const std::optional<AlarmRuntimeEvent>& event() const noexcept;

    private:
        AlarmTransitionType transition_type_;
        AlarmState previous_state_;
        AlarmState new_state_;
        std::optional<AlarmRuntimeEvent> event_;
    };

    [[nodiscard]] AlarmStateTransitionResult evaluate_alarm_state_transition(
        const AlarmDefinition& alarm_definition,
        AlarmState previous_state,
        bool condition_active,
        const dispatcher::telemetry::TelemetryValue& telemetry_value
    );
}