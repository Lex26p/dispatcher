#include <dispatcher/alarm/alarm_definition_builder.hpp>

#include <utility>

namespace dispatcher::alarm
{
    AlarmDefinitionBuilder& AlarmDefinitionBuilder::alarm_id(
        dispatcher::domain::AlarmId value
    )
    {
        alarm_id_ = std::move(value);
        return *this;
    }

    AlarmDefinitionBuilder& AlarmDefinitionBuilder::tag_id(
        dispatcher::domain::TagId value
    )
    {
        tag_id_ = std::move(value);
        return *this;
    }

    AlarmDefinitionBuilder& AlarmDefinitionBuilder::name(std::string value)
    {
        name_ = std::move(value);
        return *this;
    }

    AlarmDefinitionBuilder& AlarmDefinitionBuilder::description(std::string value)
    {
        description_ = std::move(value);
        return *this;
    }

    AlarmDefinitionBuilder& AlarmDefinitionBuilder::severity(AlarmSeverity value)
    {
        severity_ = value;
        return *this;
    }

    AlarmDefinitionBuilder& AlarmDefinitionBuilder::enabled(bool value)
    {
        enabled_ = value;
        return *this;
    }

    AlarmDefinitionBuilder& AlarmDefinitionBuilder::config_version(
        std::uint64_t value
    )
    {
        config_version_ = value;
        return *this;
    }

    AlarmDefinition AlarmDefinitionBuilder::build() const
    {
        return AlarmDefinition(
            alarm_id_,
            tag_id_,
            name_,
            description_,
            severity_,
            enabled_,
            config_version_
        );
    }
}