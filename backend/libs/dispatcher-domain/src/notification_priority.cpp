#include <dispatcher/domain/notification_priority.hpp>

namespace dispatcher::domain
{
    const char* to_string(NotificationPriority priority) noexcept
    {
        switch (priority)
        {
        case NotificationPriority::Low:
            return "low";

        case NotificationPriority::Normal:
            return "normal";

        case NotificationPriority::High:
            return "high";

        case NotificationPriority::Critical:
            return "critical";
        }

        return "normal";
    }

    std::uint32_t delivery_order(NotificationPriority priority) noexcept
    {
        switch (priority)
        {
        case NotificationPriority::Critical:
            return 0;

        case NotificationPriority::High:
            return 1;

        case NotificationPriority::Normal:
            return 2;

        case NotificationPriority::Low:
            return 3;
        }

        return 2;
    }

    bool is_urgent(NotificationPriority priority) noexcept
    {
        return priority == NotificationPriority::High
            || priority == NotificationPriority::Critical;
    }

    bool requires_operator_attention(
        NotificationPriority priority
    ) noexcept
    {
        return priority == NotificationPriority::Critical;
    }
}