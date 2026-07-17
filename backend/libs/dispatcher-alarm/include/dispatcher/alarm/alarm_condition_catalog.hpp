#pragma once

#include <dispatcher/alarm/alarm_condition_definition.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace dispatcher::alarm
{
    class AlarmConditionCatalog
    {
    public:
        AlarmConditionCatalog() = default;

        explicit AlarmConditionCatalog(
            std::vector<AlarmConditionDefinition> definitions
        );

        [[nodiscard]] const std::vector<AlarmConditionDefinition>& definitions()
            const noexcept;

        [[nodiscard]] std::optional<AlarmConditionDefinition> find_by_alarm_id(
            const dispatcher::domain::AlarmId& alarm_id
        ) const;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

    private:
        std::vector<AlarmConditionDefinition> definitions_;
    };
}