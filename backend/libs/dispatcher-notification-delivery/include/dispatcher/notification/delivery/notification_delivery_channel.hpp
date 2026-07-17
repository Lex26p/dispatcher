#pragma once

#include <dispatcher/notification/delivery/notification_delivery_types.hpp>

#include <string>

namespace dispatcher::notification::delivery
{
    class INotificationDeliveryChannel
    {
    public:
        virtual ~INotificationDeliveryChannel() = default;

        [[nodiscard]] virtual NotificationDeliveryChannelType channel_type() const noexcept = 0;

        [[nodiscard]] virtual std::string channel_name() const = 0;

        [[nodiscard]] virtual NotificationDeliveryResult deliver(
            const NotificationDeliveryMessage& message
        ) = 0;
    };
}