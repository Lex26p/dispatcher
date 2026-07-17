#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/notification_dispatch_result.hpp>
#include <dispatcher/domain/notification_message.hpp>
#include <dispatcher/domain/notification_route.hpp>
#include <dispatcher/domain/notification_route_result.hpp>
#include <dispatcher/domain/notification_route_table.hpp>

namespace dispatcher::domain
{
    class NotificationDispatcher
    {
    public:
        NotificationDispatcher() = default;

        explicit NotificationDispatcher(
            NotificationRouteTable route_table
        );

        [[nodiscard]] const NotificationRouteTable& route_table()
            const noexcept;

        [[nodiscard]] NotificationRouteTable& route_table() noexcept;

        void set_route_table(
            NotificationRouteTable route_table
        );

        [[nodiscard]] NotificationRouteResult add_route(
            NotificationRoute route
        );

        [[nodiscard]] NotificationRouteResult remove_route(
            const NotificationRouteId& route_id
        );

        [[nodiscard]] NotificationDispatchResult dispatch(
            NotificationMessage message
        ) const;

    private:
        NotificationRouteTable route_table_;
    };
}