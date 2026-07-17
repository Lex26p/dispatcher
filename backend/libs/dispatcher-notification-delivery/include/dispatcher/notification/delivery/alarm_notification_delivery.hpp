#pragma once

#include <dispatcher/notification/delivery/notification_delivery_dispatcher.hpp>
#include <dispatcher/notification/delivery/notification_delivery_retry.hpp>
#include <dispatcher/notification/delivery/notification_delivery_types.hpp>

#include <map>
#include <string>

namespace dispatcher::notification::delivery
{
    enum class AlarmNotificationSeverity
    {
        info,
        warning,
        major,
        critical
    };

    struct AlarmNotificationDeliveryRequest
    {
        std::string notification_id{};
        std::string alarm_id{};
        std::string tag_id{};
        std::string alarm_name{};
        std::string alarm_state{};
        std::string source{ "alarm-routing" };

        AlarmNotificationSeverity severity{
            AlarmNotificationSeverity::warning
        };

        NotificationDeliveryChannelType channel_type{
            NotificationDeliveryChannelType::test
        };

        NotificationDeliveryRecipient recipient{};

        std::string subject_prefix{ "Alarm notification" };
        std::string body_details{};

        std::map<std::string, std::string> attributes{};
    };

    class AlarmNotificationDeliveryMessageBuilder final
    {
    public:
        [[nodiscard]] static NotificationDeliveryMessage build_message(
            const AlarmNotificationDeliveryRequest& request
        );

        [[nodiscard]] static NotificationDeliveryResult deliver(
            NotificationDeliveryDispatcher& dispatcher,
            const AlarmNotificationDeliveryRequest& request
        );

        [[nodiscard]] static NotificationDeliveryExecutionResult deliver_with_retry(
            NotificationDeliveryRetryExecutor& executor,
            const AlarmNotificationDeliveryRequest& request
        );

        [[nodiscard]] static NotificationDeliveryPriority priority_for_severity(
            AlarmNotificationSeverity severity
        );

        [[nodiscard]] static std::string severity_to_string(
            AlarmNotificationSeverity severity
        );

        static void validate_request(
            const AlarmNotificationDeliveryRequest& request
        );

    private:
        [[nodiscard]] static std::string make_subject(
            const AlarmNotificationDeliveryRequest& request
        );

        [[nodiscard]] static std::string make_body(
            const AlarmNotificationDeliveryRequest& request
        );
    };
}