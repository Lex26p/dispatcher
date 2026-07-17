#pragma once

#include <dispatcher/alarm/alarm_acknowledgement_result.hpp>
#include <dispatcher/alarm/alarm_state.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace dispatcher::alarm
{
    class AlarmAcknowledgementRecord
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

        AlarmAcknowledgementRecord(
            dispatcher::domain::AlarmId alarm_id,
            std::string operator_id,
            std::string comment,
            AlarmAcknowledgementStatus status,
            AlarmState previous_state,
            AlarmState new_state,
            TimePoint timestamp,
            std::optional<std::uint64_t> event_sequence
        );

        [[nodiscard]] const dispatcher::domain::AlarmId& alarm_id()
            const noexcept;

        [[nodiscard]] const std::string& operator_id() const noexcept;

        [[nodiscard]] const std::string& comment() const noexcept;

        [[nodiscard]] AlarmAcknowledgementStatus status() const noexcept;

        [[nodiscard]] bool acknowledged() const noexcept;

        [[nodiscard]] bool skipped() const noexcept;

        [[nodiscard]] AlarmState previous_state() const noexcept;

        [[nodiscard]] AlarmState new_state() const noexcept;

        [[nodiscard]] TimePoint timestamp() const noexcept;

        [[nodiscard]] const std::optional<std::uint64_t>& event_sequence()
            const noexcept;

    private:
        dispatcher::domain::AlarmId alarm_id_;
        std::string operator_id_;
        std::string comment_;
        AlarmAcknowledgementStatus status_;
        AlarmState previous_state_;
        AlarmState new_state_;
        TimePoint timestamp_;
        std::optional<std::uint64_t> event_sequence_;
    };
}