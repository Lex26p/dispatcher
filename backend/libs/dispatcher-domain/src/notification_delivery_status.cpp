#include <dispatcher/domain/notification_delivery_status.hpp>

namespace dispatcher::domain
{
    const char* to_string(NotificationDeliveryStatus status) noexcept
    {
        switch (status)
        {
        case NotificationDeliveryStatus::Pending:
            return "pending";

        case NotificationDeliveryStatus::Delivered:
            return "delivered";

        case NotificationDeliveryStatus::Failed:
            return "failed";

        case NotificationDeliveryStatus::Skipped:
            return "skipped";

        case NotificationDeliveryStatus::RetryScheduled:
            return "retry_scheduled";
        }

        return "failed";
    }

    bool is_terminal(NotificationDeliveryStatus status) noexcept
    {
        switch (status)
        {
        case NotificationDeliveryStatus::Delivered:
        case NotificationDeliveryStatus::Failed:
        case NotificationDeliveryStatus::Skipped:
            return true;

        case NotificationDeliveryStatus::Pending:
        case NotificationDeliveryStatus::RetryScheduled:
            return false;
        }

        return true;
    }

    bool is_success(NotificationDeliveryStatus status) noexcept
    {
        return status == NotificationDeliveryStatus::Delivered
            || status == NotificationDeliveryStatus::Skipped;
    }

    bool is_failure(NotificationDeliveryStatus status) noexcept
    {
        return status == NotificationDeliveryStatus::Failed;
    }

    bool is_retryable(NotificationDeliveryStatus status) noexcept
    {
        return status == NotificationDeliveryStatus::Pending
            || status == NotificationDeliveryStatus::RetryScheduled
            || status == NotificationDeliveryStatus::Failed;
    }
}