#include <dispatcher/domain/notification_dispatcher.hpp>

#include <utility>

namespace dispatcher::domain
{
    NotificationDispatcher::NotificationDispatcher(
        NotificationRouteTable route_table
    )
        : route_table_(std::move(route_table))
    {
    }

    const NotificationRouteTable& NotificationDispatcher::route_table()
        const noexcept
    {
        return route_table_;
    }

    NotificationRouteTable& NotificationDispatcher::route_table() noexcept
    {
        return route_table_;
    }

    void NotificationDispatcher::set_route_table(
        NotificationRouteTable route_table
    )
    {
        route_table_ = std::move(route_table);
    }

    NotificationRouteResult NotificationDispatcher::add_route(
        NotificationRoute route
    )
    {
        return route_table_.add(
            std::move(route)
        );
    }

    NotificationRouteResult NotificationDispatcher::remove_route(
        const NotificationRouteId& route_id
    )
    {
        return route_table_.remove(route_id);
    }

    NotificationDispatchResult NotificationDispatcher::dispatch(
        NotificationMessage message
    ) const
    {
        if (!message.valid())
        {
            return NotificationDispatchResult::not_dispatched(
                NotificationDispatchStatus::InvalidMessage,
                std::move(message),
                {},
                {},
                "notification message is invalid"
            );
        }

        auto routing_result = route_table_.route(message);

        if (routing_result.empty())
        {
            return NotificationDispatchResult::not_dispatched(
                NotificationDispatchStatus::NoRoutesConfigured,
                std::move(message),
                std::move(routing_result),
                {},
                "no notification routes configured"
            );
        }

        if (routing_result.unmatched())
        {
            return NotificationDispatchResult::not_dispatched(
                NotificationDispatchStatus::NoMatchingRoutes,
                std::move(message),
                std::move(routing_result),
                {},
                "no notification routes matched message"
            );
        }

        NotificationDeliveryResult delivery_result;

        for (const auto& route : routing_result.accepted_routes())
        {
            delivery_result.record(
                NotificationDeliveryAttempt::pending(
                    message,
                    route
                )
            );
        }

        return NotificationDispatchResult::dispatched(
            std::move(message),
            std::move(routing_result),
            std::move(delivery_result)
        );
    }
}