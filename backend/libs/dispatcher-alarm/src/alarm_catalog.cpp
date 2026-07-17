#include <dispatcher/alarm/alarm_catalog.hpp>

#include <utility>

namespace dispatcher::alarm
{
    AlarmCatalog::AlarmCatalog(
        std::vector<AlarmDefinition> definitions
    )
        : definitions_(std::move(definitions))
    {
    }

    const std::vector<AlarmDefinition>& AlarmCatalog::definitions() const noexcept
    {
        return definitions_;
    }

    std::optional<AlarmDefinition> AlarmCatalog::find_by_alarm_id(
        const dispatcher::domain::AlarmId& alarm_id
    ) const
    {
        for (const auto& definition : definitions_)
        {
            if (definition.alarm_id() == alarm_id)
            {
                return definition;
            }
        }

        return std::nullopt;
    }

    std::vector<AlarmDefinition> AlarmCatalog::find_by_tag_id(
        const dispatcher::domain::TagId& tag_id
    ) const
    {
        std::vector<AlarmDefinition> result;

        for (const auto& definition : definitions_)
        {
            if (definition.tag_id() == tag_id)
            {
                result.push_back(definition);
            }
        }

        return result;
    }

    std::size_t AlarmCatalog::size() const noexcept
    {
        return definitions_.size();
    }

    bool AlarmCatalog::empty() const noexcept
    {
        return definitions_.empty();
    }
}