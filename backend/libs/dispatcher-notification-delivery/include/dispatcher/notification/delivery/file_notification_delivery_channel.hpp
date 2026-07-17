#pragma once

#include <dispatcher/notification/delivery/notification_delivery_channel.hpp>

#include <filesystem>
#include <string>

namespace dispatcher::notification::delivery
{
    struct FileNotificationDeliveryOptions
    {
        std::filesystem::path directory{ "notifications" };
        std::string file_name{ "notifications.log" };

        bool create_directories{ true };
        bool append{ true };
    };

    class FileNotificationDeliveryChannel final
        : public INotificationDeliveryChannel
    {
    public:
        explicit FileNotificationDeliveryChannel(
            FileNotificationDeliveryOptions options = {}
        );

        [[nodiscard]] NotificationDeliveryChannelType channel_type() const noexcept override;

        [[nodiscard]] std::string channel_name() const override;

        [[nodiscard]] NotificationDeliveryResult deliver(
            const NotificationDeliveryMessage& message
        ) override;

        [[nodiscard]] const FileNotificationDeliveryOptions& options() const noexcept;

        [[nodiscard]] std::filesystem::path output_path() const;

    private:
        FileNotificationDeliveryOptions options_;

        static void validate_options(
            const FileNotificationDeliveryOptions& options
        );

        [[nodiscard]] static std::string sanitize_line_value(
            const std::string& value
        );

        [[nodiscard]] static std::string make_provider_message_id(
            const std::filesystem::path& path,
            const NotificationDeliveryMessage& message
        );
    };
}