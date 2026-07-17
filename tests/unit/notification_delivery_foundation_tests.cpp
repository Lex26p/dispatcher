#include <dispatcher/notification/delivery/notification_delivery.hpp>

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace
{
    dispatcher::notification::delivery::NotificationDeliveryMessage make_message()
    {
        dispatcher::notification::delivery::NotificationDeliveryMessage message;

        message.message_id = "message-1";
        message.correlation_id = "alarm-1";
        message.source = "alarm-routing";
        message.channel_type =
            dispatcher::notification::delivery::NotificationDeliveryChannelType::test;
        message.priority =
            dispatcher::notification::delivery::NotificationDeliveryPriority::high;

        message.recipient.recipient_id = "operator-1";
        message.recipient.display_name = "Operator One";
        message.recipient.address = "operator-1";

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

    void expect_validate_message_throws(
        const dispatcher::notification::delivery::NotificationDeliveryMessage& message
    )
    {
        EXPECT_THROW(
            dispatcher::notification::delivery::NotificationDeliveryValidator::validate_message(
                message
            ),
            dispatcher::notification::delivery::NotificationDeliveryError
        );
    }

    void expect_validate_result_throws(
        const dispatcher::notification::delivery::NotificationDeliveryResult& result
    )
    {
        EXPECT_THROW(
            dispatcher::notification::delivery::NotificationDeliveryValidator::validate_result(
                result
            ),
            dispatcher::notification::delivery::NotificationDeliveryError
        );
    }

    class RecordingDeliveryChannel final
        : public dispatcher::notification::delivery::INotificationDeliveryChannel
    {
    public:
        [[nodiscard]] dispatcher::notification::delivery::NotificationDeliveryChannelType channel_type() const noexcept override
        {
            return dispatcher::notification::delivery::NotificationDeliveryChannelType::test;
        }

        [[nodiscard]] std::string channel_name() const override
        {
            return "recording-test-channel";
        }

        [[nodiscard]] dispatcher::notification::delivery::NotificationDeliveryResult deliver(
            const dispatcher::notification::delivery::NotificationDeliveryMessage& message
        ) override
        {
            dispatcher::notification::delivery::NotificationDeliveryValidator::validate_message(
                message
            );

            delivered_messages.push_back(
                message
            );

            return dispatcher::notification::delivery::NotificationDeliveryResult::delivered(
                "provider-message-1",
                "recorded"
            );
        }

        std::vector<dispatcher::notification::delivery::NotificationDeliveryMessage> delivered_messages{};
    };
}

TEST(NotificationDeliveryFoundationTests, ValidatesGoodMessage)
{
    const auto message =
        make_message();

    EXPECT_NO_THROW(
        dispatcher::notification::delivery::NotificationDeliveryValidator::validate_message(
            message
        )
    );
}

TEST(NotificationDeliveryFoundationTests, RejectsEmptyMessageId)
{
    auto message =
        make_message();

    message.message_id = "";

    expect_validate_message_throws(
        message
    );
}

TEST(NotificationDeliveryFoundationTests, RejectsEmptySource)
{
    auto message =
        make_message();

    message.source = "";

    expect_validate_message_throws(
        message
    );
}

TEST(NotificationDeliveryFoundationTests, RejectsInvalidChannelType)
{
    auto message =
        make_message();

    message.channel_type =
        static_cast<dispatcher::notification::delivery::NotificationDeliveryChannelType>(
            999
            );

    expect_validate_message_throws(
        message
    );
}

TEST(NotificationDeliveryFoundationTests, RejectsInvalidPriority)
{
    auto message =
        make_message();

    message.priority =
        static_cast<dispatcher::notification::delivery::NotificationDeliveryPriority>(
            999
            );

    expect_validate_message_throws(
        message
    );
}

TEST(NotificationDeliveryFoundationTests, RejectsEmptyRecipientAddress)
{
    auto message =
        make_message();

    message.recipient.address = "";

    expect_validate_message_throws(
        message
    );
}

TEST(NotificationDeliveryFoundationTests, RejectsEmptySubject)
{
    auto message =
        make_message();

    message.subject = "";

    expect_validate_message_throws(
        message
    );
}

TEST(NotificationDeliveryFoundationTests, RejectsEmptyBody)
{
    auto message =
        make_message();

    message.body = "";

    expect_validate_message_throws(
        message
    );
}

TEST(NotificationDeliveryFoundationTests, CreatesDeliveredResult)
{
    const auto result =
        dispatcher::notification::delivery::NotificationDeliveryResult::delivered(
            "provider-1",
            "accepted"
        );

    EXPECT_EQ(
        result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::delivered
    );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_FALSE(
        result.failure()
    );

    EXPECT_EQ(
        result.provider_message_id,
        "provider-1"
    );

    EXPECT_EQ(
        result.diagnostic_message,
        "accepted"
    );

    EXPECT_NO_THROW(
        dispatcher::notification::delivery::NotificationDeliveryValidator::validate_result(
            result
        )
    );
}

TEST(NotificationDeliveryFoundationTests, CreatesFailedResult)
{
    const auto result =
        dispatcher::notification::delivery::NotificationDeliveryResult::failed(
            "network unavailable",
            "smtp connect failed"
        );

    EXPECT_EQ(
        result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::failed
    );

    EXPECT_FALSE(
        result.success()
    );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_EQ(
        result.error_message,
        "network unavailable"
    );

    EXPECT_NO_THROW(
        dispatcher::notification::delivery::NotificationDeliveryValidator::validate_result(
            result
        )
    );
}

TEST(NotificationDeliveryFoundationTests, CreatesSkippedResult)
{
    const auto result =
        dispatcher::notification::delivery::NotificationDeliveryResult::skipped(
            "recipient muted"
        );

    EXPECT_EQ(
        result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::skipped
    );

    EXPECT_FALSE(
        result.success()
    );

    EXPECT_FALSE(
        result.failure()
    );

    EXPECT_EQ(
        result.diagnostic_message,
        "recipient muted"
    );

    EXPECT_NO_THROW(
        dispatcher::notification::delivery::NotificationDeliveryValidator::validate_result(
            result
        )
    );
}

TEST(NotificationDeliveryFoundationTests, RejectsFailedResultWithoutErrorMessage)
{
    dispatcher::notification::delivery::NotificationDeliveryResult result;

    result.status =
        dispatcher::notification::delivery::NotificationDeliveryStatus::failed;

    result.error_message = "";

    expect_validate_result_throws(
        result
    );
}

TEST(NotificationDeliveryFoundationTests, RejectsDeliveredResultWithErrorMessage)
{
    auto result =
        dispatcher::notification::delivery::NotificationDeliveryResult::delivered();

    result.error_message = "unexpected error";

    expect_validate_result_throws(
        result
    );
}

TEST(NotificationDeliveryFoundationTests, RejectsSkippedResultWithoutDiagnosticMessage)
{
    dispatcher::notification::delivery::NotificationDeliveryResult result;

    result.status =
        dispatcher::notification::delivery::NotificationDeliveryStatus::skipped;

    result.diagnostic_message = "";

    expect_validate_result_throws(
        result
    );
}

TEST(NotificationDeliveryFoundationTests, ConvertsEnumsToStrings)
{
    EXPECT_EQ(
        dispatcher::notification::delivery::NotificationDeliveryValidator::channel_type_to_string(
            dispatcher::notification::delivery::NotificationDeliveryChannelType::email
        ),
        "email"
    );

    EXPECT_EQ(
        dispatcher::notification::delivery::NotificationDeliveryValidator::priority_to_string(
            dispatcher::notification::delivery::NotificationDeliveryPriority::critical
        ),
        "critical"
    );

    EXPECT_EQ(
        dispatcher::notification::delivery::NotificationDeliveryValidator::status_to_string(
            dispatcher::notification::delivery::NotificationDeliveryStatus::delivered
        ),
        "delivered"
    );
}

TEST(NotificationDeliveryFoundationTests, UnknownEnumValuesConvertToUnknown)
{
    EXPECT_EQ(
        dispatcher::notification::delivery::NotificationDeliveryValidator::channel_type_to_string(
            static_cast<dispatcher::notification::delivery::NotificationDeliveryChannelType>(
                999
                )
        ),
        "unknown"
    );

    EXPECT_EQ(
        dispatcher::notification::delivery::NotificationDeliveryValidator::priority_to_string(
            static_cast<dispatcher::notification::delivery::NotificationDeliveryPriority>(
                999
                )
        ),
        "unknown"
    );

    EXPECT_EQ(
        dispatcher::notification::delivery::NotificationDeliveryValidator::status_to_string(
            static_cast<dispatcher::notification::delivery::NotificationDeliveryStatus>(
                999
                )
        ),
        "unknown"
    );
}

TEST(NotificationDeliveryFoundationTests, DeliveryChannelInterfaceCanDeliver)
{
    RecordingDeliveryChannel channel;

    const auto message =
        make_message();

    const auto result =
        channel.deliver(
            message
        );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_EQ(
        channel.channel_type(),
        dispatcher::notification::delivery::NotificationDeliveryChannelType::test
    );

    EXPECT_EQ(
        channel.channel_name(),
        "recording-test-channel"
    );

    ASSERT_EQ(
        channel.delivered_messages.size(),
        1U
    );

    EXPECT_EQ(
        channel.delivered_messages[0].message_id,
        "message-1"
    );

    EXPECT_EQ(
        channel.delivered_messages[0].attributes.at(
            "alarm_id"
        ),
        "alarm-1"
    );
}

TEST(NotificationDeliveryFoundationTests, DeliveryChannelRejectsInvalidMessage)
{
    RecordingDeliveryChannel channel;

    auto message =
        make_message();

    message.body = "";

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

    EXPECT_TRUE(
        channel.delivered_messages.empty()
    );
}