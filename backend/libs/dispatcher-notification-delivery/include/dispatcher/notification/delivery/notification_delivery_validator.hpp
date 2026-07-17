#pragma once

#include <dispatcher/notification/delivery/notification_delivery_types.hpp>

#include <string>

namespace dispatcher::notification::delivery
{
    class NotificationDeliveryValidator final
    {
    public:
        static void validate_message(
            const NotificationDeliveryMessage& message
        );

        static void validate_result(
            const NotificationDeliveryResult& result
        );

        [[nodiscard]] static std::string channel_type_to_string(
            NotificationDeliveryChannelType channel_type
        );

        [[nodiscard]] static std::string priority_to_string(
            NotificationDeliveryPriority priority
        );

        [[nodiscard]] static std::string status_to_string(
            NotificationDeliveryStatus status
        );

    private:
        [[nodiscard]] static bool is_valid_channel_type(
            NotificationDeliveryChannelType channel_type
        );

        [[nodiscard]] static bool is_valid_priority(
            NotificationDeliveryPriority priority
        );

        [[nodiscard]] static bool is_valid_status(
            NotificationDeliveryStatus status
        );
    };
}