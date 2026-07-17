#pragma once

#include <dispatcher/alarm/alarm_state.hpp>
#include <dispatcher/alarm/alarm_state_transition.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace dispatcher::alarm
{
    class AlarmStateStore
    {
    public:
        [[nodiscard]] bool has_state(
            const dispatcher::domain::AlarmId& alarm_id
        ) const;

        [[nodiscard]] AlarmState state_of(
            const dispatcher::domain::AlarmId& alarm_id
        ) const;

        void set_state(
            dispatcher::domain::AlarmId alarm_id,
            AlarmState state
        );

        void apply_transition(
            const dispatcher::domain::AlarmId& alarm_id,
            const AlarmStateTransitionResult& transition_result
        );

        void erase_state(
            const dispatcher::domain::AlarmId& alarm_id
        );

        void retain_only(
            const std::vector<dispatcher::domain::AlarmId>& alarm_ids
        );

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        void clear() noexcept;

    private:
        std::unordered_map<std::string, AlarmState> states_by_alarm_id_;
    };
}