#pragma once

#include <dispatcher/notification/delivery/notification_delivery_channel.hpp>

#include <map>
#include <vector>

namespace dispatcher::notification::delivery
{
    class NotificationDeliveryDispatcher final
    {
    public:
        NotificationDeliveryDispatcher() = default;

        void register_channel(
            INotificationDeliveryChannel& channel
        );

        void clear_channels();

        [[nodiscard]] bool has_channel(
            NotificationDeliveryChannelType channel_type
        ) const;

        [[nodiscard]] std::size_t channel_count() const noexcept;

        [[nodiscard]] NotificationDeliveryResult deliver(
            const NotificationDeliveryMessage& message
        );

        [[nodiscard]] std::vector<NotificationDeliveryResult> deliver_batch(
            const std::vector<NotificationDeliveryMessage>& messages
        );

    private:
        std::map<NotificationDeliveryChannelType, INotificationDeliveryChannel*> channels_{};

        [[nodiscard]] INotificationDeliveryChannel* find_channel(
            NotificationDeliveryChannelType channel_type
        ) const;
    };
}