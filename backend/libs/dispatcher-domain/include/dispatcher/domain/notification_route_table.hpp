#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/notification_message.hpp>
#include <dispatcher/domain/notification_route.hpp>
#include <dispatcher/domain/notification_routing_result.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace dispatcher::domain
{
    class NotificationRouteTable
    {
    public:
        [[nodiscard]] NotificationRouteResult add(
            NotificationRoute route
        );

        [[nodiscard]] NotificationRouteResult remove(
            const NotificationRouteId& route_id
        );

        [[nodiscard]] std::optional<NotificationRoute> find(
            const NotificationRouteId& route_id
        ) const;

        [[nodiscard]] bool contains(
            const NotificationRouteId& route_id
        ) const;

        [[nodiscard]] std::vector<NotificationRoute> routes() const;

        [[nodiscard]] NotificationRoutingResult route(
            const NotificationMessage& message
        ) const;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        void clear() noexcept;

    private:
        std::unordered_map<std::string, NotificationRoute> routes_by_id_;
    };
}