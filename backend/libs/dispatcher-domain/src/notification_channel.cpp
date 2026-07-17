#include <dispatcher/domain/notification_channel.hpp>

namespace dispatcher::domain
{
    const char* to_string(NotificationChannel channel) noexcept
    {
        switch (channel)
        {
        case NotificationChannel::Unknown:
            return "unknown";

        case NotificationChannel::Console:
            return "console";

        case NotificationChannel::Email:
            return "email";

        case NotificationChannel::Sms:
            return "sms";

        case NotificationChannel::Webhook:
            return "webhook";

        case NotificationChannel::AuditLog:
            return "audit_log";
        }

        return "unknown";
    }

    bool is_known_channel(NotificationChannel channel) noexcept
    {
        return channel != NotificationChannel::Unknown;
    }

    bool requires_address(NotificationChannel channel) noexcept
    {
        switch (channel)
        {
        case NotificationChannel::Email:
        case NotificationChannel::Sms:
        case NotificationChannel::Webhook:
            return true;

        case NotificationChannel::Unknown:
        case NotificationChannel::Console:
        case NotificationChannel::AuditLog:
            return false;
        }

        return false;
    }

    bool is_external_channel(NotificationChannel channel) noexcept
    {
        switch (channel)
        {
        case NotificationChannel::Email:
        case NotificationChannel::Sms:
        case NotificationChannel::Webhook:
            return true;

        case NotificationChannel::Unknown:
        case NotificationChannel::Console:
        case NotificationChannel::AuditLog:
            return false;
        }

        return false;
    }

    bool is_internal_channel(NotificationChannel channel) noexcept
    {
        switch (channel)
        {
        case NotificationChannel::Console:
        case NotificationChannel::AuditLog:
            return true;

        case NotificationChannel::Unknown:
        case NotificationChannel::Email:
        case NotificationChannel::Sms:
        case NotificationChannel::Webhook:
            return false;
        }

        return false;
    }
}