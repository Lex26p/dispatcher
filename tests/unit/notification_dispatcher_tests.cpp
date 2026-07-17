#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/notification_channel.hpp>
#include <dispatcher/domain/notification_delivery_status.hpp>
#include <dispatcher/domain/notification_dispatch_result.hpp>
#include <dispatcher/domain/notification_dispatch_status.hpp>
#include <dispatcher/domain/notification_dispatcher.hpp>
#include <dispatcher/domain/notification_message.hpp>
#include <dispatcher/domain/notification_priority.hpp>
#include <dispatcher/domain/notification_route.hpp>
#include <dispatcher/domain/notification_route_table.hpp>
#include <dispatcher/domain/notification_target.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <utility>

namespace
{
    dispatcher::domain::NotificationTarget make_dispatch_email_target(
        std::string address = "operator@example.local"
    )
    {
        return dispatcher::domain::NotificationTarget(
            dispatcher::domain::NotificationChannel::Email,
            std::move(address),
            "Operator"
        );
    }

    dispatcher::domain::NotificationRoute make_dispatch_route(
        std::string route_id,
        std::string name,
        dispatcher::domain::NotificationPriority minimum_priority =
        dispatcher::domain::NotificationPriority::Low,
        std::string source_filter = "alarm-runtime"
    )
    {
        return dispatcher::domain::NotificationRoute(
            dispatcher::domain::NotificationRouteId{ std::move(route_id) },
            std::move(name),
            make_dispatch_email_target(),
            minimum_priority,
            std::move(source_filter),
            true
        );
    }

    dispatcher::domain::NotificationMessage make_dispatch_message(
        dispatcher::domain::NotificationPriority priority =
        dispatcher::domain::NotificationPriority::High,
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

TEST(NotificationDispatchStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationDispatchStatus::Dispatched
        ),
        "dispatched"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationDispatchStatus::NoRoutesConfigured
        ),
        "no_routes_configured"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationDispatchStatus::NoMatchingRoutes
        ),
        "no_matching_routes"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::NotificationDispatchStatus::InvalidMessage
        ),
        "invalid_message"
    );
}

TEST(NotificationDispatchStatusTests, PredicatesWork)
{
    EXPECT_TRUE(
        dispatcher::domain::is_success(
            dispatcher::domain::NotificationDispatchStatus::Dispatched
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_failure(
            dispatcher::domain::NotificationDispatchStatus::Dispatched
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_success(
            dispatcher::domain::NotificationDispatchStatus::NoMatchingRoutes
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_failure(
            dispatcher::domain::NotificationDispatchStatus::NoMatchingRoutes
        )
    );
}

TEST(NotificationDispatchResultTests, DispatchedResultAggregatesCounts)
{
    dispatcher::domain::NotificationRoutingResult routing_result;

    const auto route = make_dispatch_route(
        "route-1",
        "email route"
    );

    routing_result.record(
        route,
        dispatcher::domain::NotificationRouteResult::accepted(
            dispatcher::domain::NotificationRouteId{ "route-1" },
            "accepted"
        )
    );

    dispatcher::domain::NotificationDeliveryResult delivery_result;

    delivery_result.record(
        dispatcher::domain::NotificationDeliveryAttempt::pending(
            make_dispatch_message(),
            route
        )
    );

    const auto result =
        dispatcher::domain::NotificationDispatchResult::dispatched(
            make_dispatch_message(),
            std::move(routing_result),
            std::move(delivery_result)
        );

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationDispatchStatus::Dispatched
    );

    EXPECT_TRUE(result.has_message());

    EXPECT_EQ(
        result.message().notification_id(),
        dispatcher::domain::NotificationId{ "notification-1" }
    );

    EXPECT_EQ(result.accepted_route_count(), 1);
    EXPECT_EQ(result.rejected_route_count(), 0);
    EXPECT_EQ(result.delivery_attempt_count(), 1);

    EXPECT_FALSE(result.has_reason());
}

TEST(NotificationDispatchResultTests, NotDispatchedResultKeepsReason)
{
    const auto result =
        dispatcher::domain::NotificationDispatchResult::not_dispatched(
            dispatcher::domain::NotificationDispatchStatus::NoMatchingRoutes,
            make_dispatch_message(),
            {},
            {},
            "no routes matched"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationDispatchStatus::NoMatchingRoutes
    );

    EXPECT_TRUE(result.has_message());
    EXPECT_TRUE(result.has_reason());
    EXPECT_EQ(result.reason(), "no routes matched");

    EXPECT_EQ(result.accepted_route_count(), 0);
    EXPECT_EQ(result.rejected_route_count(), 0);
    EXPECT_EQ(result.delivery_attempt_count(), 0);
}

TEST(NotificationDispatchResultTests, NotDispatchedRejectsSuccessStatus)
{
    const auto result =
        dispatcher::domain::NotificationDispatchResult::not_dispatched(
            dispatcher::domain::NotificationDispatchStatus::Dispatched,
            make_dispatch_message(),
            {},
            {},
            "bad caller status"
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationDispatchStatus::InvalidMessage
    );
}

TEST(NotificationDispatcherTests, DefaultDispatcherHasEmptyRouteTable)
{
    const dispatcher::domain::NotificationDispatcher dispatcher;

    EXPECT_TRUE(dispatcher.route_table().empty());
}

TEST(NotificationDispatcherTests, AddRouteStoresRouteInTable)
{
    dispatcher::domain::NotificationDispatcher dispatcher;

    const auto result =
        dispatcher.add_route(
            make_dispatch_route(
                "route-1",
                "email route"
            )
        );

    EXPECT_TRUE(result.accepted());

    EXPECT_EQ(dispatcher.route_table().size(), 1);

    EXPECT_TRUE(
        dispatcher.route_table().contains(
            dispatcher::domain::NotificationRouteId{ "route-1" }
        )
    );
}

TEST(NotificationDispatcherTests, RemoveRouteDeletesRouteFromTable)
{
    dispatcher::domain::NotificationDispatcher dispatcher;

    ASSERT_TRUE(
        dispatcher.add_route(
            make_dispatch_route(
                "route-1",
                "email route"
            )
        ).accepted()
    );

    const auto result =
        dispatcher.remove_route(
            dispatcher::domain::NotificationRouteId{ "route-1" }
        );

    EXPECT_TRUE(result.accepted());
    EXPECT_TRUE(dispatcher.route_table().empty());
}

TEST(NotificationDispatcherTests, SetRouteTableReplacesRoutes)
{
    dispatcher::domain::NotificationRouteTable table;

    ASSERT_TRUE(
        table.add(
            make_dispatch_route(
                "route-1",
                "email route"
            )
        ).accepted()
    );

    dispatcher::domain::NotificationDispatcher dispatcher;

    dispatcher.set_route_table(
        std::move(table)
    );

    EXPECT_EQ(dispatcher.route_table().size(), 1);

    EXPECT_TRUE(
        dispatcher.route_table().contains(
            dispatcher::domain::NotificationRouteId{ "route-1" }
        )
    );
}

TEST(NotificationDispatcherTests, DispatchRejectsInvalidMessage)
{
    dispatcher::domain::NotificationDispatcher dispatcher;

    ASSERT_TRUE(
        dispatcher.add_route(
            make_dispatch_route(
                "route-1",
                "email route"
            )
        ).accepted()
    );

    const dispatcher::domain::NotificationMessage invalid_message(
        dispatcher::domain::NotificationId{ "" },
        dispatcher::domain::NotificationPriority::High,
        "",
        "",
        "alarm-runtime"
    );

    const auto result =
        dispatcher.dispatch(
            invalid_message
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationDispatchStatus::InvalidMessage
    );

    EXPECT_TRUE(result.has_reason());
    EXPECT_EQ(result.reason(), "notification message is invalid");

    EXPECT_EQ(result.accepted_route_count(), 0);
    EXPECT_EQ(result.delivery_attempt_count(), 0);
}

TEST(NotificationDispatcherTests, DispatchReportsNoRoutesConfigured)
{
    const dispatcher::domain::NotificationDispatcher dispatcher;

    const auto result =
        dispatcher.dispatch(
            make_dispatch_message()
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationDispatchStatus::NoRoutesConfigured
    );

    EXPECT_TRUE(result.routing_result().empty());
    EXPECT_EQ(result.delivery_attempt_count(), 0);
}

TEST(NotificationDispatcherTests, DispatchReportsNoMatchingRoutes)
{
    dispatcher::domain::NotificationDispatcher dispatcher;

    ASSERT_TRUE(
        dispatcher.add_route(
            make_dispatch_route(
                "route-1",
                "configuration route",
                dispatcher::domain::NotificationPriority::Low,
                "configuration"
            )
        ).accepted()
    );

    const auto result =
        dispatcher.dispatch(
            make_dispatch_message(
                dispatcher::domain::NotificationPriority::High,
                "alarm-runtime"
            )
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationDispatchStatus::NoMatchingRoutes
    );

    EXPECT_FALSE(result.routing_result().empty());
    EXPECT_TRUE(result.routing_result().unmatched());

    EXPECT_EQ(result.accepted_route_count(), 0);
    EXPECT_EQ(result.rejected_route_count(), 1);
    EXPECT_EQ(result.delivery_attempt_count(), 0);
}

TEST(NotificationDispatcherTests, DispatchCreatesPendingAttemptsForAcceptedRoutes)
{
    dispatcher::domain::NotificationDispatcher dispatcher;

    ASSERT_TRUE(
        dispatcher.add_route(
            make_dispatch_route(
                "route-email",
                "email route",
                dispatcher::domain::NotificationPriority::Low,
                "alarm-runtime"
            )
        ).accepted()
    );

    ASSERT_TRUE(
        dispatcher.add_route(
            make_dispatch_route(
                "route-critical",
                "critical route",
                dispatcher::domain::NotificationPriority::Critical,
                "alarm-runtime"
            )
        ).accepted()
    );

    const auto result =
        dispatcher.dispatch(
            make_dispatch_message(
                dispatcher::domain::NotificationPriority::Critical,
                "alarm-runtime"
            )
        );

    EXPECT_TRUE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationDispatchStatus::Dispatched
    );

    EXPECT_EQ(result.accepted_route_count(), 2);
    EXPECT_EQ(result.rejected_route_count(), 0);
    EXPECT_EQ(result.delivery_attempt_count(), 2);

    ASSERT_EQ(result.delivery_result().attempts().size(), 2);

    EXPECT_EQ(
        result.delivery_result().attempts()[0].status(),
        dispatcher::domain::NotificationDeliveryStatus::Pending
    );

    EXPECT_EQ(
        result.delivery_result().attempts()[1].status(),
        dispatcher::domain::NotificationDeliveryStatus::Pending
    );

    EXPECT_TRUE(result.delivery_result().any_retryable());
}

TEST(NotificationDispatcherTests, DispatchKeepsRejectedRouteResults)
{
    dispatcher::domain::NotificationDispatcher dispatcher;

    ASSERT_TRUE(
        dispatcher.add_route(
            make_dispatch_route(
                "route-low",
                "low route",
                dispatcher::domain::NotificationPriority::Low,
                "alarm-runtime"
            )
        ).accepted()
    );

    ASSERT_TRUE(
        dispatcher.add_route(
            make_dispatch_route(
                "route-critical",
                "critical route",
                dispatcher::domain::NotificationPriority::Critical,
                "alarm-runtime"
            )
        ).accepted()
    );

    const auto result =
        dispatcher.dispatch(
            make_dispatch_message(
                dispatcher::domain::NotificationPriority::High,
                "alarm-runtime"
            )
        );

    EXPECT_TRUE(result.ok());

    EXPECT_EQ(result.accepted_route_count(), 1);
    EXPECT_EQ(result.rejected_route_count(), 1);
    EXPECT_EQ(result.delivery_attempt_count(), 1);

    EXPECT_EQ(result.routing_result().evaluated_count(), 2);
    EXPECT_EQ(result.delivery_result().attempt_count(), 1);
}