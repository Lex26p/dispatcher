#pragma once

#include <dispatcher/alarm/alarm_runtime_event.hpp>
#include <dispatcher/alarm/alarm_state.hpp>

#include <optional>
#include <string_view>

namespace dispatcher::alarm
{
    enum class AlarmAcknowledgementStatus
    {
        Acknowledged,
        UnknownAlarm,
        NotActive,
        AlreadyAcknowledged,
        InvalidCommand
    };

    constexpr std::string_view to_string(AlarmAcknowledgementStatus status)
    {
        switch (status)
        {
        case AlarmAcknowledgementStatus::Acknowledged:
            return "acknowledged";
        case AlarmAcknowledgementStatus::UnknownAlarm:
            return "unknown_alarm";
        case AlarmAcknowledgementStatus::NotActive:
            return "not_active";
        case AlarmAcknowledgementStatus::AlreadyAcknowledged:
            return "already_acknowledged";
        case AlarmAcknowledgementStatus::InvalidCommand:
            return "invalid_command";
        }

        return "unknown";
    }

    class AlarmAcknowledgementResult
    {
    public:
        AlarmAcknowledgementResult(
            AlarmAcknowledgementStatus status,
            AlarmState previous_state,
            AlarmState new_state,
            std::optional<AlarmRuntimeEvent> event
        );

        [[nodiscard]] AlarmAcknowledgementStatus status() const noexcept;

        [[nodiscard]] bool acknowledged() const noexcept;

        [[nodiscard]] bool skipped() const noexcept;

        [[nodiscard]] bool unknown_alarm() const noexcept;

        [[nodiscard]] bool not_active() const noexcept;

        [[nodiscard]] bool already_acknowledged() const noexcept;

        [[nodiscard]] bool invalid_command() const noexcept;

        [[nodiscard]] AlarmState previous_state() const noexcept;

        [[nodiscard]] AlarmState new_state() const noexcept;

        [[nodiscard]] const std::optional<AlarmRuntimeEvent>& event()
            const noexcept;

    private:
        AlarmAcknowledgementStatus status_;
        AlarmState previous_state_;
        AlarmState new_state_;
        std::optional<AlarmRuntimeEvent> event_;
    };
}