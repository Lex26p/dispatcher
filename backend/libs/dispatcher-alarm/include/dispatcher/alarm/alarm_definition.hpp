#pragma once

#include <dispatcher/alarm/alarm_severity.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <cstdint>
#include <string>

namespace dispatcher::alarm
{
    class AlarmDefinition
    {
    public:
        AlarmDefinition(
            dispatcher::domain::AlarmId alarm_id,
            dispatcher::domain::TagId tag_id,
            std::string name,
            std::string description,
            AlarmSeverity severity,
            bool enabled,
            std::uint64_t config_version
        );

        [[nodiscard]] const dispatcher::domain::AlarmId& alarm_id() const noexcept;
        [[nodiscard]] const dispatcher::domain::TagId& tag_id() const noexcept;

        [[nodiscard]] const std::string& name() const noexcept;
        [[nodiscard]] const std::string& description() const noexcept;

        [[nodiscard]] AlarmSeverity severity() const noexcept;

        [[nodiscard]] bool enabled() const noexcept;
        [[nodiscard]] std::uint64_t config_version() const noexcept;

    private:
        dispatcher::domain::AlarmId alarm_id_;
        dispatcher::domain::TagId tag_id_;

        std::string name_;
        std::string description_;

        AlarmSeverity severity_;
        bool enabled_;
        std::uint64_t config_version_;
    };
}