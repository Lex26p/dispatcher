#include <dispatcher/domain/notification_route_table.hpp>

#include <algorithm>
#include <utility>

namespace dispatcher::domain
{
    NotificationRouteResult NotificationRouteTable::add(
        NotificationRoute route
    )
    {
        const auto route_id = route.route_id().value();

        if (route_id.empty())
        {
            return NotificationRouteResult::rejected(
                NotificationRouteId{},
                NotificationRouteStatus::RejectedInvalidRoute,
                "route id is empty",
                "route_id"
            );
        }

        if (!route.valid())
        {
            return NotificationRouteResult::rejected(
                NotificationRouteId{ route_id },
                NotificationRouteStatus::RejectedInvalidRoute,
                "route is invalid",
                "route"
            );
        }

        if (routes_by_id_.contains(route_id))
        {
            return NotificationRouteResult::rejected(
                NotificationRouteId{ route_id },
                NotificationRouteStatus::RejectedInvalidRoute,
                "route id is already registered",
                "route_id"
            );
        }

        routes_by_id_.emplace(
            route_id,
            std::move(route)
        );

        return NotificationRouteResult::accepted(
            NotificationRouteId{ route_id },
            "route added"
        );
    }

    NotificationRouteResult NotificationRouteTable::remove(
        const NotificationRouteId& route_id
    )
    {
        const auto route_id_value = route_id.value();

        if (route_id_value.empty())
        {
            return NotificationRouteResult::rejected(
                NotificationRouteId{},
                NotificationRouteStatus::RejectedInvalidRoute,
                "route id is empty",
                "route_id"
            );
        }

        const auto erased_count =
            routes_by_id_.erase(route_id_value);

        if (erased_count == 0)
        {
            return NotificationRouteResult::rejected(
                NotificationRouteId{ route_id_value },
                NotificationRouteStatus::RejectedInvalidRoute,
                "route not found",
                "route_id"
            );
        }

        return NotificationRouteResult::accepted(
            NotificationRouteId{ route_id_value },
            "route removed"
        );
    }

    std::optional<NotificationRoute> NotificationRouteTable::find(
        const NotificationRouteId& route_id
    ) const
    {
        const auto iterator =
            routes_by_id_.find(route_id.value());

        if (iterator == routes_by_id_.end())
        {
            return std::nullopt;
        }

        return iterator->second;
    }

    bool NotificationRouteTable::contains(
        const NotificationRouteId& route_id
    ) const
    {
        return routes_by_id_.contains(route_id.value());
    }

    std::vector<NotificationRoute> NotificationRouteTable::routes() const
    {
        std::vector<NotificationRoute> result;

        result.reserve(routes_by_id_.size());

        for (const auto& [id, route] : routes_by_id_)
        {
            result.push_back(route);
        }

        std::sort(
            result.begin(),
            result.end(),
            [](const NotificationRoute& left, const NotificationRoute& right)
            {
                return left.route_id().value()
                    < right.route_id().value();
            }
        );

        return result;
    }

    NotificationRoutingResult NotificationRouteTable::route(
        const NotificationMessage& message
    ) const
    {
        NotificationRoutingResult result;

        for (const auto& route : routes())
        {
            auto route_result = route.evaluate(message);

            result.record(
                route,
                std::move(route_result)
            );
        }

        return result;
    }

    std::size_t NotificationRouteTable::size() const noexcept
    {
        return routes_by_id_.size();
    }

    bool NotificationRouteTable::empty() const noexcept
    {
        return routes_by_id_.empty();
    }

    void NotificationRouteTable::clear() noexcept
    {
        routes_by_id_.clear();
    }
}