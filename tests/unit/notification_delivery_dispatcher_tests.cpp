#include <dispatcher/notification/delivery/notification_delivery.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace
{
    dispatcher::notification::delivery::NotificationDeliveryMessage make_message(
        std::string message_id = "message-1",
        dispatcher::notification::delivery::NotificationDeliveryChannelType channel_type =
        dispatcher::notification::delivery::NotificationDeliveryChannelType::test
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
            channel_type;
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

        return message;
    }

    void expect_register_channel_throws(
        dispatcher::notification::delivery::NotificationDeliveryDispatcher& dispatcher,
        dispatcher::notification::delivery::INotificationDeliveryChannel& channel
    )
    {
        EXPECT_THROW(
            dispatcher.register_channel(
                channel
            ),
            dispatcher::notification::delivery::NotificationDeliveryError
        );
    }

    void expect_dispatch_throws(
        dispatcher::notification::delivery::NotificationDeliveryDispatcher& dispatcher,
        const dispatcher::notification::delivery::NotificationDeliveryMessage& message
    )
    {
        EXPECT_THROW(
            {
                const auto result =
                    dispatcher.deliver(
                        message
                    );

                static_cast<void>(
                    result
                );
            },
            dispatcher::notification::delivery::NotificationDeliveryError
        );
    }

    class EmptyNameChannel final
        : public dispatcher::notification::delivery::INotificationDeliveryChannel
    {
    public:
        [[nodiscard]] dispatcher::notification::delivery::NotificationDeliveryChannelType channel_type() const noexcept override
        {
            return dispatcher::notification::delivery::NotificationDeliveryChannelType::test;
        }

        [[nodiscard]] std::string channel_name() const override
        {
            return "";
        }

        [[nodiscard]] dispatcher::notification::delivery::NotificationDeliveryResult deliver(
            const dispatcher::notification::delivery::NotificationDeliveryMessage&
        ) override
        {
            return dispatcher::notification::delivery::NotificationDeliveryResult::delivered();
        }
    };

    class ThrowingChannel final
        : public dispatcher::notification::delivery::INotificationDeliveryChannel
    {
    public:
        [[nodiscard]] dispatcher::notification::delivery::NotificationDeliveryChannelType channel_type() const noexcept override
        {
            return dispatcher::notification::delivery::NotificationDeliveryChannelType::test;
        }

        [[nodiscard]] std::string channel_name() const override
        {
            return "throwing-channel";
        }

        [[nodiscard]] dispatcher::notification::delivery::NotificationDeliveryResult deliver(
            const dispatcher::notification::delivery::NotificationDeliveryMessage&
        ) override
        {
            throw dispatcher::notification::delivery::NotificationDeliveryError(
                "channel failed unexpectedly"
            );
        }
    };

    class InvalidResultChannel final
        : public dispatcher::notification::delivery::INotificationDeliveryChannel
    {
    public:
        [[nodiscard]] dispatcher::notification::delivery::NotificationDeliveryChannelType channel_type() const noexcept override
        {
            return dispatcher::notification::delivery::NotificationDeliveryChannelType::test;
        }

        [[nodiscard]] std::string channel_name() const override
        {
            return "invalid-result-channel";
        }

        [[nodiscard]] dispatcher::notification::delivery::NotificationDeliveryResult deliver(
            const dispatcher::notification::delivery::NotificationDeliveryMessage&
        ) override
        {
            auto result =
                dispatcher::notification::delivery::NotificationDeliveryResult::delivered(
                    "provider-message"
                );

            result.error_message =
                "invalid error on delivered result";

            return result;
        }
    };
}

TEST(NotificationDeliveryDispatcherTests, InMemoryChannelDeliversAndRecordsMessage)
{
    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel channel;

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
        result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::delivered
    );

    EXPECT_EQ(
        result.provider_message_id,
        "in-memory-notification-delivery:message-1"
    );

    EXPECT_EQ(
        channel.delivery_attempt_count(),
        1
    );

    ASSERT_EQ(
        channel.delivered_messages().size(),
        1U
    );

    EXPECT_EQ(
        channel.delivered_messages()[0].message_id,
        "message-1"
    );
}

TEST(NotificationDeliveryDispatcherTests, InMemoryChannelReturnsSkippedForWrongChannelType)
{
    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel channel{
        dispatcher::notification::delivery::NotificationDeliveryChannelType::email,
        "memory-email"
    };

    const auto message =
        make_message(
            "message-1",
            dispatcher::notification::delivery::NotificationDeliveryChannelType::test
        );

    const auto result =
        channel.deliver(
            message
        );

    EXPECT_EQ(
        result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::skipped
    );

    EXPECT_FALSE(
        result.success()
    );

    EXPECT_TRUE(
        channel.delivered_messages().empty()
    );

    EXPECT_EQ(
        channel.delivery_attempt_count(),
        1
    );
}

TEST(NotificationDeliveryDispatcherTests, InMemoryChannelCanBeForcedToFail)
{
    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel channel;

    channel.set_failure(
        "provider unavailable"
    );

    const auto result =
        channel.deliver(
            make_message()
        );

    EXPECT_EQ(
        result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::failed
    );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_EQ(
        result.error_message,
        "provider unavailable"
    );

    EXPECT_TRUE(
        channel.delivered_messages().empty()
    );
}

TEST(NotificationDeliveryDispatcherTests, InMemoryChannelClearResetsState)
{
    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel channel;

    const auto first_result =
        channel.deliver(
            make_message()
        );

    static_cast<void>(
        first_result
        );

    channel.set_failure(
        "provider unavailable"
    );

    channel.clear();

    EXPECT_EQ(
        channel.delivery_attempt_count(),
        0
    );

    EXPECT_TRUE(
        channel.delivered_messages().empty()
    );

    const auto second_result =
        channel.deliver(
            make_message(
                "message-2"
            )
        );

    EXPECT_TRUE(
        second_result.success()
    );

    ASSERT_EQ(
        channel.delivered_messages().size(),
        1U
    );

    EXPECT_EQ(
        channel.delivered_messages()[0].message_id,
        "message-2"
    );
}

TEST(NotificationDeliveryDispatcherTests, DispatcherRegistersChannel)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel channel;

    EXPECT_FALSE(
        dispatcher.has_channel(
            dispatcher::notification::delivery::NotificationDeliveryChannelType::test
        )
    );

    dispatcher.register_channel(
        channel
    );

    EXPECT_TRUE(
        dispatcher.has_channel(
            dispatcher::notification::delivery::NotificationDeliveryChannelType::test
        )
    );

    EXPECT_EQ(
        dispatcher.channel_count(),
        1U
    );
}

TEST(NotificationDeliveryDispatcherTests, DispatcherRejectsDuplicateChannelType)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel first{
        dispatcher::notification::delivery::NotificationDeliveryChannelType::test,
        "first"
    };

    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel second{
        dispatcher::notification::delivery::NotificationDeliveryChannelType::test,
        "second"
    };

    dispatcher.register_channel(
        first
    );

    expect_register_channel_throws(
        dispatcher,
        second
    );
}

TEST(NotificationDeliveryDispatcherTests, DispatcherRejectsEmptyChannelName)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    EmptyNameChannel channel;

    expect_register_channel_throws(
        dispatcher,
        channel
    );
}

TEST(NotificationDeliveryDispatcherTests, DispatcherClearChannelsRemovesRegistrations)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel channel;

    dispatcher.register_channel(
        channel
    );

    ASSERT_EQ(
        dispatcher.channel_count(),
        1U
    );

    dispatcher.clear_channels();

    EXPECT_EQ(
        dispatcher.channel_count(),
        0U
    );

    EXPECT_FALSE(
        dispatcher.has_channel(
            dispatcher::notification::delivery::NotificationDeliveryChannelType::test
        )
    );
}

TEST(NotificationDeliveryDispatcherTests, DispatcherDeliversToRegisteredChannel)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel channel;

    dispatcher.register_channel(
        channel
    );

    const auto result =
        dispatcher.deliver(
            make_message()
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
        "message-1"
    );
}

TEST(NotificationDeliveryDispatcherTests, DispatcherReturnsFailedForMissingChannel)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    const auto result =
        dispatcher.deliver(
            make_message(
                "message-1",
                dispatcher::notification::delivery::NotificationDeliveryChannelType::webhook
            )
        );

    EXPECT_EQ(
        result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::failed
    );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_NE(
        result.error_message.find(
            "No notification delivery channel registered"
        ),
        std::string::npos
    );
}

TEST(NotificationDeliveryDispatcherTests, DispatcherValidatesMessageBeforeDelivery)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel channel;

    dispatcher.register_channel(
        channel
    );

    auto message =
        make_message();

    message.body = "";

    expect_dispatch_throws(
        dispatcher,
        message
    );

    EXPECT_TRUE(
        channel.delivered_messages().empty()
    );
}

TEST(NotificationDeliveryDispatcherTests, DispatcherConvertsChannelExceptionToFailedResult)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    ThrowingChannel channel;

    dispatcher.register_channel(
        channel
    );

    const auto result =
        dispatcher.deliver(
            make_message()
        );

    EXPECT_EQ(
        result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::failed
    );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_EQ(
        result.error_message,
        "channel failed unexpectedly"
    );
}

TEST(NotificationDeliveryDispatcherTests, DispatcherConvertsInvalidChannelResultToFailedResult)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    InvalidResultChannel channel;

    dispatcher.register_channel(
        channel
    );

    const auto result =
        dispatcher.deliver(
            make_message()
        );

    EXPECT_EQ(
        result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::failed
    );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_NE(
        result.error_message.find(
            "Delivered notification delivery result must not contain error_message"
        ),
        std::string::npos
    );
}

TEST(NotificationDeliveryDispatcherTests, DispatcherDeliversBatchInOrder)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel test_channel{
        dispatcher::notification::delivery::NotificationDeliveryChannelType::test,
        "memory-test"
    };

    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel file_channel{
        dispatcher::notification::delivery::NotificationDeliveryChannelType::file,
        "memory-file"
    };

    dispatcher.register_channel(
        test_channel
    );

    dispatcher.register_channel(
        file_channel
    );

    const std::vector<dispatcher::notification::delivery::NotificationDeliveryMessage> messages{
        make_message(
            "message-1",
            dispatcher::notification::delivery::NotificationDeliveryChannelType::test
        ),
        make_message(
            "message-2",
            dispatcher::notification::delivery::NotificationDeliveryChannelType::file
        ),
        make_message(
            "message-3",
            dispatcher::notification::delivery::NotificationDeliveryChannelType::webhook
        )
    };

    const auto results =
        dispatcher.deliver_batch(
            messages
        );

    ASSERT_EQ(
        results.size(),
        3U
    );

    EXPECT_TRUE(
        results[0].success()
    );

    EXPECT_TRUE(
        results[1].success()
    );

    EXPECT_TRUE(
        results[2].failure()
    );

    ASSERT_EQ(
        test_channel.delivered_messages().size(),
        1U
    );

    ASSERT_EQ(
        file_channel.delivered_messages().size(),
        1U
    );

    EXPECT_EQ(
        test_channel.delivered_messages()[0].message_id,
        "message-1"
    );

    EXPECT_EQ(
        file_channel.delivered_messages()[0].message_id,
        "message-2"
    );
}

TEST(NotificationDeliveryDispatcherTests, DispatcherBatchStopsOnInvalidMessageByThrowing)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel channel;

    dispatcher.register_channel(
        channel
    );

    auto valid =
        make_message(
            "message-1"
        );

    auto invalid =
        make_message(
            "message-2"
        );

    invalid.subject = "";

    const std::vector<dispatcher::notification::delivery::NotificationDeliveryMessage> messages{
        valid,
        invalid
    };

    EXPECT_THROW(
        {
            const auto results =
                dispatcher.deliver_batch(
                    messages
                );

            static_cast<void>(
                results
            );
        },
        dispatcher::notification::delivery::NotificationDeliveryError
    );

    ASSERT_EQ(
        channel.delivered_messages().size(),
        1U
    );

    EXPECT_EQ(
        channel.delivered_messages()[0].message_id,
        "message-1"
    );
}