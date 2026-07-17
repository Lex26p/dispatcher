#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/notification_channel.hpp>
#include <dispatcher/domain/notification_delivery_attempt.hpp>
#include <dispatcher/domain/notification_delivery_result.hpp>
#include <dispatcher/domain/notification_delivery_status.hpp>
#include <dispatcher/domain/notification_message.hpp>
#include <dispatcher/domain/notification_priority.hpp>
#include <dispatcher/domain/notification_route.hpp>
#include <dispatcher/domain/notification_target.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <utility>

namespace
{
    dispatcher::domain::NotificationMessage make_delivery_message(
        dispatcher::domain::NotificationPriority priority =
        dispatcher::domain::NotificationPriority::High
    )
    {
        return dispatcher::domain::NotificationMessage::create(
            dispatcher::domain::NotificationId{ "notification-1" },
            priority,
            "Alarm active",
            "Temperature alarm is active",
            "alarm-runtime"
        );
    }

    dispatcher::domain::NotificationRoute make_delivery_route(
        std::string route_id = "route-1",
        std::string address = "operator@example.local"
    )
    {
        return dispatcher::domain::NotificationRoute(
            dispatcher::domain::NotificationRouteId{ std::move(route_id) },
            "email route",
            dispatcher::domain::NotificationTarget(
                dispatcher::domain::NotificationChannel::Email,
                std::move(address),
                "Operator"
            ),
            dispatcher::domain::NotificationPriority::Low,
            "alarm-runtime",
            true
        );
    }
}

TEST(NotificationDeliveryStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationDeliveryStatus::Pending
        ),
        "pending"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationDeliveryStatus::Delivered
        ),
        "delivered"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationDeliveryStatus::Failed
        ),
        "failed"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationDeliveryStatus::Skipped
        ),
        "skipped"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationDeliveryStatus::RetryScheduled
        ),
        "retry_scheduled"
    );
}

TEST(NotificationDeliveryStatusTests, PredicatesWork)
{
    EXPECT_FALSE(
        dispatcher::domain::is_terminal(
            dispatcher::domain::NotificationDeliveryStatus::Pending
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_terminal(
            dispatcher::domain::NotificationDeliveryStatus::Delivered
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_terminal(
            dispatcher::domain::NotificationDeliveryStatus::Failed
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_success(
            dispatcher::domain::NotificationDeliveryStatus::Delivered
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_success(
            dispatcher::domain::NotificationDeliveryStatus::Skipped
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_failure(
            dispatcher::domain::NotificationDeliveryStatus::Failed
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_retryable(
            dispatcher::domain::NotificationDeliveryStatus::Pending
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_retryable(
            dispatcher::domain::NotificationDeliveryStatus::RetryScheduled
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_retryable(
            dispatcher::domain::NotificationDeliveryStatus::Failed
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_retryable(
            dispatcher::domain::NotificationDeliveryStatus::Delivered
        )
    );
}

TEST(NotificationDeliveryAttemptTests, PendingAttemptCapturesMessageRouteAndTarget)
{
    const auto attempt =
        dispatcher::domain::NotificationDeliveryAttempt::pending(
            make_delivery_message(),
            make_delivery_route()
        );

    EXPECT_EQ(
        attempt.message().notification_id(),
        dispatcher::domain::NotificationId{ "notification-1" }
    );

    EXPECT_EQ(
        attempt.route().route_id(),
        dispatcher::domain::NotificationRouteId{ "route-1" }
    );

    EXPECT_EQ(
        attempt.target().channel(),
        dispatcher::domain::NotificationChannel::Email
    );

    EXPECT_EQ(attempt.target().address(), "operator@example.local");

    EXPECT_EQ(
        attempt.status(),
        dispatcher::domain::NotificationDeliveryStatus::Pending
    );

    EXPECT_FALSE(attempt.has_detail());
    EXPECT_FALSE(attempt.terminal());
    EXPECT_FALSE(attempt.success());
    EXPECT_FALSE(attempt.failure());
    EXPECT_TRUE(attempt.retryable());
}

TEST(NotificationDeliveryAttemptTests, DeliveredAttemptIsSuccessfulTerminal)
{
    const auto attempt =
        dispatcher::domain::NotificationDeliveryAttempt::delivered(
            make_delivery_message(),
            make_delivery_route(),
            "sent through email adapter"
        );

    EXPECT_EQ(
        attempt.status(),
        dispatcher::domain::NotificationDeliveryStatus::Delivered
    );

    EXPECT_TRUE(attempt.has_detail());
    EXPECT_EQ(attempt.detail(), "sent through email adapter");

    EXPECT_TRUE(attempt.terminal());
    EXPECT_TRUE(attempt.success());
    EXPECT_FALSE(attempt.failure());
    EXPECT_FALSE(attempt.retryable());
}

TEST(NotificationDeliveryAttemptTests, FailedAttemptIsRetryableFailure)
{
    const auto attempt =
        dispatcher::domain::NotificationDeliveryAttempt::failed(
            make_delivery_message(),
            make_delivery_route(),
            "smtp timeout"
        );

    EXPECT_EQ(
        attempt.status(),
        dispatcher::domain::NotificationDeliveryStatus::Failed
    );

    EXPECT_TRUE(attempt.has_detail());
    EXPECT_EQ(attempt.detail(), "smtp timeout");

    EXPECT_TRUE(attempt.terminal());
    EXPECT_FALSE(attempt.success());
    EXPECT_TRUE(attempt.failure());
    EXPECT_TRUE(attempt.retryable());
}

TEST(NotificationDeliveryAttemptTests, SkippedAttemptIsSuccessfulTerminal)
{
    const auto attempt =
        dispatcher::domain::NotificationDeliveryAttempt::skipped(
            make_delivery_message(),
            make_delivery_route(),
            "target disabled"
        );

    EXPECT_EQ(
        attempt.status(),
        dispatcher::domain::NotificationDeliveryStatus::Skipped
    );

    EXPECT_TRUE(attempt.terminal());
    EXPECT_TRUE(attempt.success());
    EXPECT_FALSE(attempt.failure());
    EXPECT_FALSE(attempt.retryable());
}

TEST(NotificationDeliveryAttemptTests, ConstructorCapturesAttemptTime)
{
    const auto now =
        dispatcher::domain::NotificationDeliveryAttempt::Clock::now();

    const dispatcher::domain::NotificationDeliveryAttempt attempt(
        make_delivery_message(),
        make_delivery_route(),
        make_delivery_route().target(),
        dispatcher::domain::NotificationDeliveryStatus::RetryScheduled,
        "retry later",
        now
    );

    EXPECT_EQ(
        attempt.status(),
        dispatcher::domain::NotificationDeliveryStatus::RetryScheduled
    );

    EXPECT_EQ(attempt.detail(), "retry later");
    EXPECT_EQ(attempt.attempted_at(), now);
    EXPECT_FALSE(attempt.terminal());
    EXPECT_TRUE(attempt.retryable());
}

TEST(NotificationDeliveryResultTests, DefaultResultIsEmpty)
{
    const dispatcher::domain::NotificationDeliveryResult result;

    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(result.has_attempts());

    EXPECT_EQ(result.attempt_count(), 0);
    EXPECT_EQ(result.delivered_count(), 0);
    EXPECT_EQ(result.failed_count(), 0);
    EXPECT_EQ(result.skipped_count(), 0);
    EXPECT_EQ(result.retryable_count(), 0);

    EXPECT_FALSE(result.all_delivered());
    EXPECT_FALSE(result.any_failed());
    EXPECT_FALSE(result.any_retryable());
}

TEST(NotificationDeliveryResultTests, RecordsDeliveredAttempts)
{
    dispatcher::domain::NotificationDeliveryResult result;

    result.record(
        dispatcher::domain::NotificationDeliveryAttempt::delivered(
            make_delivery_message(),
            make_delivery_route("route-1"),
            "delivered"
        )
    );

    result.record(
        dispatcher::domain::NotificationDeliveryAttempt::delivered(
            make_delivery_message(),
            make_delivery_route("route-2"),
            "delivered"
        )
    );

    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.has_attempts());

    EXPECT_EQ(result.attempt_count(), 2);
    EXPECT_EQ(result.delivered_count(), 2);
    EXPECT_EQ(result.failed_count(), 0);
    EXPECT_EQ(result.skipped_count(), 0);
    EXPECT_EQ(result.retryable_count(), 0);

    EXPECT_TRUE(result.all_delivered());
    EXPECT_FALSE(result.any_failed());
    EXPECT_FALSE(result.any_retryable());
}

TEST(NotificationDeliveryResultTests, RecordsFailedAndSkippedAttempts)
{
    dispatcher::domain::NotificationDeliveryResult result;

    result.record(
        dispatcher::domain::NotificationDeliveryAttempt::delivered(
            make_delivery_message(),
            make_delivery_route("route-delivered"),
            "delivered"
        )
    );

    result.record(
        dispatcher::domain::NotificationDeliveryAttempt::failed(
            make_delivery_message(),
            make_delivery_route("route-failed"),
            "smtp timeout"
        )
    );

    result.record(
        dispatcher::domain::NotificationDeliveryAttempt::skipped(
            make_delivery_message(),
            make_delivery_route("route-skipped"),
            "target disabled"
        )
    );

    EXPECT_EQ(result.attempt_count(), 3);
    EXPECT_EQ(result.delivered_count(), 1);
    EXPECT_EQ(result.failed_count(), 1);
    EXPECT_EQ(result.skipped_count(), 1);
    EXPECT_EQ(result.retryable_count(), 1);

    EXPECT_FALSE(result.all_delivered());
    EXPECT_TRUE(result.any_failed());
    EXPECT_TRUE(result.any_retryable());

    ASSERT_EQ(result.attempts().size(), 3);
}