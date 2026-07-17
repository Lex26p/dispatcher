#include <dispatcher/alarm/alarm_suppression_runtime_snapshot.hpp>

namespace dispatcher::alarm
{
    bool AlarmSuppressionRuntimeSnapshot::empty() const noexcept
    {
        return store_size == 0;
    }

    bool AlarmSuppressionRuntimeSnapshot::has_records() const noexcept
    {
        return store_size > 0;
    }

    bool AlarmSuppressionRuntimeSnapshot::has_active_records() const noexcept
    {
        return active_count > 0;
    }

    bool AlarmSuppressionRuntimeSnapshot::has_expired_records() const noexcept
    {
        return expired_count > 0;
    }

    bool AlarmSuppressionRuntimeSnapshot::has_operator_controlled_records()
        const noexcept
    {
        return operator_controlled_count > 0;
    }

    bool AlarmSuppressionRuntimeSnapshot::has_system_controlled_records()
        const noexcept
    {
        return system_controlled_count > 0;
    }

    std::uint64_t AlarmSuppressionRuntimeSnapshot::total_command_count()
        const noexcept
    {
        return applied_count + cleared_count + rejected_count;
    }
}