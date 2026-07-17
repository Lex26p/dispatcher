#pragma once

#include <dispatcher/notification/delivery/notification_delivery_channel.hpp>

#include <string>
#include <vector>

namespace dispatcher::notification::delivery
{
    class InMemoryNotificationDeliveryChannel final
        : public INotificationDeliveryChannel
    {
    public:
        explicit InMemoryNotificationDeliveryChannel(
            NotificationDeliveryChannelType channel_type = NotificationDeliveryChannelType::test,
            std::string channel_name = "in-memory-notification-delivery"
        );

        [[nodiscard]] NotificationDeliveryChannelType channel_type() const noexcept override;

        [[nodiscard]] std::string channel_name() const override;

        [[nodiscard]] NotificationDeliveryResult deliver(
            const NotificationDeliveryMessage& message
        ) override;

        void set_failure(
            std::string error_message
        );

        void clear_failure();

        void clear();

        [[nodiscard]] const std::vector<NotificationDeliveryMessage>& delivered_messages() const noexcept;

        [[nodiscard]] int delivery_attempt_count() const noexcept;

    private:
        NotificationDeliveryChannelType channel_type_;
        std::string channel_name_;

        std::string failure_message_{};

        int delivery_attempt_count_{ 0 };

        std::vector<NotificationDeliveryMessage> delivered_messages_{};

        [[nodiscard]] std::string provider_message_id_for(
            const NotificationDeliveryMessage& message
        ) const;
    };
}