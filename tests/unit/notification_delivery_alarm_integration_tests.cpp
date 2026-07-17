#include <dispatcher/notification/delivery/notification_delivery.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    dispatcher::notification::delivery::AlarmNotificationDeliveryRequest make_alarm_request(
        dispatcher::notification::delivery::NotificationDeliveryChannelType channel_type =
        dispatcher::notification::delivery::NotificationDeliveryChannelType::test
    )
    {
        dispatcher::notification::delivery::AlarmNotificationDeliveryRequest request;

        request.notification_id = "notification-1";
        request.alarm_id = "alarm-1";
        request.tag_id = "pump.pressure";
        request.alarm_name = "Pump pressure high";
        request.alarm_state = "raised";
        request.source = "alarm-routing";
        request.severity =
            dispatcher::notification::delivery::AlarmNotificationSeverity::critical;
        request.channel_type =
            channel_type;

        request.recipient.recipient_id = "operator-1";
        request.recipient.display_name = "Operator One";
        request.recipient.address = "operator-1@example.local";

        request.subject_prefix = "Dispatcher alarm";
        request.body_details = "Pressure exceeded configured high limit.";

        request.attributes.emplace(
            "device_id",
            "pump-1"
        );

        request.attributes.emplace(
            "site",
            "plant-a"
        );

        return request;
    }

    void expect_build_message_throws(
        const dispatcher::notification::delivery::AlarmNotificationDeliveryRequest& request
    )
    {
        EXPECT_THROW(
            {
                const auto message =
                    dispatcher::notification::delivery::AlarmNotificationDeliveryMessageBuilder::build_message(
                        request
                    );

                static_cast<void>(
                    message
                );
            },
            dispatcher::notification::delivery::NotificationDeliveryError
        );
    }

    std::filesystem::path test_root(
        const std::string& test_name
    )
    {
        return std::filesystem::temp_directory_path()
            / "dispatcher-notification-delivery-alarm-tests"
            / test_name;
    }

    void clean_directory(
        const std::filesystem::path& path
    )
    {
        std::error_code error_code;

        const auto removed =
            std::filesystem::remove_all(
                path,
                error_code
            );

        static_cast<void>(
            removed
            );

        static_cast<void>(
            error_code
            );
    }

    std::string read_text_file(
        const std::filesystem::path& path
    )
    {
        std::ifstream input{
            path
        };

        return std::string{
            std::istreambuf_iterator<char>{
                input
            },
            std::istreambuf_iterator<char>{}
        };
    }

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
}

TEST(NotificationDeliveryAlarmIntegrationTests, BuildsDeliveryMessageFromAlarmRequest)
{
    const auto request =
        make_alarm_request();

    const auto message =
        dispatcher::notification::delivery::AlarmNotificationDeliveryMessageBuilder::build_message(
            request
        );

    EXPECT_EQ(
        message.message_id,
        "notification-1"
    );

    EXPECT_EQ(
        message.correlation_id,
        "alarm-1"
    );

    EXPECT_EQ(
        message.source,
        "alarm-routing"
    );

    EXPECT_EQ(
        message.channel_type,
        dispatcher::notification::delivery::NotificationDeliveryChannelType::test
    );

    EXPECT_EQ(
        message.priority,
        dispatcher::notification::delivery::NotificationDeliveryPriority::critical
    );

    EXPECT_EQ(
        message.recipient.address,
        "operator-1@example.local"
    );

    EXPECT_NE(
        message.subject.find(
            "Pump pressure high"
        ),
        std::string::npos
    );

    EXPECT_NE(
        message.body.find(
            "Pressure exceeded configured high limit."
        ),
        std::string::npos
    );

    EXPECT_EQ(
        message.attributes.at(
            "alarm_id"
        ),
        "alarm-1"
    );

    EXPECT_EQ(
        message.attributes.at(
            "tag_id"
        ),
        "pump.pressure"
    );

    EXPECT_EQ(
        message.attributes.at(
            "severity"
        ),
        "critical"
    );

    EXPECT_EQ(
        message.attributes.at(
            "device_id"
        ),
        "pump-1"
    );
}

TEST(NotificationDeliveryAlarmIntegrationTests, MapsSeverityToDeliveryPriority)
{
    EXPECT_EQ(
        dispatcher::notification::delivery::AlarmNotificationDeliveryMessageBuilder::priority_for_severity(
            dispatcher::notification::delivery::AlarmNotificationSeverity::info
        ),
        dispatcher::notification::delivery::NotificationDeliveryPriority::low
    );

    EXPECT_EQ(
        dispatcher::notification::delivery::AlarmNotificationDeliveryMessageBuilder::priority_for_severity(
            dispatcher::notification::delivery::AlarmNotificationSeverity::warning
        ),
        dispatcher::notification::delivery::NotificationDeliveryPriority::normal
    );

    EXPECT_EQ(
        dispatcher::notification::delivery::AlarmNotificationDeliveryMessageBuilder::priority_for_severity(
            dispatcher::notification::delivery::AlarmNotificationSeverity::major
        ),
        dispatcher::notification::delivery::NotificationDeliveryPriority::high
    );

    EXPECT_EQ(
        dispatcher::notification::delivery::AlarmNotificationDeliveryMessageBuilder::priority_for_severity(
            dispatcher::notification::delivery::AlarmNotificationSeverity::critical
        ),
        dispatcher::notification::delivery::NotificationDeliveryPriority::critical
    );
}

TEST(NotificationDeliveryAlarmIntegrationTests, ConvertsSeverityToString)
{
    EXPECT_EQ(
        dispatcher::notification::delivery::AlarmNotificationDeliveryMessageBuilder::severity_to_string(
            dispatcher::notification::delivery::AlarmNotificationSeverity::info
        ),
        "info"
    );

    EXPECT_EQ(
        dispatcher::notification::delivery::AlarmNotificationDeliveryMessageBuilder::severity_to_string(
            dispatcher::notification::delivery::AlarmNotificationSeverity::warning
        ),
        "warning"
    );

    EXPECT_EQ(
        dispatcher::notification::delivery::AlarmNotificationDeliveryMessageBuilder::severity_to_string(
            dispatcher::notification::delivery::AlarmNotificationSeverity::major
        ),
        "major"
    );

    EXPECT_EQ(
        dispatcher::notification::delivery::AlarmNotificationDeliveryMessageBuilder::severity_to_string(
            dispatcher::notification::delivery::AlarmNotificationSeverity::critical
        ),
        "critical"
    );
}

TEST(NotificationDeliveryAlarmIntegrationTests, RejectsMissingNotificationId)
{
    auto request =
        make_alarm_request();

    request.notification_id = "";

    expect_build_message_throws(
        request
    );
}

TEST(NotificationDeliveryAlarmIntegrationTests, RejectsMissingAlarmId)
{
    auto request =
        make_alarm_request();

    request.alarm_id = "";

    expect_build_message_throws(
        request
    );
}

TEST(NotificationDeliveryAlarmIntegrationTests, RejectsMissingTagId)
{
    auto request =
        make_alarm_request();

    request.tag_id = "";

    expect_build_message_throws(
        request
    );
}

TEST(NotificationDeliveryAlarmIntegrationTests, RejectsMissingAlarmName)
{
    auto request =
        make_alarm_request();

    request.alarm_name = "";

    expect_build_message_throws(
        request
    );
}

TEST(NotificationDeliveryAlarmIntegrationTests, RejectsMissingAlarmState)
{
    auto request =
        make_alarm_request();

    request.alarm_state = "";

    expect_build_message_throws(
        request
    );
}

TEST(NotificationDeliveryAlarmIntegrationTests, RejectsMissingRecipientAddress)
{
    auto request =
        make_alarm_request();

    request.recipient.address = "";

    expect_build_message_throws(
        request
    );
}

TEST(NotificationDeliveryAlarmIntegrationTests, DeliversAlarmNotificationThroughInMemoryChannel)
{
    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel channel;

    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher.register_channel(
        channel
    );

    const auto result =
        dispatcher::notification::delivery::AlarmNotificationDeliveryMessageBuilder::deliver(
            dispatcher,
            make_alarm_request()
        );

    EXPECT_TRUE(
        result.success()
    );

    ASSERT_EQ(
        channel.delivered_messages().size(),
        1U
    );

    EXPECT_EQ(
        channel.delivered_messages()[0].message_id,
        "notification-1"
    );

    EXPECT_EQ(
        channel.delivered_messages()[0].attributes.at(
            "alarm_id"
        ),
        "alarm-1"
    );
}

TEST(NotificationDeliveryAlarmIntegrationTests, DeliversAlarmNotificationThroughRetryExecutor)
{
    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel channel;

    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher.register_channel(
        channel
    );

    dispatcher::notification::delivery::NotificationDeliveryRetryExecutor executor{
        dispatcher
    };

    const auto result =
        dispatcher::notification::delivery::AlarmNotificationDeliveryMessageBuilder::deliver_with_retry(
            executor,
            make_alarm_request()
        );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_EQ(
        result.attempt_count(),
        1U
    );

    ASSERT_EQ(
        result.attempts.size(),
        1U
    );

    EXPECT_EQ(
        result.attempts[0].status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::delivered
    );

    ASSERT_EQ(
        channel.delivered_messages().size(),
        1U
    );
}

TEST(NotificationDeliveryAlarmIntegrationTests, AlarmNotificationThroughRetryRetriesFailure)
{
    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel channel;

    channel.set_failure(
        "provider unavailable"
    );

    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher.register_channel(
        channel
    );

    dispatcher::notification::delivery::NotificationDeliveryRetryPolicy policy;

    policy.max_attempts = 2;
    policy.retry_failed = true;

    dispatcher::notification::delivery::NotificationDeliveryRetryExecutor executor{
        dispatcher,
        policy
    };

    const auto result =
        dispatcher::notification::delivery::AlarmNotificationDeliveryMessageBuilder::deliver_with_retry(
            executor,
            make_alarm_request()
        );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_EQ(
        result.attempt_count(),
        2U
    );

    EXPECT_EQ(
        channel.delivery_attempt_count(),
        2
    );
}

TEST(NotificationDeliveryAlarmIntegrationTests, DeliversAlarmNotificationThroughFileChannel)
{
    const auto root =
        test_root(
            "delivers-through-file-channel"
        );

    clean_directory(
        root
    );

    dispatcher::notification::delivery::FileNotificationDeliveryOptions options;

    options.directory =
        root;

    options.file_name =
        "alarm-notifications.log";

    options.create_directories = true;
    options.append = true;

    dispatcher::notification::delivery::FileNotificationDeliveryChannel channel{
        options
    };

    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher.register_channel(
        channel
    );

    const auto result =
        dispatcher::notification::delivery::AlarmNotificationDeliveryMessageBuilder::deliver(
            dispatcher,
            make_alarm_request(
                dispatcher::notification::delivery::NotificationDeliveryChannelType::file
            )
        );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_TRUE(
        std::filesystem::exists(
            channel.output_path()
        )
    );

    const auto content =
        read_text_file(
            channel.output_path()
        );

    EXPECT_NE(
        content.find(
            "message_id=notification-1"
        ),
        std::string::npos
    );

    EXPECT_NE(
        content.find(
            "attribute.alarm_id=alarm-1"
        ),
        std::string::npos
    );

    EXPECT_NE(
        content.find(
            "attribute.severity=critical"
        ),
        std::string::npos
    );

    clean_directory(
        root
    );
}

TEST(NotificationDeliveryAlarmIntegrationTests, DeliversAlarmNotificationThroughWebhookChannel)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            200,
            "{\"ok\":true}",
            "accepted"
        }
    };

    dispatcher::notification::delivery::WebhookNotificationDeliveryOptions options;

    options.url =
        "https://hooks.example.local/dispatcher";

    options.timeout_ms = 1000;
    options.include_attributes = true;

    dispatcher::notification::delivery::WebhookNotificationDeliveryChannel channel(
        options,
        client
    );

    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher.register_channel(
        channel
    );

    const auto result =
        dispatcher::notification::delivery::AlarmNotificationDeliveryMessageBuilder::deliver(
            dispatcher,
            make_alarm_request(
                dispatcher::notification::delivery::NotificationDeliveryChannelType::webhook
            )
        );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_EQ(
        client.post_count(),
        1
    );

    ASSERT_EQ(
        client.requests.size(),
        1U
    );

    EXPECT_NE(
        client.requests[0].body.find(
            "\"message_id\": \"notification-1\""
        ),
        std::string::npos
    );

    EXPECT_NE(
        client.requests[0].body.find(
            "\"alarm_id\": \"alarm-1\""
        ),
        std::string::npos
    );

    EXPECT_NE(
        client.requests[0].body.find(
            "\"severity\": \"critical\""
        ),
        std::string::npos
    );
}

TEST(NotificationDeliveryAlarmIntegrationTests, WebhookFailureIsRetriedForAlarmNotification)
{
    ScriptedWebhookHttpClient client{
        dispatcher::notification::delivery::WebhookHttpResponse{
            500,
            "server error",
            "failed"
        }
    };

    dispatcher::notification::delivery::WebhookNotificationDeliveryOptions options;

    options.url =
        "https://hooks.example.local/dispatcher";

    options.timeout_ms = 1000;

    dispatcher::notification::delivery::WebhookNotificationDeliveryChannel channel(
        options,
        client
    );

    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher.register_channel(
        channel
    );

    dispatcher::notification::delivery::NotificationDeliveryRetryPolicy policy;

    policy.max_attempts = 2;
    policy.retry_failed = true;

    dispatcher::notification::delivery::NotificationDeliveryRetryExecutor executor{
        dispatcher,
        policy
    };

    const auto result =
        dispatcher::notification::delivery::AlarmNotificationDeliveryMessageBuilder::deliver_with_retry(
            executor,
            make_alarm_request(
                dispatcher::notification::delivery::NotificationDeliveryChannelType::webhook
            )
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