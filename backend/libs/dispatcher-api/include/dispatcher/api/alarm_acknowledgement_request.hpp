#pragma once

#include <dispatcher/alarm/alarm_acknowledgement_command.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <string>

namespace dispatcher::api
{
    struct AlarmAcknowledgementRequest
    {
        dispatcher::domain::AlarmId alarm_id;
        std::string operator_id;
        std::string comment;

        [[nodiscard]] bool has_alarm_id() const noexcept;

        [[nodiscard]] bool has_operator_id() const noexcept;

        [[nodiscard]] bool has_comment() const noexcept;

        [[nodiscard]] dispatcher::alarm::AlarmAcknowledgementCommand
            to_command() const;
    };
}