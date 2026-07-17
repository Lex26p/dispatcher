#include <dispatcher/domain/notification_dispatch_status.hpp>

namespace dispatcher::domain
{
    const char* to_string(NotificationDispatchStatus status) noexcept
    {
        switch (status)
        {
        case NotificationDispatchStatus::Dispatched:
            return "dispatched";

        case NotificationDispatchStatus::NoRoutesConfigured:
            return "no_routes_configured";

        case NotificationDispatchStatus::NoMatchingRoutes:
            return "no_matching_routes";

        case NotificationDispatchStatus::InvalidMessage:
            return "invalid_message";
        }

        return "invalid_message";
    }

    bool is_success(NotificationDispatchStatus status) noexcept
    {
        return status == NotificationDispatchStatus::Dispatched;
    }

    bool is_failure(NotificationDispatchStatus status) noexcept
    {
        return !is_success(status);
    }
}