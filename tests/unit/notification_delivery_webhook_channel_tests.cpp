#include <dispatcher/notification/delivery/notification_delivery.hpp>

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    class ScriptedWebhookHttpClient final
        : public dispatcher::notification::delivery::IWebhookHttpClient
    {
    public:
        explicit ScriptedWebhookHttpClient(
            dispatcher::notification::delivery::WebhookHttpResponse response
        )
            : response_(
                std::move(
                    response
                )
            )
        {
        }

        [[nodiscard]] dispatcher::notification::delivery::WebhookHttpResponse post(
            const dispatcher::notification::delivery::WebhookHttpRequest& request
        ) override
        {
            ++post_count_;

            requests.push_back(
                request
            );

            if (!exception_message_.empty())
            {
                throw std::runtime_error{
                    exception_message_
                };
            }

            return response_;
        }

        void set_response(
            dispatcher::notification::delivery::WebhookHttpResponse response
        )
        {
            response_ =
                std::move(
                    response
                );
        }

        void set_exception(
            std::string exception_message
        )
        {
            exception_message_ =
                std::move(
                    exception_message
                );
        }

        [[nodiscard]] int post_count() const noexcept
        {
            return post_count_;
        }

        std::vector<dispatcher::notification::delivery::WebhookHttpRequest> requests{};

    private:
        dispatcher::notification::delivery::WebhookHttpResponse response_;
        std::string exception_message_{};
        int post_count_{ 0 };
    };

    dispatcher::notification::delivery::WebhookNotificationDeliveryOptions make_options()
    {
        dispatcher::notification::delivery::WebhookNotificationDeliveryOptions options;

        options.url =
            "https://hooks.example.local/dispatcher";

        options.default_headers.emplace(
            "X-Dispatcher-Test",
            "true"
        );

        options.timeout_ms = 2500;
        options.success_status_min = 200;
        options.success_status_max = 299;
        options.include_attributes = true;

        return options;
    }

    dispatcher::notification::delivery::NotificationDeliveryMessage make_webhook_message(
        std::string message_id = "message-1"
    )
    {
        dispatcher::notification::delivery::NotificationDeliveryMessage message;

        message.message_id =
            std::move(
                message_id
            );

        message.correlation_id = "alarm-1";
        message.source = "alarm-routing";
        message.channel_type =
            dispatcher::notification::delivery::NotificationDeliveryChannelType::webhook;
        message.priority =
            dispatcher::notification::delivery::NotificationDeliveryPriority::critical;

        message.recipient.recipient_id = "operator-1";
        message.recipient.display_name = "Operator One";
        message.recipient.address = "operator-1@example.local";

        message.subject = "Alarm raised";
        message.body = "Pump pressure is high.";

        message.attributes.emplace(
            "alarm_id",
            "alarm-1"
        );

        message.attributes.emplace(
            "tag_id",
            "pump.pressure"
        );

        return message;
    }

    void expect_channel_construction_throws(
        const dispatcher::notification::delivery::WebhookNotificationDeliveryOptions& options,
        dispatcher::notification::delivery::IWebhookHttpClient& client
    )
    {
        EXPECT_THROW(
            {
                dispatcher::notification::delivery::WebhookNotificationDeliveryChannel channel(
                    options,
                    client
                );

                static_cast<void>(
                    channel
                );
            },
            dispatcher::notification::delivery::NotificationDeliveryError
        );
    }

    void expect_deliver_throws(
        dispatcher::notification::delivery::WebhookNotificationDeliveryChannel& channel,
        const dispatcher::notification::delivery::NotificationDeliveryMessage& message
    )
    {
        EXPECT_THROW(
            {
                const auto result =
                    channel.deliver(
                        message
                    );

                static_cast<void>(
                    result
                );
            },
            dispatcher::notification::delivery::NotificationDeliveryError
        );
    }
}

TEST(NotificationDeliveryWebhookChannelTests, ExposesWebhookChannelMetadata)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            200,
            "{}",
            "ok"
        }
    };

    dispatcher::notification::delivery::WebhookNotificationDeliveryChannel channel(
        make_options(),
        client
    );

    EXPECT_EQ(
        channel.channel_type(),
        dispatcher::notification::delivery::NotificationDeliveryChannelType::webhook
    );

    EXPECT_EQ(
        channel.channel_name(),
        "webhook-notification-delivery"
    );

    EXPECT_EQ(
        channel.options().url,
        "https://hooks.example.local/dispatcher"
    );
}

TEST(NotificationDeliveryWebhookChannelTests, DeliversSuccessfulWebhookRequest)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            200,
            "{\"ok\":true}",
            "accepted"
        }
    };

    dispatcher::notification::delivery::WebhookNotificationDeliveryChannel channel(
        make_options(),
        client
    );

    const auto result =
        channel.deliver(
            make_webhook_message()
        );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_EQ(
        result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::delivered
    );

    EXPECT_EQ(
        result.provider_message_id,
        "webhook:message-1:200"
    );

    EXPECT_EQ(
        result.diagnostic_message,
        "accepted"
    );

    ASSERT_EQ(
        client.requests.size(),
        1U
    );

    EXPECT_EQ(
        client.requests[0].method,
        "POST"
    );

    EXPECT_EQ(
        client.requests[0].url,
        "https://hooks.example.local/dispatcher"
    );

    EXPECT_EQ(
        client.requests[0].timeout_ms,
        2500U
    );

    EXPECT_EQ(
        client.requests[0].headers.at(
            "Content-Type"
        ),
        "application/json"
    );

    EXPECT_EQ(
        client.requests[0].headers.at(
            "X-Dispatcher-Test"
        ),
        "true"
    );

    EXPECT_NE(
        client.requests[0].body.find(
            "\"message_id\": \"message-1\""
        ),
        std::string::npos
    );

    EXPECT_NE(
        client.requests[0].body.find(
            "\"channel_type\": \"webhook\""
        ),
        std::string::npos
    );

    EXPECT_NE(
        client.requests[0].body.find(
            "\"priority\": \"critical\""
        ),
        std::string::npos
    );

    EXPECT_NE(
        client.requests[0].body.find(
            "\"alarm_id\": \"alarm-1\""
        ),
        std::string::npos
    );
}

TEST(NotificationDeliveryWebhookChannelTests, AcceptsNoContentSuccessStatus)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            204,
            "",
            ""
        }
    };

    dispatcher::notification::delivery::WebhookNotificationDeliveryChannel channel(
        make_options(),
        client
    );

    const auto result =
        channel.deliver(
            make_webhook_message()
        );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_EQ(
        result.provider_message_id,
        "webhook:message-1:204"
    );

    EXPECT_EQ(
        result.diagnostic_message,
        "Delivered by webhook notification delivery channel."
    );
}

TEST(NotificationDeliveryWebhookChannelTests, ReturnsFailedForNonSuccessStatus)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            500,
            "server error",
            "failed"
        }
    };

    dispatcher::notification::delivery::WebhookNotificationDeliveryChannel channel(
        make_options(),
        client
    );

    const auto result =
        channel.deliver(
            make_webhook_message()
        );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_EQ(
        result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::failed
    );

    EXPECT_NE(
        result.error_message.find(
            "HTTP status 500"
        ),
        std::string::npos
    );

    EXPECT_EQ(
        result.diagnostic_message,
        "server error"
    );
}

TEST(NotificationDeliveryWebhookChannelTests, SupportsCustomSuccessStatusRange)
{
    auto options =
        make_options();

    options.success_status_min = 202;
    options.success_status_max = 202;

    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            202,
            "{}",
            "queued"
        }
    };

    dispatcher::notification::delivery::WebhookNotificationDeliveryChannel channel(
        options,
        client
    );

    const auto result =
        channel.deliver(
            make_webhook_message()
        );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_EQ(
        result.provider_message_id,
        "webhook:message-1:202"
    );
}

TEST(NotificationDeliveryWebhookChannelTests, SkipsWrongChannelTypeWithoutPosting)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            200,
            "{}",
            "accepted"
        }
    };

    dispatcher::notification::delivery::WebhookNotificationDeliveryChannel channel(
        make_options(),
        client
    );

    auto message =
        make_webhook_message();

    message.channel_type =
        dispatcher::notification::delivery::NotificationDeliveryChannelType::test;

    const auto result =
        channel.deliver(
            message
        );

    EXPECT_EQ(
        result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::skipped
    );

    EXPECT_EQ(
        client.post_count(),
        0
    );
}

TEST(NotificationDeliveryWebhookChannelTests, ConvertsClientExceptionToFailedResult)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            200,
            "{}",
            "accepted"
        }
    };

    client.set_exception(
        "connection refused"
    );

    dispatcher::notification::delivery::WebhookNotificationDeliveryChannel channel(
        make_options(),
        client
    );

    const auto result =
        channel.deliver(
            make_webhook_message()
        );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_EQ(
        result.error_message,
        "connection refused"
    );

    EXPECT_EQ(
        result.diagnostic_message,
        "Webhook HTTP client threw an exception."
    );

    EXPECT_EQ(
        client.post_count(),
        1
    );
}

TEST(NotificationDeliveryWebhookChannelTests, RejectsInvalidMessage)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            200,
            "{}",
            "accepted"
        }
    };

    dispatcher::notification::delivery::WebhookNotificationDeliveryChannel channel(
        make_options(),
        client
    );

    auto message =
        make_webhook_message();

    message.body = "";

    expect_deliver_throws(
        channel,
        message
    );

    EXPECT_EQ(
        client.post_count(),
        0
    );
}

TEST(NotificationDeliveryWebhookChannelTests, RejectsEmptyUrl)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            200,
            "{}",
            "accepted"
        }
    };

    auto options =
        make_options();

    options.url = "";

    expect_channel_construction_throws(
        options,
        client
    );
}

TEST(NotificationDeliveryWebhookChannelTests, RejectsUnsupportedUrlScheme)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            200,
            "{}",
            "accepted"
        }
    };

    auto options =
        make_options();

    options.url = "ftp://hooks.example.local/dispatcher";

    expect_channel_construction_throws(
        options,
        client
    );
}

TEST(NotificationDeliveryWebhookChannelTests, RejectsZeroTimeout)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            200,
            "{}",
            "accepted"
        }
    };

    auto options =
        make_options();

    options.timeout_ms = 0;

    expect_channel_construction_throws(
        options,
        client
    );
}

TEST(NotificationDeliveryWebhookChannelTests, RejectsInvalidSuccessStatusRange)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            200,
            "{}",
            "accepted"
        }
    };

    auto options =
        make_options();

    options.success_status_min = 300;
    options.success_status_max = 200;

    expect_channel_construction_throws(
        options,
        client
    );
}

TEST(NotificationDeliveryWebhookChannelTests, RejectsOutOfRangeSuccessStatus)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            200,
            "{}",
            "accepted"
        }
    };

    auto options =
        make_options();

    options.success_status_min = 99;
    options.success_status_max = 200;

    expect_channel_construction_throws(
        options,
        client
    );
}

TEST(NotificationDeliveryWebhookChannelTests, EscapesJsonPayloadValues)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            200,
            "{}",
            "accepted"
        }
    };

    dispatcher::notification::delivery::WebhookNotificationDeliveryChannel channel(
        make_options(),
        client
    );

    auto message =
        make_webhook_message();

    message.subject =
        "Alarm \"raised\"";

    message.body =
        "Line 1\nLine 2";

    const auto result =
        channel.deliver(
            message
        );

    EXPECT_TRUE(
        result.success()
    );

    ASSERT_EQ(
        client.requests.size(),
        1U
    );

    EXPECT_NE(
        client.requests[0].body.find(
            "Alarm \\\"raised\\\""
        ),
        std::string::npos
    );

    EXPECT_NE(
        client.requests[0].body.find(
            "Line 1\\nLine 2"
        ),
        std::string::npos
    );
}

TEST(NotificationDeliveryWebhookChannelTests, CanExcludeAttributesFromPayload)
{
    auto options =
        make_options();

    options.include_attributes = false;

    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            200,
            "{}",
            "accepted"
        }
    };

    dispatcher::notification::delivery::WebhookNotificationDeliveryChannel channel(
        options,
        client
    );

    const auto result =
        channel.deliver(
            make_webhook_message()
        );

    EXPECT_TRUE(
        result.success()
    );

    ASSERT_EQ(
        client.requests.size(),
        1U
    );

    EXPECT_EQ(
        client.requests[0].body.find(
            "\"attributes\""
        ),
        std::string::npos
    );

    EXPECT_EQ(
        client.requests[0].body.find(
            "\"alarm_id\""
        ),
        std::string::npos
    );
}

TEST(NotificationDeliveryWebhookChannelTests, DispatcherDeliversThroughWebhookChannel)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            200,
            "{}",
            "accepted"
        }
    };

    dispatcher::notification::delivery::WebhookNotificationDeliveryChannel channel(
        make_options(),
        client
    );

    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher.register_channel(
        channel
    );

    const auto result =
        dispatcher.deliver(
            make_webhook_message()
        );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_EQ(
        client.post_count(),
        1
    );
}

TEST(NotificationDeliveryWebhookChannelTests, RetryExecutorRetriesWebhookFailure)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            500,
            "server error",
            "failed"
        }
    };

    dispatcher::notification::delivery::WebhookNotificationDeliveryChannel channel(
        make_options(),
        client
    );

    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher.register_channel(
        channel
    );

    dispatcher::notification::delivery::NotificationDeliveryRetryPolicy policy;

    policy.max_attempts = 2;
    policy.retry_failed = true;

    dispatcher::notification::delivery::NotificationDeliveryRetryExecutor executor(
        dispatcher,
        policy
    );

    const auto result =
        executor.deliver_with_retry(
            make_webhook_message()
        );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_EQ(
        result.attempt_count(),
        2U
    );

    EXPECT_EQ(
        client.post_count(),
        2
    );
}