#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/notification_route_status.hpp>

#include <string>

namespace dispatcher::domain
{
    class NotificationRouteResult
    {
    public:
        [[nodiscard]] static NotificationRouteResult accepted(
            NotificationRouteId route_id,
            std::string message = {}
        );

        [[nodiscard]] static NotificationRouteResult rejected(
            NotificationRouteId route_id,
            NotificationRouteStatus status,
            std::string reason = {},
            std::string field = {}
        );

        [[nodiscard]] bool accepted() const noexcept;

        [[nodiscard]] bool rejected() const noexcept;

        [[nodiscard]] NotificationRouteStatus status() const noexcept;

        [[nodiscard]] const NotificationRouteId& route_id() const noexcept;

        [[nodiscard]] const std::string& message() const noexcept;

        [[nodiscard]] const std::string& reason() const noexcept;

        [[nodiscard]] const std::string& field() const noexcept;

        [[nodiscard]] bool has_message() const noexcept;

        [[nodiscard]] bool has_reason() const noexcept;

        [[nodiscard]] bool has_field() const noexcept;

    private:
        NotificationRouteResult(
            NotificationRouteId route_id,
            NotificationRouteStatus status,
            std::string message,
            std::string reason,
            std::string field
        );

        NotificationRouteId route_id_;
        NotificationRouteStatus status_{
            NotificationRouteStatus::RejectedInvalidRoute
        };
        std::string message_;
        std::string reason_;
        std::string field_;
    };
}