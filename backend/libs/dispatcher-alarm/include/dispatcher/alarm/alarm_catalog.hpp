#pragma once

#include <dispatcher/alarm/alarm_definition.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace dispatcher::alarm
{
    class AlarmCatalog
    {
    public:
        AlarmCatalog() = default;

        explicit AlarmCatalog(
            std::vector<AlarmDefinition> definitions
        );

        [[nodiscard]] const std::vector<AlarmDefinition>& definitions()
            const noexcept;

        [[nodiscard]] std::optional<AlarmDefinition> find_by_alarm_id(
            const dispatcher::domain::AlarmId& alarm_id
        ) const;

        [[nodiscard]] std::vector<AlarmDefinition> find_by_tag_id(
            const dispatcher::domain::TagId& tag_id
        ) const;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

    private:
        std::vector<AlarmDefinition> definitions_;
    };
}