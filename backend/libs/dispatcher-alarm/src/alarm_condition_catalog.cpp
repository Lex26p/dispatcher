#include <dispatcher/alarm/alarm_condition_catalog.hpp>

#include <utility>

namespace dispatcher::alarm
{
    AlarmConditionCatalog::AlarmConditionCatalog(
        std::vector<AlarmConditionDefinition> definitions
    )
        : definitions_(std::move(definitions))
    {
    }

    const std::vector<AlarmConditionDefinition>&
        AlarmConditionCatalog::definitions() const noexcept
    {
        return definitions_;
    }

    std::optional<AlarmConditionDefinition> AlarmConditionCatalog::find_by_alarm_id(
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

    std::size_t AlarmConditionCatalog::size() const noexcept
    {
        return definitions_.size();
    }

    bool AlarmConditionCatalog::empty() const noexcept
    {
        return definitions_.empty();
    }
}