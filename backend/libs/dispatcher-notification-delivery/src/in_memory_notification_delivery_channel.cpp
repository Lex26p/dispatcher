#include <dispatcher/notification/delivery/in_memory_notification_delivery_channel.hpp>

#include <dispatcher/notification/delivery/notification_delivery_validator.hpp>

#include <utility>

namespace dispatcher::notification::delivery
{
    InMemoryNotificationDeliveryChannel::InMemoryNotificationDeliveryChannel(
        NotificationDeliveryChannelType channel_type,
        std::string channel_name
    )
        : channel_type_(
            channel_type
        )
        , channel_name_(
            std::move(
                channel_name
            )
        )
    {
    }

    NotificationDeliveryChannelType InMemoryNotificationDeliveryChannel::channel_type() const noexcept
    {
        return channel_type_;
    }

    std::string InMemoryNotificationDeliveryChannel::channel_name() const
    {
        return channel_name_;
    }

    NotificationDeliveryResult InMemoryNotificationDeliveryChannel::deliver(
        const NotificationDeliveryMessage& message
    )
    {
        ++delivery_attempt_count_;

        NotificationDeliveryValidator::validate_message(
            message
        );

        if (message.channel_type != channel_type_)
        {
            return NotificationDeliveryResult::skipped(
                "Message channel_type does not match in-memory delivery channel."
            );
        }

        if (!failure_message_.empty())
        {
            return NotificationDeliveryResult::failed(
                failure_message_,
                "In-memory delivery channel forced failure."
            );
        }

        delivered_messages_.push_back(
            message
        );

        return NotificationDeliveryResult::delivered(
            provider_message_id_for(
                message
            ),
            "Delivered by in-memory notification delivery channel."
        );
    }

    void InMemoryNotificationDeliveryChannel::set_failure(
        std::string error_message
    )
    {
        failure_message_ =
            std::move(
                error_message
            );
    }

    void InMemoryNotificationDeliveryChannel::clear_failure()
    {
        failure_message_.clear();
    }

    void InMemoryNotificationDeliveryChannel::clear()
    {
        failure_message_.clear();
        delivery_attempt_count_ = 0;
        delivered_messages_.clear();
    }

    const std::vector<NotificationDeliveryMessage>& InMemoryNotificationDeliveryChannel::delivered_messages() const noexcept
    {
        return delivered_messages_;
    }

    int InMemoryNotificationDeliveryChannel::delivery_attempt_count() const noexcept
    {
        return delivery_attempt_count_;
    }

    std::string InMemoryNotificationDeliveryChannel::provider_message_id_for(
        const NotificationDeliveryMessage& message
    ) const
    {
        return channel_name_
            + ":"
            + message.message_id;
    }
}