#include <dispatcher/notification/delivery/webhook_notification_delivery_channel.hpp>

#include <dispatcher/notification/delivery/notification_delivery_error.hpp>
#include <dispatcher/notification/delivery/notification_delivery_validator.hpp>

#include <exception>
#include <sstream>
#include <string>
#include <utility>

namespace dispatcher::notification::delivery
{
    namespace
    {
        [[nodiscard]] bool starts_with(
            const std::string& value,
            const std::string& prefix
        )
        {
            return value.rfind(
                prefix,
                0
            ) == 0;
        }

        void append_json_string_field(
            std::ostringstream& output,
            const std::string& name,
            const std::string& value,
            bool trailing_comma
        )
        {
            output
                << "  \""
                << WebhookNotificationDeliveryChannel::json_escape(
                    name
                )
                << "\": \""
                << WebhookNotificationDeliveryChannel::json_escape(
                    value
                )
                << "\"";

            if (trailing_comma)
            {
                output << ",";
            }

            output << "\n";
        }
    }

    WebhookNotificationDeliveryChannel::WebhookNotificationDeliveryChannel(
        WebhookNotificationDeliveryOptions options,
        IWebhookHttpClient& http_client
    )
        : options_(
            std::move(
                options
            )
        )
        , http_client_(
            &http_client
        )
    {
        validate_options(
            options_
        );
    }

    NotificationDeliveryChannelType WebhookNotificationDeliveryChannel::channel_type() const noexcept
    {
        return NotificationDeliveryChannelType::webhook;
    }

    std::string WebhookNotificationDeliveryChannel::channel_name() const
    {
        return "webhook-notification-delivery";
    }

    NotificationDeliveryResult WebhookNotificationDeliveryChannel::deliver(
        const NotificationDeliveryMessage& message
    )
    {
        NotificationDeliveryValidator::validate_message(
            message
        );

        if (message.channel_type != NotificationDeliveryChannelType::webhook)
        {
            return NotificationDeliveryResult::skipped(
                "Message channel_type does not match webhook notification delivery channel."
            );
        }

        try
        {
            const auto request =
                build_request(
                    message
                );

            const auto response =
                http_client_->post(
                    request
                );

            if (is_success_status(
                response.status_code,
                options_
            ))
            {
                return NotificationDeliveryResult::delivered(
                    make_provider_message_id(
                        message,
                        response.status_code
                    ),
                    response.diagnostic_message.empty()
                    ? "Delivered by webhook notification delivery channel."
                    : response.diagnostic_message
                );
            }

            return NotificationDeliveryResult::failed(
                "Webhook delivery failed with HTTP status "
                + std::to_string(
                    response.status_code
                )
                + ".",
                response.body
            );
        }
        catch (const std::exception& exception)
        {
            return NotificationDeliveryResult::failed(
                exception.what(),
                "Webhook HTTP client threw an exception."
            );
        }
    }

    const WebhookNotificationDeliveryOptions& WebhookNotificationDeliveryChannel::options() const noexcept
    {
        return options_;
    }

    void WebhookNotificationDeliveryChannel::validate_options(
        const WebhookNotificationDeliveryOptions& options
    )
    {
        if (options.url.empty())
        {
            throw NotificationDeliveryError(
                "Webhook notification delivery url must not be empty."
            );
        }

        if (!starts_with(
            options.url,
            "http://"
        )
            && !starts_with(
                options.url,
                "https://"
            ))
        {
            throw NotificationDeliveryError(
                "Webhook notification delivery url must start with http:// or https://."
            );
        }

        if (options.timeout_ms == 0)
        {
            throw NotificationDeliveryError(
                "Webhook notification delivery timeout_ms must not be zero."
            );
        }

        if (options.success_status_min < 100
            || options.success_status_min > 599
            || options.success_status_max < 100
            || options.success_status_max > 599)
        {
            throw NotificationDeliveryError(
                "Webhook notification delivery success status range must be between 100 and 599."
            );
        }

        if (options.success_status_min > options.success_status_max)
        {
            throw NotificationDeliveryError(
                "Webhook notification delivery success_status_min must not exceed success_status_max."
            );
        }
    }

    WebhookHttpRequest WebhookNotificationDeliveryChannel::build_request(
        const NotificationDeliveryMessage& message
    ) const
    {
        WebhookHttpRequest request;

        request.method = "POST";
        request.url =
            options_.url;
        request.timeout_ms =
            options_.timeout_ms;
        request.headers =
            options_.default_headers;

        request.headers.insert_or_assign(
            "Content-Type",
            "application/json"
        );

        request.body =
            make_json_payload(
                message,
                options_.include_attributes
            );

        return request;
    }

    std::string WebhookNotificationDeliveryChannel::make_json_payload(
        const NotificationDeliveryMessage& message,
        bool include_attributes
    )
    {
        std::ostringstream output;

        output << "{\n";

        append_json_string_field(
            output,
            "message_id",
            message.message_id,
            true
        );

        append_json_string_field(
            output,
            "correlation_id",
            message.correlation_id,
            true
        );

        append_json_string_field(
            output,
            "source",
            message.source,
            true
        );

        append_json_string_field(
            output,
            "channel_type",
            NotificationDeliveryValidator::channel_type_to_string(
                message.channel_type
            ),
            true
        );

        append_json_string_field(
            output,
            "priority",
            NotificationDeliveryValidator::priority_to_string(
                message.priority
            ),
            true
        );

        output << "  \"recipient\": {\n";

        output
            << "    \"recipient_id\": \""
            << json_escape(
                message.recipient.recipient_id
            )
            << "\",\n";

        output
            << "    \"display_name\": \""
            << json_escape(
                message.recipient.display_name
            )
            << "\",\n";

        output
            << "    \"address\": \""
            << json_escape(
                message.recipient.address
            )
            << "\"\n";

        output << "  },\n";

        append_json_string_field(
            output,
            "subject",
            message.subject,
            true
        );

        append_json_string_field(
            output,
            "body",
            message.body,
            include_attributes
        );

        if (include_attributes)
        {
            output << "  \"attributes\": {";

            if (!message.attributes.empty())
            {
                output << "\n";

                std::size_t index = 0;

                for (const auto& [key, value] : message.attributes)
                {
                    output
                        << "    \""
                        << json_escape(
                            key
                        )
                        << "\": \""
                        << json_escape(
                            value
                        )
                        << "\"";

                    ++index;

                    if (index < message.attributes.size())
                    {
                        output << ",";
                    }

                    output << "\n";
                }

                output << "  }\n";
            }
            else
            {
                output << "}\n";
            }
        }

        output << "}";

        return output.str();
    }

    std::string WebhookNotificationDeliveryChannel::json_escape(
        const std::string& value
    )
    {
        std::string escaped;

        escaped.reserve(
            value.size()
        );

        for (const auto character : value)
        {
            switch (character)
            {
            case '\\':
                escaped += "\\\\";
                break;

            case '"':
                escaped += "\\\"";
                break;

            case '\n':
                escaped += "\\n";
                break;

            case '\r':
                escaped += "\\r";
                break;

            case '\t':
                escaped += "\\t";
                break;

            default:
                escaped.push_back(
                    character
                );
                break;
            }
        }

        return escaped;
    }

    bool WebhookNotificationDeliveryChannel::is_success_status(
        int status_code,
        const WebhookNotificationDeliveryOptions& options
    )
    {
        return status_code >= options.success_status_min
            && status_code <= options.success_status_max;
    }

    std::string WebhookNotificationDeliveryChannel::make_provider_message_id(
        const NotificationDeliveryMessage& message,
        int status_code
    )
    {
        return "webhook:"
            + message.message_id
            + ":"
            + std::to_string(
                status_code
            );
    }
}