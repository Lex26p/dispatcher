#pragma once

#include <dispatcher/alarm/alarm_evaluation_result.hpp>
#include <dispatcher/alarm/alarm_runtime_event.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <cstddef>
#include <vector>

namespace dispatcher::alarm
{
    class AlarmEventStore
    {
    public:
        void append(AlarmRuntimeEvent event);

        [[nodiscard]] bool append_if_present(
            const std::optional<AlarmRuntimeEvent>& event
        );

        [[nodiscard]] bool append_from_evaluation_result(
            const AlarmEvaluationResult& evaluation_result
        );

        [[nodiscard]] std::vector<AlarmRuntimeEvent> find_by_alarm_id(
            const dispatcher::domain::AlarmId& alarm_id
        ) const;

        [[nodiscard]] std::vector<AlarmRuntimeEvent> find_by_tag_id(
            const dispatcher::domain::TagId& tag_id
        ) const;

        [[nodiscard]] const std::vector<AlarmRuntimeEvent>& events() const noexcept;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        void clear() noexcept;

    private:
        std::vector<AlarmRuntimeEvent> events_;
    };
}