#pragma once

#include <dispatcher/alarm/alarm_suppression_record.hpp>
#include <dispatcher/alarm/alarm_suppression_status.hpp>

#include <optional>
#include <string>

namespace dispatcher::alarm
{
    class AlarmSuppressionResult
    {
    public:
        [[nodiscard]] static AlarmSuppressionResult applied(
            AlarmSuppressionRecord record
        );

        [[nodiscard]] static AlarmSuppressionResult cleared(
            AlarmSuppressionRecord record
        );

        [[nodiscard]] static AlarmSuppressionResult failure(
            AlarmSuppressionStatus status,
            std::string message = {}
        );

        [[nodiscard]] bool success() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] bool applied() const noexcept;

        [[nodiscard]] bool cleared() const noexcept;

        [[nodiscard]] AlarmSuppressionStatus status() const noexcept;

        [[nodiscard]] const std::string& message() const noexcept;

        [[nodiscard]] bool has_message() const noexcept;

        [[nodiscard]] bool has_record() const noexcept;

        [[nodiscard]] const AlarmSuppressionRecord& record() const;

    private:
        AlarmSuppressionResult(
            AlarmSuppressionStatus status,
            std::optional<AlarmSuppressionRecord> record,
            std::string message
        );

        AlarmSuppressionStatus status_{ AlarmSuppressionStatus::InvalidCommand };
        std::optional<AlarmSuppressionRecord> record_;
        std::string message_;
    };
}