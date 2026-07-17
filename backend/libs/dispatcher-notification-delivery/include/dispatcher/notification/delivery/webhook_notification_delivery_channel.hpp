#pragma once

#include <dispatcher/notification/delivery/notification_delivery_channel.hpp>

#include <cstdint>
#include <map>
#include <string>

namespace dispatcher::notification::delivery
{
    struct WebhookHttpRequest
    {
        std::string method{ "POST" };
        std::string url{};
        std::map<std::string, std::string> headers{};
        std::string body{};
        std::uint32_t timeout_ms{ 5000 };
    };

    struct WebhookHttpResponse
    {
        int status_code{ 0 };
        std::string body{};
        std::string diagnostic_message{};
    };

    class IWebhookHttpClient
    {
    public:
        virtual ~IWebhookHttpClient() = default;

        [[nodiscard]] virtual WebhookHttpResponse post(
            const WebhookHttpRequest& request
        ) = 0;
    };

    struct WebhookNotificationDeliveryOptions
    {
        std::string url{};
        std::map<std::string, std::string> default_headers{};

        std::uint32_t timeout_ms{ 5000 };

        int success_status_min{ 200 };
        int success_status_max{ 299 };

        bool include_attributes{ true };
    };

    class WebhookNotificationDeliveryChannel final
        : public INotificationDeliveryChannel
    {
    public:
        WebhookNotificationDeliveryChannel(
            WebhookNotificationDeliveryOptions options,
            IWebhookHttpClient& http_client
        );

        [[nodiscard]] NotificationDeliveryChannelType channel_type() const noexcept override;

        [[nodiscard]] std::string channel_name() const override;

        [[nodiscard]] NotificationDeliveryResult deliver(
            const NotificationDeliveryMessage& message
        ) override;

        [[nodiscard]] const WebhookNotificationDeliveryOptions& options() const noexcept;

        [[nodiscard]] static std::string json_escape(
            const std::string& value
        );

    private:
        WebhookNotificationDeliveryOptions options_;
        IWebhookHttpClient* http_client_;

        static void validate_options(
            const WebhookNotificationDeliveryOptions& options
        );

        [[nodiscard]] WebhookHttpRequest build_request(
            const NotificationDeliveryMessage& message
        ) const;

        [[nodiscard]] static std::string make_json_payload(
            const NotificationDeliveryMessage& message,
            bool include_attributes
        );

        [[nodiscard]] static bool is_success_status(
            int status_code,
            const WebhookNotificationDeliveryOptions& options
        );

        [[nodiscard]] static std::string make_provider_message_id(
            const NotificationDeliveryMessage& message,
            int status_code
        );
    };
}