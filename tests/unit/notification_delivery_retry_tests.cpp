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

    void expect_policy_validation_throws(
        const dispatcher::notification::delivery::NotificationDeliveryRetryPolicy& policy
    )
    {
        EXPECT_THROW(
            dispatcher::notification::delivery::NotificationDeliveryRetryExecutor::validate_policy(
                policy
            ),
            dispatcher::notification::delivery::NotificationDeliveryError
        );
    }

    void expect_executor_construction_throws(
        dispatcher::notification::delivery::NotificationDeliveryDispatcher& dispatcher,
        const dispatcher::notification::delivery::NotificationDeliveryRetryPolicy& policy
    )
    {
        EXPECT_THROW(
            {
                dispatcher::notification::delivery::NotificationDeliveryRetryExecutor executor(
                    dispatcher,
                    policy
                );

                static_cast<void>(
                    executor
                );
            },
            dispatcher::notification::delivery::NotificationDeliveryError
        );
    }

    void expect_deliver_with_retry_throws(
        dispatcher::notification::delivery::NotificationDeliveryRetryExecutor& executor,
        const dispatcher::notification::delivery::NotificationDeliveryMessage& message
    )
    {
        EXPECT_THROW(
            {
                const auto result =
                    executor.deliver_with_retry(
                        message
                    );

                static_cast<void>(
                    result
                );
            },
            dispatcher::notification::delivery::NotificationDeliveryError
        );
    }

    class FlakyDeliveryChannel final
        : public dispatcher::notification::delivery::INotificationDeliveryChannel
    {
    public:
        explicit FlakyDeliveryChannel(
            int failures_before_success
        )
            : failures_before_success_(
                failures_before_success
            )
        {
        }

        [[nodiscard]] dispatcher::notification::delivery::NotificationDeliveryChannelType channel_type() const noexcept override
        {
            return dispatcher::notification::delivery::NotificationDeliveryChannelType::test;
        }

        [[nodiscard]] std::string channel_name() const override
        {
            return "flaky-channel";
        }

        [[nodiscard]] dispatcher::notification::delivery::NotificationDeliveryResult deliver(
            const dispatcher::notification::delivery::NotificationDeliveryMessage& message
        ) override
        {
            dispatcher::notification::delivery::NotificationDeliveryValidator::validate_message(
                message
            );

            ++attempt_count_;

            if (attempt_count_ <= failures_before_success_)
            {
                return dispatcher::notification::delivery::NotificationDeliveryResult::failed(
                    "temporary failure",
                    "flaky failure"
                );
            }

            return dispatcher::notification::delivery::NotificationDeliveryResult::delivered(
                "flaky:" + message.message_id,
                "flaky delivered"
            );
        }

        [[nodiscard]] int attempt_count() const noexcept
        {
            return attempt_count_;
        }

    private:
        int failures_before_success_{ 0 };
        int attempt_count_{ 0 };
    };

    class SkippingDeliveryChannel final
        : public dispatcher::notification::delivery::INotificationDeliveryChannel
    {
    public:
        [[nodiscard]] dispatcher::notification::delivery::NotificationDeliveryChannelType channel_type() const noexcept override
        {
            return dispatcher::notification::delivery::NotificationDeliveryChannelType::test;
        }

        [[nodiscard]] std::string channel_name() const override
        {
            return "skipping-channel";
        }

        [[nodiscard]] dispatcher::notification::delivery::NotificationDeliveryResult deliver(
            const dispatcher::notification::delivery::NotificationDeliveryMessage&
        ) override
        {
            ++attempt_count_;

            return dispatcher::notification::delivery::NotificationDeliveryResult::skipped(
                "recipient muted"
            );
        }

        [[nodiscard]] int attempt_count() const noexcept
        {
            return attempt_count_;
        }

    private:
        int attempt_count_{ 0 };
    };
}

TEST(NotificationDeliveryRetryTests, ValidatesDefaultPolicy)
{
    const dispatcher::notification::delivery::NotificationDeliveryRetryPolicy policy;

    EXPECT_NO_THROW(
        dispatcher::notification::delivery::NotificationDeliveryRetryExecutor::validate_policy(
            policy
        )
    );

    EXPECT_EQ(
        policy.max_attempts,
        3U
    );

    EXPECT_TRUE(
        policy.retry_failed
    );

    EXPECT_FALSE(
        policy.retry_skipped
    );
}

TEST(NotificationDeliveryRetryTests, RejectsZeroMaxAttempts)
{
    dispatcher::notification::delivery::NotificationDeliveryRetryPolicy policy;

    policy.max_attempts = 0;

    expect_policy_validation_throws(
        policy
    );
}

TEST(NotificationDeliveryRetryTests, RejectsTooLargeMaxAttempts)
{
    dispatcher::notification::delivery::NotificationDeliveryRetryPolicy policy;

    policy.max_attempts = 11;

    expect_policy_validation_throws(
        policy
    );
}

TEST(NotificationDeliveryRetryTests, ConstructorValidatesPolicy)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher::notification::delivery::NotificationDeliveryRetryPolicy policy;

    policy.max_attempts = 0;

    expect_executor_construction_throws(
        dispatcher,
        policy
    );
}

TEST(NotificationDeliveryRetryTests, DeliversOnceWhenFirstAttemptSucceeds)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel channel;

    dispatcher.register_channel(
        channel
    );

    dispatcher::notification::delivery::NotificationDeliveryRetryExecutor executor{
        dispatcher
    };

    const auto result =
        executor.deliver_with_retry(
            make_message()
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
        result.attempts[0].attempt_number,
        1U
    );

    EXPECT_EQ(
        result.attempts[0].status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::delivered
    );

    EXPECT_EQ(
        channel.delivery_attempt_count(),
        1
    );
}

TEST(NotificationDeliveryRetryTests, RetriesFailedDeliveryUntilSuccess)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    FlakyDeliveryChannel channel{
        2
    };

    dispatcher.register_channel(
        channel
    );

    dispatcher::notification::delivery::NotificationDeliveryRetryPolicy policy;

    policy.max_attempts = 3;
    policy.retry_failed = true;

    dispatcher::notification::delivery::NotificationDeliveryRetryExecutor executor{
        dispatcher,
        policy
    };

    const auto result =
        executor.deliver_with_retry(
            make_message()
        );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_EQ(
        result.attempt_count(),
        3U
    );

    ASSERT_EQ(
        result.attempts.size(),
        3U
    );

    EXPECT_EQ(
        result.attempts[0].status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::failed
    );

    EXPECT_EQ(
        result.attempts[1].status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::failed
    );

    EXPECT_EQ(
        result.attempts[2].status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::delivered
    );

    EXPECT_EQ(
        result.final_result.provider_message_id,
        "flaky:message-1"
    );

    EXPECT_EQ(
        channel.attempt_count(),
        3
    );
}

TEST(NotificationDeliveryRetryTests, StopsAfterMaxAttempts)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    FlakyDeliveryChannel channel{
        99
    };

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
        executor.deliver_with_retry(
            make_message()
        );

    EXPECT_FALSE(
        result.success()
    );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_EQ(
        result.attempt_count(),
        2U
    );

    ASSERT_EQ(
        result.attempts.size(),
        2U
    );

    EXPECT_EQ(
        result.attempts[0].attempt_number,
        1U
    );

    EXPECT_EQ(
        result.attempts[1].attempt_number,
        2U
    );

    EXPECT_EQ(
        result.final_result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::failed
    );

    EXPECT_EQ(
        result.final_result.error_message,
        "temporary failure"
    );

    EXPECT_EQ(
        channel.attempt_count(),
        2
    );
}

TEST(NotificationDeliveryRetryTests, DoesNotRetryFailedWhenPolicyDisablesIt)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    FlakyDeliveryChannel channel{
        99
    };

    dispatcher.register_channel(
        channel
    );

    dispatcher::notification::delivery::NotificationDeliveryRetryPolicy policy;

    policy.max_attempts = 3;
    policy.retry_failed = false;

    dispatcher::notification::delivery::NotificationDeliveryRetryExecutor executor{
        dispatcher,
        policy
    };

    const auto result =
        executor.deliver_with_retry(
            make_message()
        );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_EQ(
        result.attempt_count(),
        1U
    );

    EXPECT_EQ(
        channel.attempt_count(),
        1
    );
}

TEST(NotificationDeliveryRetryTests, DoesNotRetrySkippedByDefault)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    SkippingDeliveryChannel channel;

    dispatcher.register_channel(
        channel
    );

    dispatcher::notification::delivery::NotificationDeliveryRetryExecutor executor{
        dispatcher
    };

    const auto result =
        executor.deliver_with_retry(
            make_message()
        );

    EXPECT_EQ(
        result.final_result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::skipped
    );

    EXPECT_EQ(
        result.attempt_count(),
        1U
    );

    EXPECT_EQ(
        channel.attempt_count(),
        1
    );
}

TEST(NotificationDeliveryRetryTests, RetriesSkippedWhenPolicyEnablesIt)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    SkippingDeliveryChannel channel;

    dispatcher.register_channel(
        channel
    );

    dispatcher::notification::delivery::NotificationDeliveryRetryPolicy policy;

    policy.max_attempts = 3;
    policy.retry_skipped = true;

    dispatcher::notification::delivery::NotificationDeliveryRetryExecutor executor{
        dispatcher,
        policy
    };

    const auto result =
        executor.deliver_with_retry(
            make_message()
        );

    EXPECT_EQ(
        result.final_result.status,
        dispatcher::notification::delivery::NotificationDeliveryStatus::skipped
    );

    EXPECT_EQ(
        result.attempt_count(),
        3U
    );

    EXPECT_EQ(
        channel.attempt_count(),
        3
    );
}

TEST(NotificationDeliveryRetryTests, MissingChannelIsRetriedAsFailedResult)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher::notification::delivery::NotificationDeliveryRetryPolicy policy;

    policy.max_attempts = 2;
    policy.retry_failed = true;

    dispatcher::notification::delivery::NotificationDeliveryRetryExecutor executor{
        dispatcher,
        policy
    };

    const auto result =
        executor.deliver_with_retry(
            make_message()
        );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_EQ(
        result.attempt_count(),
        2U
    );

    EXPECT_NE(
        result.final_result.error_message.find(
            "No notification delivery channel registered"
        ),
        std::string::npos
    );
}

TEST(NotificationDeliveryRetryTests, InvalidMessageThrowsBeforeAttempts)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel channel;

    dispatcher.register_channel(
        channel
    );

    dispatcher::notification::delivery::NotificationDeliveryRetryExecutor executor{
        dispatcher
    };

    auto message =
        make_message();

    message.body = "";

    expect_deliver_with_retry_throws(
        executor,
        message
    );

    EXPECT_EQ(
        channel.delivery_attempt_count(),
        0
    );
}

TEST(NotificationDeliveryRetryTests, BatchDeliveryWithRetryPreservesOrder)
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

    dispatcher::notification::delivery::NotificationDeliveryRetryPolicy policy;

    policy.max_attempts = 2;

    dispatcher::notification::delivery::NotificationDeliveryRetryExecutor executor{
        dispatcher,
        policy
    };

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
        executor.deliver_batch_with_retry(
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

    EXPECT_EQ(
        results[0].attempt_count(),
        1U
    );

    EXPECT_EQ(
        results[1].attempt_count(),
        1U
    );

    EXPECT_EQ(
        results[2].attempt_count(),
        2U
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

TEST(NotificationDeliveryRetryTests, AttemptCopiesProviderAndDiagnostics)
{
    dispatcher::notification::delivery::NotificationDeliveryDispatcher dispatcher;

    dispatcher::notification::delivery::InMemoryNotificationDeliveryChannel channel{
        dispatcher::notification::delivery::NotificationDeliveryChannelType::test,
        "memory-test"
    };

    dispatcher.register_channel(
        channel
    );

    dispatcher::notification::delivery::NotificationDeliveryRetryExecutor executor{
        dispatcher
    };

    const auto result =
        executor.deliver_with_retry(
            make_message()
        );

    ASSERT_EQ(
        result.attempts.size(),
        1U
    );

    EXPECT_EQ(
        result.attempts[0].provider_message_id,
        "memory-test:message-1"
    );

    EXPECT_EQ(
        result.attempts[0].diagnostic_message,
        "Delivered by in-memory notification delivery channel."
    );

    EXPECT_TRUE(
        result.attempts[0].error_message.empty()
    );
}