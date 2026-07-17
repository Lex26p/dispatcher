#include <dispatcher/notification/delivery/alarm_notification_delivery.hpp>

#include <dispatcher/notification/delivery/notification_delivery_error.hpp>
#include <dispatcher/notification/delivery/notification_delivery_validator.hpp>

#include <utility>

namespace dispatcher::notification::delivery
{
    NotificationDeliveryMessage AlarmNotificationDeliveryMessageBuilder::build_message(
        const AlarmNotificationDeliveryRequest& request
    )
    {
        validate_request(
            request
        );

        NotificationDeliveryMessage message;

        message.message_id =
            request.notification_id;

        message.correlation_id =
            request.alarm_id;

        message.source =
            request.source;

        message.channel_type =
            request.channel_type;

        message.priority =
            priority_for_severity(
                request.severity
            );

        message.recipient =
            request.recipient;

        message.subject =
            make_subject(
                request
            );

        message.body =
            make_body(
                request
            );

        message.attributes =
            request.attributes;

        message.attributes.insert_or_assign(
            "alarm_id",
            request.alarm_id
        );

        message.attributes.insert_or_assign(
            "tag_id",
            request.tag_id
        );

        message.attributes.insert_or_assign(
            "alarm_name",
            request.alarm_name
        );

        message.attributes.insert_or_assign(
            "alarm_state",
            request.alarm_state
        );

        message.attributes.insert_or_assign(
            "severity",
            severity_to_string(
                request.severity
            )
        );

        NotificationDeliveryValidator::validate_message(
            message
        );

        return message;
    }

    NotificationDeliveryResult AlarmNotificationDeliveryMessageBuilder::deliver(
        NotificationDeliveryDispatcher& dispatcher,
        const AlarmNotificationDeliveryRequest& request
    )
    {
        return dispatcher.deliver(
            build_message(
                request
            )
        );
    }

    NotificationDeliveryExecutionResult AlarmNotificationDeliveryMessageBuilder::deliver_with_retry(
        NotificationDeliveryRetryExecutor& executor,
        const AlarmNotificationDeliveryRequest& request
    )
    {
        return executor.deliver_with_retry(
            build_message(
                request
            )
        );
    }

    NotificationDeliveryPriority AlarmNotificationDeliveryMessageBuilder::priority_for_severity(
        AlarmNotificationSeverity severity
    )
    {
        switch (severity)
        {
        case AlarmNotificationSeverity::info:
            return NotificationDeliveryPriority::low;

        case AlarmNotificationSeverity::warning:
            return NotificationDeliveryPriority::normal;

        case AlarmNotificationSeverity::major:
            return NotificationDeliveryPriority::high;

        case AlarmNotificationSeverity::critical:
            return NotificationDeliveryPriority::critical;
        }

        return NotificationDeliveryPriority::normal;
    }

    std::string AlarmNotificationDeliveryMessageBuilder::severity_to_string(
        AlarmNotificationSeverity severity
    )
    {
        switch (severity)
        {
        case AlarmNotificationSeverity::info:
            return "info";

        case AlarmNotificationSeverity::warning:
            return "warning";

        case AlarmNotificationSeverity::major:
            return "major";

        case AlarmNotificationSeverity::critical:
            return "critical";
        }

        return "unknown";
    }

    void AlarmNotificationDeliveryMessageBuilder::validate_request(
        const AlarmNotificationDeliveryRequest& request
    )
    {
        if (request.notification_id.empty())
        {
            throw NotificationDeliveryError(
                "Alarm notification delivery notification_id must not be empty."
            );
        }

        if (request.alarm_id.empty())
        {
            throw NotificationDeliveryError(
                "Alarm notification delivery alarm_id must not be empty."
            );
        }

        if (request.tag_id.empty())
        {
            throw NotificationDeliveryError(
                "Alarm notification delivery tag_id must not be empty."
            );
        }

        if (request.alarm_name.empty())
        {
            throw NotificationDeliveryError(
                "Alarm notification delivery alarm_name must not be empty."
            );
        }

        if (request.alarm_state.empty())
        {
            throw NotificationDeliveryError(
                "Alarm notification delivery alarm_state must not be empty."
            );
        }

        if (request.source.empty())
        {
            throw NotificationDeliveryError(
                "Alarm notification delivery source must not be empty."
            );
        }

        if (request.recipient.address.empty())
        {
            throw NotificationDeliveryError(
                "Alarm notification delivery recipient address must not be empty."
            );
        }

        if (request.subject_prefix.empty())
        {
            throw NotificationDeliveryError(
                "Alarm notification delivery subject_prefix must not be empty."
            );
        }
    }

    std::string AlarmNotificationDeliveryMessageBuilder::make_subject(
        const AlarmNotificationDeliveryRequest& request
    )
    {
        return request.subject_prefix
            + ": "
            + request.alarm_name
            + " ["
            + severity_to_string(
                request.severity
            )
            + "]";
    }

    std::string AlarmNotificationDeliveryMessageBuilder::make_body(
        const AlarmNotificationDeliveryRequest& request
    )
    {
        auto body =
            std::string{
                "Alarm: "
        }
            + request.alarm_name
            + "\n"
            + "Alarm ID: "
            + request.alarm_id
            + "\n"
            + "Tag ID: "
            + request.tag_id
            + "\n"
            + "State: "
            + request.alarm_state
            + "\n"
            + "Severity: "
            + severity_to_string(
                request.severity
            );

        if (!request.body_details.empty())
        {
            body += "\nDetails: ";
            body += request.body_details;
        }

        return body;
    }
}