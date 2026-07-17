#include <dispatcher/alarm/alarm_definition.hpp>

#include <utility>

namespace dispatcher::alarm
{
    AlarmDefinition::AlarmDefinition(
        dispatcher::domain::AlarmId alarm_id,
        dispatcher::domain::TagId tag_id,
        std::string name,
        std::string description,
        AlarmSeverity severity,
        bool enabled,
        std::uint64_t config_version
    )
        : alarm_id_(std::move(alarm_id))
        , tag_id_(std::move(tag_id))
        , name_(std::move(name))
        , description_(std::move(description))
        , severity_(severity)
        , enabled_(enabled)
        , config_version_(config_version)
    {
    }

    const dispatcher::domain::AlarmId& AlarmDefinition::alarm_id() const noexcept
    {
        return alarm_id_;
    }

    const dispatcher::domain::TagId& AlarmDefinition::tag_id() const noexcept
    {
        return tag_id_;
    }

    const std::string& AlarmDefinition::name() const noexcept
    {
        return name_;
    }

    const std::string& AlarmDefinition::description() const noexcept
    {
        return description_;
    }

    AlarmSeverity AlarmDefinition::severity() const noexcept
    {
        return severity_;
    }

    bool AlarmDefinition::enabled() const noexcept
    {
        return enabled_;
    }

    std::uint64_t AlarmDefinition::config_version() const noexcept
    {
        return config_version_;
    }
}