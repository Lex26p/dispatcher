#include <dispatcher/domain/notification_routing_result.hpp>

#include <utility>

namespace dispatcher::domain
{
    void NotificationRoutingResult::record(
        NotificationRoute route,
        NotificationRouteResult result
    )
    {
        if (result.accepted())
        {
            accepted_routes_.push_back(
                std::move(route)
            );
        }

        results_.push_back(
            std::move(result)
        );
    }

    const std::vector<NotificationRoute>&
        NotificationRoutingResult::accepted_routes() const noexcept
    {
        return accepted_routes_;
    }

    const std::vector<NotificationRouteResult>&
        NotificationRoutingResult::results() const noexcept
    {
        return results_;
    }

    bool NotificationRoutingResult::empty() const noexcept
    {
        return results_.empty();
    }

    bool NotificationRoutingResult::matched() const noexcept
    {
        return !accepted_routes_.empty();
    }

    bool NotificationRoutingResult::unmatched() const noexcept
    {
        return !matched();
    }

    std::size_t NotificationRoutingResult::accepted_count() const noexcept
    {
        return accepted_routes_.size();
    }

    std::size_t NotificationRoutingResult::rejected_count() const noexcept
    {
        return results_.size() - accepted_routes_.size();
    }

    std::size_t NotificationRoutingResult::evaluated_count() const noexcept
    {
        return results_.size();
    }
}