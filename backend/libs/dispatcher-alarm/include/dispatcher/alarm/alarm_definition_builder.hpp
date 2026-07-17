#pragma once

#include <dispatcher/alarm/alarm_definition.hpp>

#include <cstdint>
#include <string>

namespace dispatcher::alarm
{
    class AlarmDefinitionBuilder
    {
    public:
        AlarmDefinitionBuilder& alarm_id(dispatcher::domain::AlarmId value);
        AlarmDefinitionBuilder& tag_id(dispatcher::domain::TagId value);

        AlarmDefinitionBuilder& name(std::string value);
        AlarmDefinitionBuilder& description(std::string value);

        AlarmDefinitionBuilder& severity(AlarmSeverity value);

        AlarmDefinitionBuilder& enabled(bool value);
        AlarmDefinitionBuilder& config_version(std::uint64_t value);

        [[nodiscard]] AlarmDefinition build() const;

    private:
        dispatcher::domain::AlarmId alarm_id_;
        dispatcher::domain::TagId tag_id_;

        std::string name_;
        std::string description_;

        AlarmSeverity severity_{ AlarmSeverity::Warning };
        bool enabled_{ true };
        std::uint64_t config_version_{ 1 };
    };
}