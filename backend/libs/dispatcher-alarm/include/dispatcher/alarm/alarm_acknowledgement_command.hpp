#pragma once

#include <dispatcher/domain/id_types.hpp>

#include <string>

namespace dispatcher::alarm
{
    class AlarmAcknowledgementCommand
    {
    public:
        AlarmAcknowledgementCommand(
            dispatcher::domain::AlarmId alarm_id,
            std::string operator_id,
            std::string comment
        );

        [[nodiscard]] const dispatcher::domain::AlarmId& alarm_id() const noexcept;

        [[nodiscard]] const std::string& operator_id() const noexcept;

        [[nodiscard]] const std::string& comment() const noexcept;

    private:
        dispatcher::domain::AlarmId alarm_id_;
        std::string operator_id_;
        std::string comment_;
    };
}