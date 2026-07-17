#pragma once

#include <cstddef>
#include <cstdint>

namespace dispatcher::alarm
{
    struct AlarmOperatorSnapshot
    {
        std::uint64_t configuration_version{ 0 };
        std::size_t configured_alarm_count{ 0 };

        std::size_t normal_alarm_count{ 0 };
        std::size_t active_alarm_count{ 0 };
        std::size_t acknowledged_alarm_count{ 0 };
        std::size_t unacknowledged_alarm_count{ 0 };

        std::size_t event_store_size{ 0 };
        std::size_t acknowledgement_store_size{ 0 };

        std::uint64_t activated_count{ 0 };
        std::uint64_t acknowledged_count{ 0 };
        std::uint64_t cleared_count{ 0 };

        std::size_t suppression_store_size{ 0 };
        std::size_t suppressed_alarm_count{ 0 };
        std::size_t shelved_alarm_count{ 0 };
        std::size_t inhibited_alarm_count{ 0 };
        std::size_t operator_controlled_suppression_count{ 0 };
        std::size_t system_controlled_suppression_count{ 0 };

        [[nodiscard]] std::size_t active_or_acknowledged_alarm_count()
            const noexcept
        {
            return active_alarm_count + acknowledged_alarm_count;
        }

        [[nodiscard]] bool has_configured_alarms() const noexcept
        {
            return configured_alarm_count > 0;
        }

        [[nodiscard]] bool has_active_alarms() const noexcept
        {
            return active_alarm_count > 0;
        }

        [[nodiscard]] bool has_acknowledged_alarms() const noexcept
        {
            return acknowledged_alarm_count > 0;
        }

        [[nodiscard]] bool has_unacknowledged_alarms() const noexcept
        {
            return unacknowledged_alarm_count > 0;
        }

        [[nodiscard]] bool has_active_or_acknowledged_alarms() const noexcept
        {
            return active_or_acknowledged_alarm_count() > 0;
        }

        [[nodiscard]] bool requires_operator_attention() const noexcept
        {
            return has_unacknowledged_alarms();
        }

        [[nodiscard]] bool has_suppressed_alarms() const noexcept
        {
            return suppressed_alarm_count > 0;
        }

        [[nodiscard]] bool has_shelved_alarms() const noexcept
        {
            return shelved_alarm_count > 0;
        }

        [[nodiscard]] bool has_inhibited_alarms() const noexcept
        {
            return inhibited_alarm_count > 0;
        }

        [[nodiscard]] bool has_any_suppression() const noexcept
        {
            return suppression_store_size > 0;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return configuration_version == 0
                && configured_alarm_count == 0
                && normal_alarm_count == 0
                && active_alarm_count == 0
                && acknowledged_alarm_count == 0
                && unacknowledged_alarm_count == 0
                && event_store_size == 0
                && acknowledgement_store_size == 0
                && activated_count == 0
                && acknowledged_count == 0
                && cleared_count == 0
                && suppression_store_size == 0
                && suppressed_alarm_count == 0
                && shelved_alarm_count == 0
                && inhibited_alarm_count == 0
                && operator_controlled_suppression_count == 0
                && system_controlled_suppression_count == 0;
        }
    };
}