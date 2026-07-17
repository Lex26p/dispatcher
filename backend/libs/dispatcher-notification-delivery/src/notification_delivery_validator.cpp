#include <dispatcher/notification/delivery/notification_delivery_validator.hpp>

#include <dispatcher/notification/delivery/notification_delivery_error.hpp>

namespace dispatcher::notification::delivery
{
    void NotificationDeliveryValidator::validate_message(
        const NotificationDeliveryMessage& message
    )
    {
        if (message.message_id.empty())
        {
            throw NotificationDeliveryError(
                "Notification delivery message_id must not be empty."
            );
        }

        if (message.source.empty())
        {
            throw NotificationDeliveryError(
                "Notification delivery source must not be empty."
            );
        }

        if (!is_valid_channel_type(
            message.channel_type
        ))
        {
            throw NotificationDeliveryError(
                "Notification delivery channel_type is invalid."
            );
        }

        if (!is_valid_priority(
            message.priority
        ))
        {
            throw NotificationDeliveryError(
                "Notification delivery priority is invalid."
            );
        }

        if (message.recipient.address.empty())
        {
            throw NotificationDeliveryError(
                "Notification delivery recipient address must not be empty."
            );
        }

        if (message.subject.empty())
        {
            throw NotificationDeliveryError(
                "Notification delivery subject must not be empty."
            );
        }

        if (message.body.empty())
        {
            throw NotificationDeliveryError(
                "Notification delivery body must not be empty."
            );
        }
    }

    void NotificationDeliveryValidator::validate_result(
        const NotificationDeliveryResult& result
    )
    {
        if (!is_valid_status(
            result.status
        ))
        {
            throw NotificationDeliveryError(
                "Notification delivery result status is invalid."
            );
        }

        if (result.status == NotificationDeliveryStatus::failed
            && result.error_message.empty())
        {
            throw NotificationDeliveryError(
                "Failed notification delivery result must contain error_message."
            );
        }

        if (result.status == NotificationDeliveryStatus::delivered
            && !result.error_message.empty())
        {
            throw NotificationDeliveryError(
                "Delivered notification delivery result must not contain error_message."
            );
        }

        if (result.status == NotificationDeliveryStatus::skipped
            && result.diagnostic_message.empty())
        {
            throw NotificationDeliveryError(
                "Skipped notification delivery result must contain diagnostic_message."
            );
        }
    }

    std::string NotificationDeliveryValidator::channel_type_to_string(
        NotificationDeliveryChannelType channel_type
    )
    {
        switch (channel_type)
        {
        case NotificationDeliveryChannelType::console:
            return "console";

        case NotificationDeliveryChannelType::email:
            return "email";

        case NotificationDeliveryChannelType::sms:
            return "sms";

        case NotificationDeliveryChannelType::webhook:
            return "webhook";

        case NotificationDeliveryChannelType::file:
            return "file";

        case NotificationDeliveryChannelType::test:
            return "test";
        }

        return "unknown";
    }

    std::string NotificationDeliveryValidator::priority_to_string(
        NotificationDeliveryPriority priority
    )
    {
        switch (priority)
        {
        case NotificationDeliveryPriority::low:
            return "low";

        case NotificationDeliveryPriority::normal:
            return "normal";

        case NotificationDeliveryPriority::high:
            return "high";

        case NotificationDeliveryPriority::critical:
            return "critical";
        }

        return "unknown";
    }

    std::string NotificationDeliveryValidator::status_to_string(
        NotificationDeliveryStatus status
    )
    {
        switch (status)
        {
        case NotificationDeliveryStatus::pending:
            return "pending";

        case NotificationDeliveryStatus::delivered:
            return "delivered";

        case NotificationDeliveryStatus::failed:
            return "failed";

        case NotificationDeliveryStatus::skipped:
            return "skipped";
        }

        return "unknown";
    }

    bool NotificationDeliveryValidator::is_valid_channel_type(
        NotificationDeliveryChannelType channel_type
    )
    {
        switch (channel_type)
        {
        case NotificationDeliveryChannelType::console:
        case NotificationDeliveryChannelType::email:
        case NotificationDeliveryChannelType::sms:
        case NotificationDeliveryChannelType::webhook:
        case NotificationDeliveryChannelType::file:
        case NotificationDeliveryChannelType::test:
            return true;
        }

        return false;
    }

    bool NotificationDeliveryValidator::is_valid_priority(
        NotificationDeliveryPriority priority
    )
    {
        switch (priority)
        {
        case NotificationDeliveryPriority::low:
        case NotificationDeliveryPriority::normal:
        case NotificationDeliveryPriority::high:
        case NotificationDeliveryPriority::critical:
            return true;
        }

        return false;
    }

    bool NotificationDeliveryValidator::is_valid_status(
        NotificationDeliveryStatus status
    )
    {
        switch (status)
        {
        case NotificationDeliveryStatus::pending:
        case NotificationDeliveryStatus::delivered:
        case NotificationDeliveryStatus::failed:
        case NotificationDeliveryStatus::skipped:
            return true;
        }

        return false;
    }
}