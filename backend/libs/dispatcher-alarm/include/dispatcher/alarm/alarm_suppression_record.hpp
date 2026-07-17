#pragma once

#include <dispatcher/alarm/alarm_suppression_command.hpp>
#include <dispatcher/alarm/alarm_suppression_mode.hpp>
#include <dispatcher/alarm/alarm_suppression_reason.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <chrono>
#include <optional>
#include <string>

namespace dispatcher::alarm
{
    class AlarmSuppressionRecord
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

        AlarmSuppressionRecord(
            dispatcher::domain::AlarmId alarm_id,
            std::string operator_id,
            AlarmSuppressionMode mode,
            AlarmSuppressionReason reason,
            TimePoint applied_at,
            std::string comment = {},
            std::optional<TimePoint> expires_at = std::nullopt
        );

        [[nodiscard]] static AlarmSuppressionRecord from_command(
            const AlarmSuppressionCommand& command,
            TimePoint applied_at = Clock::now()
        );

        [[nodiscard]] const dispatcher::domain::AlarmId& alarm_id()
            const noexcept;

        [[nodiscard]] const std::string& operator_id() const noexcept;

        [[nodiscard]] AlarmSuppressionMode mode() const noexcept;

        [[nodiscard]] AlarmSuppressionReason reason() const noexcept;

        [[nodiscard]] TimePoint applied_at() const noexcept;

        [[nodiscard]] const std::string& comment() const noexcept;

        [[nodiscard]] const std::optional<TimePoint>& expires_at()
            const noexcept;

        [[nodiscard]] bool has_comment() const noexcept;

        [[nodiscard]] bool has_expiration() const noexcept;

        [[nodiscard]] bool expired_at(TimePoint now) const noexcept;

        [[nodiscard]] bool active_at(TimePoint now) const noexcept;

    private:
        dispatcher::domain::AlarmId alarm_id_;
        std::string operator_id_;
        AlarmSuppressionMode mode_{ AlarmSuppressionMode::Shelved };
        AlarmSuppressionReason reason_{ AlarmSuppressionReason::Unknown };
        TimePoint applied_at_{};
        std::string comment_;
        std::optional<TimePoint> expires_at_;
    };
}