#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/notification_channel.hpp>
#include <dispatcher/domain/notification_message.hpp>
#include <dispatcher/domain/notification_priority.hpp>
#include <dispatcher/domain/notification_route.hpp>
#include <dispatcher/domain/notification_route_result.hpp>
#include <dispatcher/domain/notification_route_status.hpp>
#include <dispatcher/domain/notification_route_table.hpp>
#include <dispatcher/domain/notification_routing_result.hpp>
#include <dispatcher/domain/notification_target.hpp>

#include <gtest/gtest.h>

#include <string>
#include <utility>

namespace
{
    dispatcher::domain::NotificationTarget make_table_email_target(
        std::string address = "operator@example.local"
    )
    {
        return dispatcher::domain::NotificationTarget(
            dispatcher::domain::NotificationChannel::Email,
            std::move(address),
            "Operator"
        );
    }

    dispatcher::domain::NotificationRoute make_table_route(
        std::string route_id,
        std::string name,
        dispatcher::domain::NotificationPriority minimum_priority =
        dispatcher::domain::NotificationPriority::Low,
        std::string source_filter = {},
        bool enabled = true
    )
    {
        return dispatcher::domain::NotificationRoute(
            dispatcher::domain::NotificationRouteId{ std::move(route_id) },
            std::move(name),
            make_table_email_target(),
            minimum_priority,
            std::move(source_filter),
            enabled
        );
    }

    dispatcher::domain::NotificationMessage make_table_message(
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

TEST(NotificationRoutingResultTests, DefaultResultIsEmptyAndUnmatched)
{
    const dispatcher::domain::NotificationRoutingResult result;

    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(result.matched());
    EXPECT_TRUE(result.unmatched());

    EXPECT_EQ(result.accepted_count(), 0);
    EXPECT_EQ(result.rejected_count(), 0);
    EXPECT_EQ(result.evaluated_count(), 0);

    EXPECT_TRUE(result.accepted_routes().empty());
    EXPECT_TRUE(result.results().empty());
}

TEST(NotificationRoutingResultTests, RecordAcceptedRoute)
{
    dispatcher::domain::NotificationRoutingResult result;

    const auto route = make_table_route(
        "route-1",
        "route one"
    );

    result.record(
        route,
        dispatcher::domain::NotificationRouteResult::accepted(
            dispatcher::domain::NotificationRouteId{ "route-1" },
            "accepted"
        )
    );

    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.matched());
    EXPECT_FALSE(result.unmatched());

    EXPECT_EQ(result.accepted_count(), 1);
    EXPECT_EQ(result.rejected_count(), 0);
    EXPECT_EQ(result.evaluated_count(), 1);

    ASSERT_EQ(result.accepted_routes().size(), 1);
    EXPECT_EQ(
        result.accepted_routes().front().route_id(),
        dispatcher::domain::NotificationRouteId{ "route-1" }
    );

    ASSERT_EQ(result.results().size(), 1);
    EXPECT_TRUE(result.results().front().accepted());
}

TEST(NotificationRoutingResultTests, RecordRejectedRoute)
{
    dispatcher::domain::NotificationRoutingResult result;

    const auto route = make_table_route(
        "route-1",
        "route one",
        dispatcher::domain::NotificationPriority::Critical
    );

    result.record(
        route,
        dispatcher::domain::NotificationRouteResult::rejected(
            dispatcher::domain::NotificationRouteId{ "route-1" },
            dispatcher::domain::NotificationRouteStatus::RejectedPriorityTooLow,
            "priority too low",
            "priority"
        )
    );

    EXPECT_FALSE(result.empty());
    EXPECT_FALSE(result.matched());
    EXPECT_TRUE(result.unmatched());

    EXPECT_EQ(result.accepted_count(), 0);
    EXPECT_EQ(result.rejected_count(), 1);
    EXPECT_EQ(result.evaluated_count(), 1);

    EXPECT_TRUE(result.accepted_routes().empty());

    ASSERT_EQ(result.results().size(), 1);
    EXPECT_TRUE(result.results().front().rejected());
}

TEST(NotificationRouteTableTests, DefaultTableIsEmpty)
{
    const dispatcher::domain::NotificationRouteTable table;

    EXPECT_TRUE(table.empty());
    EXPECT_EQ(table.size(), 0);

    EXPECT_FALSE(
        table.contains(
            dispatcher::domain::NotificationRouteId{ "route-1" }
        )
    );

    EXPECT_FALSE(
        table.find(
            dispatcher::domain::NotificationRouteId{ "route-1" }
        ).has_value()
    );

    EXPECT_TRUE(table.routes().empty());
}

TEST(NotificationRouteTableTests, AddStoresRoute)
{
    dispatcher::domain::NotificationRouteTable table;

    const auto result =
        table.add(
            make_table_route(
                "route-1",
                "route one"
            )
        );

    EXPECT_TRUE(result.accepted());

    EXPECT_FALSE(table.empty());
    EXPECT_EQ(table.size(), 1);

    EXPECT_TRUE(
        table.contains(
            dispatcher::domain::NotificationRouteId{ "route-1" }
        )
    );

    const auto found =
        table.find(
            dispatcher::domain::NotificationRouteId{ "route-1" }
        );

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name(), "route one");
}

TEST(NotificationRouteTableTests, AddRejectsEmptyRouteId)
{
    dispatcher::domain::NotificationRouteTable table;

    const auto result =
        table.add(
            make_table_route(
                "",
                "route without id"
            )
        );

    EXPECT_TRUE(result.rejected());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationRouteStatus::RejectedInvalidRoute
    );

    EXPECT_EQ(result.field(), "route_id");
    EXPECT_TRUE(table.empty());
}

TEST(NotificationRouteTableTests, AddRejectsInvalidRoute)
{
    dispatcher::domain::NotificationRouteTable table;

    const auto result =
        table.add(
            make_table_route(
                "route-1",
                ""
            )
        );

    EXPECT_TRUE(result.rejected());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationRouteStatus::RejectedInvalidRoute
    );

    EXPECT_EQ(result.field(), "route");
    EXPECT_TRUE(table.empty());
}

TEST(NotificationRouteTableTests, AddRejectsDuplicateRouteId)
{
    dispatcher::domain::NotificationRouteTable table;

    ASSERT_TRUE(
        table.add(
            make_table_route(
                "route-1",
                "route one"
            )
        ).accepted()
    );

    const auto result =
        table.add(
            make_table_route(
                "route-1",
                "route duplicate"
            )
        );

    EXPECT_TRUE(result.rejected());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationRouteStatus::RejectedInvalidRoute
    );

    EXPECT_EQ(result.field(), "route_id");
    EXPECT_EQ(table.size(), 1);
}

TEST(NotificationRouteTableTests, RoutesAreReturnedSortedByRouteId)
{
    dispatcher::domain::NotificationRouteTable table;

    ASSERT_TRUE(
        table.add(
            make_table_route(
                "route-z",
                "zeta"
            )
        ).accepted()
    );

    ASSERT_TRUE(
        table.add(
            make_table_route(
                "route-a",
                "alpha"
            )
        ).accepted()
    );

    ASSERT_TRUE(
        table.add(
            make_table_route(
                "route-m",
                "middle"
            )
        ).accepted()
    );

    const auto routes = table.routes();

    ASSERT_EQ(routes.size(), 3);

    EXPECT_EQ(routes[0].route_id().value(), "route-a");
    EXPECT_EQ(routes[1].route_id().value(), "route-m");
    EXPECT_EQ(routes[2].route_id().value(), "route-z");
}

TEST(NotificationRouteTableTests, RemoveDeletesRoute)
{
    dispatcher::domain::NotificationRouteTable table;

    ASSERT_TRUE(
        table.add(
            make_table_route(
                "route-1",
                "route one"
            )
        ).accepted()
    );

    const auto result =
        table.remove(
            dispatcher::domain::NotificationRouteId{ "route-1" }
        );

    EXPECT_TRUE(result.accepted());

    EXPECT_FALSE(
        table.contains(
            dispatcher::domain::NotificationRouteId{ "route-1" }
        )
    );

    EXPECT_TRUE(table.empty());
}

TEST(NotificationRouteTableTests, RemoveRejectsMissingRoute)
{
    dispatcher::domain::NotificationRouteTable table;

    const auto result =
        table.remove(
            dispatcher::domain::NotificationRouteId{ "missing" }
        );

    EXPECT_TRUE(result.rejected());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::NotificationRouteStatus::RejectedInvalidRoute
    );

    EXPECT_EQ(result.field(), "route_id");
}

TEST(NotificationRouteTableTests, RouteReturnsEmptyResultWhenTableIsEmpty)
{
    const dispatcher::domain::NotificationRouteTable table;

    const auto result =
        table.route(
            make_table_message()
        );

    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(result.matched());
    EXPECT_TRUE(result.unmatched());
}

TEST(NotificationRouteTableTests, RouteMatchesAllAcceptedRoutes)
{
    dispatcher::domain::NotificationRouteTable table;

    ASSERT_TRUE(
        table.add(
            make_table_route(
                "route-email",
                "email route",
                dispatcher::domain::NotificationPriority::Low
            )
        ).accepted()
    );

    ASSERT_TRUE(
        table.add(
            make_table_route(
                "route-critical",
                "critical route",
                dispatcher::domain::NotificationPriority::Critical
            )
        ).accepted()
    );

    const auto result =
        table.route(
            make_table_message(
                dispatcher::domain::NotificationPriority::Critical,
                "alarm-runtime"
            )
        );

    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.matched());

    EXPECT_EQ(result.evaluated_count(), 2);
    EXPECT_EQ(result.accepted_count(), 2);
    EXPECT_EQ(result.rejected_count(), 0);

    ASSERT_EQ(result.accepted_routes().size(), 2);

    EXPECT_EQ(
        result.accepted_routes()[0].route_id(),
        dispatcher::domain::NotificationRouteId{ "route-critical" }
    );

    EXPECT_EQ(
        result.accepted_routes()[1].route_id(),
        dispatcher::domain::NotificationRouteId{ "route-email" }
    );
}

TEST(NotificationRouteTableTests, RouteSeparatesAcceptedAndRejectedRoutes)
{
    dispatcher::domain::NotificationRouteTable table;

    ASSERT_TRUE(
        table.add(
            make_table_route(
                "route-low",
                "low route",
                dispatcher::domain::NotificationPriority::Low
            )
        ).accepted()
    );

    ASSERT_TRUE(
        table.add(
            make_table_route(
                "route-critical",
                "critical route",
                dispatcher::domain::NotificationPriority::Critical
            )
        ).accepted()
    );

    ASSERT_TRUE(
        table.add(
            make_table_route(
                "route-config",
                "config route",
                dispatcher::domain::NotificationPriority::Low,
                "configuration"
            )
        ).accepted()
    );

    const auto result =
        table.route(
            make_table_message(
                dispatcher::domain::NotificationPriority::High,
                "alarm-runtime"
            )
        );

    EXPECT_FALSE(result.empty());

    EXPECT_EQ(result.evaluated_count(), 3);
    EXPECT_EQ(result.accepted_count(), 1);
    EXPECT_EQ(result.rejected_count(), 2);

    ASSERT_EQ(result.accepted_routes().size(), 1);

    EXPECT_EQ(
        result.accepted_routes().front().route_id(),
        dispatcher::domain::NotificationRouteId{ "route-low" }
    );

    ASSERT_EQ(result.results().size(), 3);

    EXPECT_TRUE(result.results()[0].rejected());
    EXPECT_TRUE(result.results()[1].rejected());
    EXPECT_TRUE(result.results()[2].accepted());
}

TEST(NotificationRouteTableTests, ClearRemovesAllRoutes)
{
    dispatcher::domain::NotificationRouteTable table;

    ASSERT_TRUE(
        table.add(
            make_table_route(
                "route-1",
                "route one"
            )
        ).accepted()
    );

    ASSERT_TRUE(
        table.add(
            make_table_route(
                "route-2",
                "route two"
            )
        ).accepted()
    );

    ASSERT_EQ(table.size(), 2);

    table.clear();

    EXPECT_TRUE(table.empty());
    EXPECT_EQ(table.size(), 0);
    EXPECT_TRUE(table.routes().empty());
}