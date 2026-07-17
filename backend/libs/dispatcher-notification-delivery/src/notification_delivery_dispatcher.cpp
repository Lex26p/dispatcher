#include <dispatcher/notification/delivery/notification_delivery_dispatcher.hpp>

#include <dispatcher/notification/delivery/notification_delivery_error.hpp>
#include <dispatcher/notification/delivery/notification_delivery_validator.hpp>

#include <exception>
#include <string>

namespace dispatcher::notification::delivery
{
    void NotificationDeliveryDispatcher::register_channel(
        INotificationDeliveryChannel& channel
    )
    {
        if (channel.channel_name().empty())
        {
            throw NotificationDeliveryError(
                "Notification delivery channel_name must not be empty."
            );
        }

        const auto channel_type =
            channel.channel_type();

        if (channels_.contains(
            channel_type
        ))
        {
            throw NotificationDeliveryError(
                "Notification delivery channel is already registered for type: "
                + NotificationDeliveryValidator::channel_type_to_string(
                    channel_type
                )
                + "."
            );
        }

        channels_.emplace(
            channel_type,
            &channel
        );
    }

    void NotificationDeliveryDispatcher::clear_channels()
    {
        channels_.clear();
    }

    bool NotificationDeliveryDispatcher::has_channel(
        NotificationDeliveryChannelType channel_type
    ) const
    {
        return channels_.contains(
            channel_type
        );
    }

    std::size_t NotificationDeliveryDispatcher::channel_count() const noexcept
    {
        return channels_.size();
    }

    NotificationDeliveryResult NotificationDeliveryDispatcher::deliver(
        const NotificationDeliveryMessage& message
    )
    {
        NotificationDeliveryValidator::validate_message(
            message
        );

        auto* channel =
            find_channel(
                message.channel_type
            );

        if (channel == nullptr)
        {
            return NotificationDeliveryResult::failed(
                "No notification delivery channel registered for type: "
                + NotificationDeliveryValidator::channel_type_to_string(
                    message.channel_type
                )
                + ".",
                "NotificationDeliveryDispatcher missing channel."
            );
        }

        try
        {
            auto result =
                channel->deliver(
                    message
                );

            NotificationDeliveryValidator::validate_result(
                result
            );

            return result;
        }
        catch (const std::exception& exception)
        {
            return NotificationDeliveryResult::failed(
                exception.what(),
                "Notification delivery channel threw an exception."
            );
        }
    }

    std::vector<NotificationDeliveryResult> NotificationDeliveryDispatcher::deliver_batch(
        const std::vector<NotificationDeliveryMessage>& messages
    )
    {
        std::vector<NotificationDeliveryResult> results;

        results.reserve(
            messages.size()
        );

        for (const auto& message : messages)
        {
            results.push_back(
                deliver(
                    message
                )
            );
        }

        return results;
    }

    INotificationDeliveryChannel* NotificationDeliveryDispatcher::find_channel(
        NotificationDeliveryChannelType channel_type
    ) const
    {
        const auto iterator =
            channels_.find(
                channel_type
            );

        if (iterator == channels_.end())
        {
            return nullptr;
        }

        return iterator->second;
    }
}