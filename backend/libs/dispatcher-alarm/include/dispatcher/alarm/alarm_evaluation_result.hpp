#pragma once

#include <dispatcher/alarm/alarm_runtime_event.hpp>
#include <dispatcher/alarm/alarm_state.hpp>
#include <dispatcher/alarm/alarm_transition_type.hpp>

#include <optional>
#include <string_view>

namespace dispatcher::alarm
{
    enum class AlarmEvaluationStatus
    {
        Evaluated,
        DisabledAlarm,
        TagMismatch,
        UnsupportedValueType
    };

    constexpr std::string_view to_string(AlarmEvaluationStatus status)
    {
        switch (status)
        {
        case AlarmEvaluationStatus::Evaluated:
            return "evaluated";
        case AlarmEvaluationStatus::DisabledAlarm:
            return "disabled_alarm";
        case AlarmEvaluationStatus::TagMismatch:
            return "tag_mismatch";
        case AlarmEvaluationStatus::UnsupportedValueType:
            return "unsupported_value_type";
        }

        return "unknown";
    }

    class AlarmEvaluationResult
    {
    public:
        AlarmEvaluationResult(
            AlarmEvaluationStatus status,
            AlarmTransitionType transition_type,
            AlarmState previous_state,
            AlarmState new_state,
            bool condition_active,
            std::optional<AlarmRuntimeEvent> event
        );

        [[nodiscard]] AlarmEvaluationStatus status() const noexcept;

        [[nodiscard]] bool evaluated() const noexcept;
        [[nodiscard]] bool skipped() const noexcept;

        [[nodiscard]] AlarmTransitionType transition_type() const noexcept;

        [[nodiscard]] AlarmState previous_state() const noexcept;
        [[nodiscard]] AlarmState new_state() const noexcept;

        [[nodiscard]] bool condition_active() const noexcept;

        [[nodiscard]] bool transitioned() const noexcept;
        [[nodiscard]] bool activated() const noexcept;
        [[nodiscard]] bool acknowledged() const noexcept;
        [[nodiscard]] bool cleared() const noexcept;

        [[nodiscard]] const std::optional<AlarmRuntimeEvent>& event() const noexcept;

    private:
        AlarmEvaluationStatus status_;
        AlarmTransitionType transition_type_;
        AlarmState previous_state_;
        AlarmState new_state_;
        bool condition_active_;
        std::optional<AlarmRuntimeEvent> event_;
    };
}