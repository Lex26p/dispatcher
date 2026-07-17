#include <dispatcher/alarm/alarm_state_store.hpp>

#include <unordered_set>
#include <utility>

namespace dispatcher::alarm
{
    bool AlarmStateStore::has_state(
        const dispatcher::domain::AlarmId& alarm_id
    ) const
    {
        return states_by_alarm_id_.contains(alarm_id.value());
    }

    AlarmState AlarmStateStore::state_of(
        const dispatcher::domain::AlarmId& alarm_id
    ) const
    {
        const auto iterator = states_by_alarm_id_.find(alarm_id.value());

        if (iterator == states_by_alarm_id_.end())
        {
            return AlarmState::Normal;
        }

        return iterator->second;
    }

    void AlarmStateStore::set_state(
        dispatcher::domain::AlarmId alarm_id,
        AlarmState state
    )
    {
        states_by_alarm_id_[alarm_id.value()] = state;
    }

    void AlarmStateStore::apply_transition(
        const dispatcher::domain::AlarmId& alarm_id,
        const AlarmStateTransitionResult& transition_result
    )
    {
        states_by_alarm_id_[alarm_id.value()] = transition_result.new_state();
    }

    void AlarmStateStore::erase_state(
        const dispatcher::domain::AlarmId& alarm_id
    )
    {
        states_by_alarm_id_.erase(alarm_id.value());
    }

    void AlarmStateStore::retain_only(
        const std::vector<dispatcher::domain::AlarmId>& alarm_ids
    )
    {
        std::unordered_set<std::string> allowed_alarm_ids;

        for (const auto& alarm_id : alarm_ids)
        {
            allowed_alarm_ids.insert(alarm_id.value());
        }

        for (auto iterator = states_by_alarm_id_.begin();
            iterator != states_by_alarm_id_.end();)
        {
            if (!allowed_alarm_ids.contains(iterator->first))
            {
                iterator = states_by_alarm_id_.erase(iterator);
            }
            else
            {
                ++iterator;
            }
        }
    }

    std::size_t AlarmStateStore::size() const noexcept
    {
        return states_by_alarm_id_.size();
    }

    bool AlarmStateStore::empty() const noexcept
    {
        return states_by_alarm_id_.empty();
    }

    void AlarmStateStore::clear() noexcept
    {
        states_by_alarm_id_.clear();
    }
}