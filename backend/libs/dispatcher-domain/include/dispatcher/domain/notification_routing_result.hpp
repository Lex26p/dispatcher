#pragma once

#include <dispatcher/domain/notification_route.hpp>
#include <dispatcher/domain/notification_route_result.hpp>

#include <cstddef>
#include <vector>

namespace dispatcher::domain
{
    class NotificationRoutingResult
    {
    public:
        void record(
            NotificationRoute route,
            NotificationRouteResult result
        );

        [[nodiscard]] const std::vector<NotificationRoute>& accepted_routes()
            const noexcept;

        [[nodiscard]] const std::vector<NotificationRouteResult>& results()
            const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool matched() const noexcept;

        [[nodiscard]] bool unmatched() const noexcept;

        [[nodiscard]] std::size_t accepted_count() const noexcept;

        [[nodiscard]] std::size_t rejected_count() const noexcept;

        [[nodiscard]] std::size_t evaluated_count() const noexcept;

    private:
        std::vector<NotificationRoute> accepted_routes_;
        std::vector<NotificationRouteResult> results_;
    };
}