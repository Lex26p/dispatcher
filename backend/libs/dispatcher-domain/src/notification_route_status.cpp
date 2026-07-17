#include <dispatcher/domain/notification_route_status.hpp>

namespace dispatcher::domain
{
    const char* to_string(NotificationRouteStatus status) noexcept
    {
        switch (status)
        {
        case NotificationRouteStatus::Accepted:
            return "accepted";

        case NotificationRouteStatus::RejectedInvalidRoute:
            return "rejected_invalid_route";

        case NotificationRouteStatus::RejectedDisabledRoute:
            return "rejected_disabled_route";

        case NotificationRouteStatus::RejectedPriorityTooLow:
            return "rejected_priority_too_low";

        case NotificationRouteStatus::RejectedSourceMismatch:
            return "rejected_source_mismatch";

        case NotificationRouteStatus::RejectedTargetInvalid:
            return "rejected_target_invalid";
        }

        return "rejected_invalid_route";
    }

    bool is_accepted(NotificationRouteStatus status) noexcept
    {
        return status == NotificationRouteStatus::Accepted;
    }

    bool is_rejected(NotificationRouteStatus status) noexcept
    {
        return !is_accepted(status);
    }
}