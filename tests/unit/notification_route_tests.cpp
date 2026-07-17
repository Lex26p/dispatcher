#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/notification_channel.hpp>
#include <dispatcher/domain/notification_message.hpp>
#include <dispatcher/domain/notification_priority.hpp>
#include <dispatcher/domain/notification_route.hpp>
#include <dispatcher/domain/notification_route_result.hpp>
#include <dispatcher/domain/notification_route_status.hpp>
#include <dispatcher/domain/notification_target.hpp>

#include <gtest/gtest.h>

#include <string>
#include <utility>

namespace
{
    dispatcher::domain::NotificationTarget make_route_email_target()
    {
        return dispatcher::domain::NotificationTarget(
            dispatcher::domain::NotificationChannel::Email,
            "operator@example.local",
            "Operator"
        );
    }

    dispatcher::domain::NotificationRoute make_route(
        std::string route_id = "route-1",
        std::string name = "primary email route",
        dispatcher::domain::NotificationPriority minimum_priority =
        dispatcher::domain::NotificationPriority::Low,
        std::string source_filter = {},
        bool enabled = true
    )
    {
        return dispatcher::domain::NotificationRoute(
            dispatcher::domain::NotificationRouteId{ std::move(route_id) },
            std::move(name),
            make_route_email_target(),
            minimum_priority,
            std::move(source_filter),
            enabled
        );
    }

    dispatcher::domain::NotificationMessage make_route_message(
        dispatcher::domain::NotificationPriority priority =
        dispatcher::domain::NotificationPriority::Normal,
        std::string source = "alarm-runtime"
    )
    {
        return dispatcher::domain::NotificationMessage::create(
            dispatcher::domain::NotificationId{ "notification-1" },
            priority,
            "Alarm active",
            "Temperature alarm is active",
            std::move(source)
        );
    }
}

TEST(NotificationRouteStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationRouteStatus::Accepted
        ),
        "accepted"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationRouteStatus::RejectedInvalidRoute
        ),
        "rejected_invalid_route"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationRouteStatus::RejectedDisabledRoute
        ),
        "rejected_disabled_route"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationRouteStatus::RejectedPriorityTooLow
        ),
        "rejected_priority_too_low"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationRouteStatus::RejectedSourceMismatch
        ),
        "rejected_source_mismatch"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationRouteStatus::RejectedTargetInvalid
        ),
        "rejected_target_invalid"
    );
}

TEST(NotificationRouteStatusTests, PredicatesWork)
{
    EXPECT_TRUE(
        dispatcher::domain::is_accepted(
            dispatcher::domain::NotificationRouteStatus::Accepted
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_rejected(
            dispatcher::domain::NotificationRouteStatus::Accepted
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_accepted(
            dispatcher::domain::NotificationRouteStatus::RejectedInvalidRoute
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_rejected(
            dispatcher::domain::NotificationRouteStatus::RejectedInvalidRoute
        )
    );
}

TEST(NotificationRouteResultTests, AcceptedResultWorks)
{
    const auto result =
        dispatcher::domain::NotificationRouteResult::accepted(
            dispatcher::domain::NotificationRouteId{ "route-1" },
            "accepted"
        );

    EXPECT_TRUE(result.accepted());
    EXPECT_FALSE(result.rejected());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationRouteStatus::Accepted
    );

    EXPECT_EQ(
        result.route_id(),
        dispatcher::domain::NotificationRouteId{ "route-1" }
    );

    EXPECT_TRUE(result.has_message());
    EXPECT_EQ(result.message(), "accepted");

    EXPECT_FALSE(result.has_reason());
    EXPECT_FALSE(result.has_field());
}

TEST(NotificationRouteResultTests, RejectedResultWorks)
{
    const auto result =
        dispatcher::domain::NotificationRouteResult::rejected(
            dispatcher::domain::NotificationRouteId{ "route-1" },
            dispatcher::domain::NotificationRouteStatus::RejectedPriorityTooLow,
            "priority too low",
            "priority"
        );

    EXPECT_FALSE(result.accepted());
    EXPECT_TRUE(result.rejected());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationRouteStatus::RejectedPriorityTooLow
    );

    EXPECT_EQ(result.reason(), "priority too low");
    EXPECT_EQ(result.field(), "priority");

    EXPECT_FALSE(result.has_message());
    EXPECT_TRUE(result.has_reason());
    EXPECT_TRUE(result.has_field());
}

TEST(NotificationRouteResultTests, RejectedRejectsAcceptedStatus)
{
    const auto result =
        dispatcher::domain::NotificationRouteResult::rejected(
            dispatcher::domain::NotificationRouteId{ "route-1" },
            dispatcher::domain::NotificationRouteStatus::Accepted,
            "bad caller status",
            "status"
        );

    EXPECT_TRUE(result.rejected());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationRouteStatus::RejectedInvalidRoute
    );
}

TEST(NotificationRouteTests, RouteCapturesFields)
{
    const auto route = make_route(
        "route-1",
        "primary email route",
        dispatcher::domain::NotificationPriority::High,
        "alarm-runtime"
    );

    EXPECT_EQ(
        route.route_id(),
        dispatcher::domain::NotificationRouteId{ "route-1" }
    );

    EXPECT_EQ(route.name(), "primary email route");

    EXPECT_EQ(
        route.minimum_priority(),
        dispatcher::domain::NotificationPriority::High
    );

    EXPECT_EQ(route.source_filter(), "alarm-runtime");

    EXPECT_TRUE(route.enabled());
    EXPECT_FALSE(route.disabled());

    EXPECT_TRUE(route.has_name());
    EXPECT_TRUE(route.has_source_filter());

    EXPECT_TRUE(route.valid());
}

TEST(NotificationRouteTests, RouteWithoutIdIsInvalid)
{
    const auto route = make_route(
        "",
        "primary email route"
    );

    EXPECT_FALSE(route.valid());
}

TEST(NotificationRouteTests, RouteWithoutNameIsInvalid)
{
    const auto route = make_route(
        "route-1",
        ""
    );

    EXPECT_FALSE(route.valid());
}

TEST(NotificationRouteTests, RouteWithInvalidTargetIsInvalid)
{
    const dispatcher::domain::NotificationRoute route(
        dispatcher::domain::NotificationRouteId{ "route-1" },
        "invalid email route",
        dispatcher::domain::NotificationTarget(
            dispatcher::domain::NotificationChannel::Email
        )
    );

    EXPECT_FALSE(route.valid());
}

TEST(NotificationRouteTests, AcceptsPriorityUsesMinimumPriority)
{
    const auto route = make_route(
        "route-1",
        "critical-only",
        dispatcher::domain::NotificationPriority::Critical
    );

    EXPECT_TRUE(
        route.accepts_priority(
            dispatcher::domain::NotificationPriority::Critical
        )
    );

    EXPECT_FALSE(
        route.accepts_priority(
            dispatcher::domain::NotificationPriority::High
        )
    );

    EXPECT_FALSE(
        route.accepts_priority(
            dispatcher::domain::NotificationPriority::Normal
        )
    );
}

TEST(NotificationRouteTests, AcceptsSourceMatchesOptionalFilter)
{
    const auto route_without_filter = make_route();

    EXPECT_TRUE(route_without_filter.accepts_source(""));
    EXPECT_TRUE(route_without_filter.accepts_source("alarm-runtime"));
    EXPECT_TRUE(route_without_filter.accepts_source("configuration"));

    const auto route_with_filter = make_route(
        "route-1",
        "alarm route",
        dispatcher::domain::NotificationPriority::Low,
        "alarm-runtime"
    );

    EXPECT_TRUE(route_with_filter.accepts_source("alarm-runtime"));
    EXPECT_FALSE(route_with_filter.accepts_source("configuration"));
    EXPECT_FALSE(route_with_filter.accepts_source(""));
}

TEST(NotificationRouteTests, EvaluateAcceptsMatchingMessage)
{
    const auto route = make_route(
        "route-1",
        "alarm email route",
        dispatcher::domain::NotificationPriority::High,
        "alarm-runtime"
    );

    const auto result = route.evaluate(
        make_route_message(
            dispatcher::domain::NotificationPriority::Critical,
            "alarm-runtime"
        )
    );

    EXPECT_TRUE(result.accepted());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationRouteStatus::Accepted
    );
}

TEST(NotificationRouteTests, EvaluateRejectsInvalidRoute)
{
    const auto route = make_route(
        "",
        "missing id"
    );

    const auto result = route.evaluate(
        make_route_message()
    );

    EXPECT_TRUE(result.rejected());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationRouteStatus::RejectedInvalidRoute
    );

    EXPECT_EQ(result.field(), "route");
}

TEST(NotificationRouteTests, EvaluateRejectsDisabledRoute)
{
    const auto route = make_route(
        "route-1",
        "disabled route",
        dispatcher::domain::NotificationPriority::Low,
        {},
        false
    );

    const auto result = route.evaluate(
        make_route_message()
    );

    EXPECT_TRUE(result.rejected());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationRouteStatus::RejectedDisabledRoute
    );

    EXPECT_EQ(result.field(), "enabled");
}

TEST(NotificationRouteTests, EvaluateRejectsPriorityTooLow)
{
    const auto route = make_route(
        "route-1",
        "critical-only",
        dispatcher::domain::NotificationPriority::Critical
    );

    const auto result = route.evaluate(
        make_route_message(
            dispatcher::domain::NotificationPriority::High
        )
    );

    EXPECT_TRUE(result.rejected());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationRouteStatus::RejectedPriorityTooLow
    );

    EXPECT_EQ(result.field(), "priority");
}

TEST(NotificationRouteTests, EvaluateRejectsSourceMismatch)
{
    const auto route = make_route(
        "route-1",
        "alarm-only",
        dispatcher::domain::NotificationPriority::Low,
        "alarm-runtime"
    );

    const auto result = route.evaluate(
        make_route_message(
            dispatcher::domain::NotificationPriority::Critical,
            "configuration"
        )
    );

    EXPECT_TRUE(result.rejected());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationRouteStatus::RejectedSourceMismatch
    );

    EXPECT_EQ(result.field(), "source");
}