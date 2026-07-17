#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/notification_routing.hpp>

#include <gtest/gtest.h>

TEST(NotificationRoutingHeaderTests, UmbrellaHeaderCanBeIncluded)
{
    dispatcher::domain::NotificationDispatcher dispatcher;

    const auto add_result =
        dispatcher.add_route(
            dispatcher::domain::NotificationRoute(
                dispatcher::domain::NotificationRouteId{ "route-1" },
                "primary email route",
                dispatcher::domain::NotificationTarget(
                    dispatcher::domain::NotificationChannel::Email,
                    "operator@example.local",
                    "Operator"
                ),
                dispatcher::domain::NotificationPriority::Low,
                "alarm-runtime",
                true
            )
        );

    ASSERT_TRUE(add_result.accepted());

    const auto message =
        dispatcher::domain::NotificationMessage::create(
            dispatcher::domain::NotificationId{ "notification-1" },
            dispatcher::domain::NotificationPriority::Critical,
            "Critical alarm",
            "Critical pressure alarm is active",
            "alarm-runtime"
        );

    const auto dispatch_result =
        dispatcher.dispatch(message);

    EXPECT_TRUE(dispatch_result.ok());

    EXPECT_EQ(
        dispatch_result.status(),
        dispatcher::domain::NotificationDispatchStatus::Dispatched
    );

    EXPECT_EQ(dispatch_result.accepted_route_count(), 1);
    EXPECT_EQ(dispatch_result.delivery_attempt_count(), 1);

    ASSERT_EQ(dispatch_result.delivery_result().attempts().size(), 1);

    EXPECT_EQ(
        dispatch_result.delivery_result().attempts().front().status(),
        dispatcher::domain::NotificationDeliveryStatus::Pending
    );

    EXPECT_EQ(
        dispatcher::domain::to_string(
            dispatch_result.delivery_result().attempts().front().target().channel()
        ),
        std::string{ "email" }
    );
}