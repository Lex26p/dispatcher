#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/notification_channel.hpp>
#include <dispatcher/domain/notification_message.hpp>
#include <dispatcher/domain/notification_priority.hpp>
#include <dispatcher/domain/notification_target.hpp>

#include <gtest/gtest.h>

#include <chrono>

TEST(NotificationChannelTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationChannel::Unknown
        ),
        "unknown"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationChannel::Console
        ),
        "console"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationChannel::Email
        ),
        "email"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationChannel::Sms
        ),
        "sms"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationChannel::Webhook
        ),
        "webhook"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationChannel::AuditLog
        ),
        "audit_log"
    );
}

TEST(NotificationChannelTests, ChannelPredicatesWork)
{
    EXPECT_FALSE(
        dispatcher::domain::is_known_channel(
            dispatcher::domain::NotificationChannel::Unknown
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_known_channel(
            dispatcher::domain::NotificationChannel::Email
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::requires_address(
            dispatcher::domain::NotificationChannel::Email
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::requires_address(
            dispatcher::domain::NotificationChannel::Sms
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::requires_address(
            dispatcher::domain::NotificationChannel::Webhook
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::requires_address(
            dispatcher::domain::NotificationChannel::Console
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_external_channel(
            dispatcher::domain::NotificationChannel::Webhook
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_external_channel(
            dispatcher::domain::NotificationChannel::AuditLog
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_internal_channel(
            dispatcher::domain::NotificationChannel::Console
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_internal_channel(
            dispatcher::domain::NotificationChannel::AuditLog
        )
    );
}

TEST(NotificationPriorityTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationPriority::Low
        ),
        "low"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationPriority::Normal
        ),
        "normal"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationPriority::High
        ),
        "high"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationPriority::Critical
        ),
        "critical"
    );
}

TEST(NotificationPriorityTests, DeliveryOrderPrioritizesCritical)
{
    EXPECT_LT(
        dispatcher::domain::delivery_order(
            dispatcher::domain::NotificationPriority::Critical
        ),
        dispatcher::domain::delivery_order(
            dispatcher::domain::NotificationPriority::High
        )
    );

    EXPECT_LT(
        dispatcher::domain::delivery_order(
            dispatcher::domain::NotificationPriority::High
        ),
        dispatcher::domain::delivery_order(
            dispatcher::domain::NotificationPriority::Normal
        )
    );

    EXPECT_LT(
        dispatcher::domain::delivery_order(
            dispatcher::domain::NotificationPriority::Normal
        ),
        dispatcher::domain::delivery_order(
            dispatcher::domain::NotificationPriority::Low
        )
    );
}

TEST(NotificationPriorityTests, UrgentAndAttentionPredicatesWork)
{
    EXPECT_FALSE(
        dispatcher::domain::is_urgent(
            dispatcher::domain::NotificationPriority::Low
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_urgent(
            dispatcher::domain::NotificationPriority::Normal
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_urgent(
            dispatcher::domain::NotificationPriority::High
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_urgent(
            dispatcher::domain::NotificationPriority::Critical
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::requires_operator_attention(
            dispatcher::domain::NotificationPriority::High
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::requires_operator_attention(
            dispatcher::domain::NotificationPriority::Critical
        )
    );
}

TEST(NotificationTargetTests, EmailTargetRequiresAddress)
{
    const dispatcher::domain::NotificationTarget invalid_email(
        dispatcher::domain::NotificationChannel::Email
    );

    EXPECT_FALSE(invalid_email.has_address());
    EXPECT_FALSE(invalid_email.valid());

    const dispatcher::domain::NotificationTarget email(
        dispatcher::domain::NotificationChannel::Email,
        "operator@example.local",
        "Operator"
    );

    EXPECT_EQ(
        email.channel(),
        dispatcher::domain::NotificationChannel::Email
    );

    EXPECT_EQ(email.address(), "operator@example.local");
    EXPECT_EQ(email.display_name(), "Operator");

    EXPECT_TRUE(email.enabled());
    EXPECT_FALSE(email.disabled());

    EXPECT_TRUE(email.has_address());
    EXPECT_TRUE(email.has_display_name());

    EXPECT_TRUE(email.valid());
    EXPECT_TRUE(email.external());
    EXPECT_FALSE(email.internal());
}

TEST(NotificationTargetTests, InternalTargetsCanBeAddressless)
{
    const dispatcher::domain::NotificationTarget console(
        dispatcher::domain::NotificationChannel::Console
    );

    EXPECT_TRUE(console.valid());
    EXPECT_TRUE(console.internal());
    EXPECT_FALSE(console.external());

    const dispatcher::domain::NotificationTarget audit_log(
        dispatcher::domain::NotificationChannel::AuditLog
    );

    EXPECT_TRUE(audit_log.valid());
    EXPECT_TRUE(audit_log.internal());
}

TEST(NotificationTargetTests, DisabledTargetIsNotValid)
{
    const dispatcher::domain::NotificationTarget target(
        dispatcher::domain::NotificationChannel::Webhook,
        "https://notifications.example.local/hook",
        "main webhook",
        false
    );

    EXPECT_FALSE(target.enabled());
    EXPECT_TRUE(target.disabled());
    EXPECT_FALSE(target.valid());
}

TEST(NotificationTargetTests, UnknownTargetIsNotValid)
{
    const dispatcher::domain::NotificationTarget target(
        dispatcher::domain::NotificationChannel::Unknown
    );

    EXPECT_FALSE(target.valid());
    EXPECT_FALSE(target.internal());
    EXPECT_FALSE(target.external());
}

TEST(NotificationMessageTests, MessageCapturesFields)
{
    const auto now =
        dispatcher::domain::NotificationMessage::Clock::now();

    const dispatcher::domain::NotificationMessage message(
        dispatcher::domain::NotificationId{ "notification-1" },
        dispatcher::domain::NotificationPriority::High,
        "Alarm active",
        "Temperature alarm is active",
        "alarm-runtime",
        now
    );

    EXPECT_EQ(
        message.notification_id(),
        dispatcher::domain::NotificationId{ "notification-1" }
    );

    EXPECT_EQ(message.notification_id().value(), "notification-1");
    EXPECT_EQ(message.priority(), dispatcher::domain::NotificationPriority::High);
    EXPECT_EQ(message.subject(), "Alarm active");
    EXPECT_EQ(message.body(), "Temperature alarm is active");
    EXPECT_EQ(message.source(), "alarm-runtime");
    EXPECT_EQ(message.created_at(), now);

    EXPECT_TRUE(message.has_subject());
    EXPECT_TRUE(message.has_body());
    EXPECT_TRUE(message.has_source());

    EXPECT_FALSE(message.empty());
    EXPECT_TRUE(message.valid());

    EXPECT_TRUE(message.urgent());
    EXPECT_FALSE(message.requires_attention());
}

TEST(NotificationMessageTests, CriticalMessageRequiresAttention)
{
    const auto message =
        dispatcher::domain::NotificationMessage::create(
            dispatcher::domain::NotificationId{ "notification-critical" },
            dispatcher::domain::NotificationPriority::Critical,
            "Critical alarm",
            "Critical pressure alarm is active",
            "alarm-runtime"
        );

    EXPECT_TRUE(message.valid());
    EXPECT_TRUE(message.urgent());
    EXPECT_TRUE(message.requires_attention());
}

TEST(NotificationMessageTests, MessageWithoutIdIsInvalid)
{
    const dispatcher::domain::NotificationMessage message(
        dispatcher::domain::NotificationId{ "" },
        dispatcher::domain::NotificationPriority::Normal,
        "Subject",
        "Body"
    );

    EXPECT_FALSE(message.valid());
    EXPECT_FALSE(message.empty());
}

TEST(NotificationMessageTests, MessageWithoutSubjectAndBodyIsInvalid)
{
    const dispatcher::domain::NotificationMessage message(
        dispatcher::domain::NotificationId{ "notification-empty" },
        dispatcher::domain::NotificationPriority::Normal,
        "",
        ""
    );

    EXPECT_TRUE(message.empty());
    EXPECT_FALSE(message.valid());

    EXPECT_FALSE(message.has_subject());
    EXPECT_FALSE(message.has_body());
    EXPECT_FALSE(message.has_source());
}