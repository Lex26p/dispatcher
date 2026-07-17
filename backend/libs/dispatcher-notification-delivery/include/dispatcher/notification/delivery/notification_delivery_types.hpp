#pragma once

#include <chrono>
#include <map>
#include <string>

namespace dispatcher::notification::delivery
{
    enum class NotificationDeliveryChannelType
    {
        console,
        email,
        sms,
        webhook,
        file,
        test
    };

    enum class NotificationDeliveryPriority
    {
        low,
        normal,
        high,
        critical
    };

    enum class NotificationDeliveryStatus
    {
        pending,
        delivered,
        failed,
        skipped
    };

    struct NotificationDeliveryRecipient
    {
        std::string recipient_id{};
        std::string display_name{};
        std::string address{};
    };

    struct NotificationDeliveryMessage
    {
        std::string message_id{};
        std::string correlation_id{};
        std::string source{};

        NotificationDeliveryChannelType channel_type{
            NotificationDeliveryChannelType::test
        };

        NotificationDeliveryPriority priority{
            NotificationDeliveryPriority::normal
        };

        NotificationDeliveryRecipient recipient{};

        std::string subject{};
        std::string body{};

        std::map<std::string, std::string> attributes{};

        std::chrono::system_clock::time_point created_at{
            std::chrono::system_clock::now()
        };
    };

    struct NotificationDeliveryResult
    {
        NotificationDeliveryStatus status{
            NotificationDeliveryStatus::pending
        };

        std::string provider_message_id{};
        std::string error_message{};
        std::string diagnostic_message{};

        std::chrono::system_clock::time_point completed_at{
            std::chrono::system_clock::now()
        };

        [[nodiscard]] static NotificationDeliveryResult delivered(
            std::string provider_message_id = {},
            std::string diagnostic_message = {}
        )
        {
            NotificationDeliveryResult result;

            result.status =
                NotificationDeliveryStatus::delivered;

            result.provider_message_id =
                std::move(
                    provider_message_id
                );

            result.diagnostic_message =
                std::move(
                    diagnostic_message
                );

            result.completed_at =
                std::chrono::system_clock::now();

            return result;
        }

        [[nodiscard]] static NotificationDeliveryResult failed(
            std::string error_message,
            std::string diagnostic_message = {}
        )
        {
            NotificationDeliveryResult result;

            result.status =
                NotificationDeliveryStatus::failed;

            result.error_message =
                std::move(
                    error_message
                );

            result.diagnostic_message =
                std::move(
                    diagnostic_message
                );

            result.completed_at =
                std::chrono::system_clock::now();

            return result;
        }

        [[nodiscard]] static NotificationDeliveryResult skipped(
            std::string diagnostic_message
        )
        {
            NotificationDeliveryResult result;

            result.status =
                NotificationDeliveryStatus::skipped;

            result.diagnostic_message =
                std::move(
                    diagnostic_message
                );

            result.completed_at =
                std::chrono::system_clock::now();

            return result;
        }

        [[nodiscard]] bool success() const noexcept
        {
            return status == NotificationDeliveryStatus::delivered;
        }

        [[nodiscard]] bool failure() const noexcept
        {
            return status == NotificationDeliveryStatus::failed;
        }
    };
}