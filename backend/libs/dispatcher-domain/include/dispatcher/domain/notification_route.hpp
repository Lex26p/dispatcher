#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/notification_message.hpp>
#include <dispatcher/domain/notification_priority.hpp>
#include <dispatcher/domain/notification_route_result.hpp>
#include <dispatcher/domain/notification_target.hpp>

#include <string>

namespace dispatcher::domain
{
    class NotificationRoute
    {
    public:
        NotificationRoute(
            NotificationRouteId route_id,
            std::string name,
            NotificationTarget target,
            NotificationPriority minimum_priority =
            NotificationPriority::Low,
            std::string source_filter = {},
            bool enabled = true
        );

        [[nodiscard]] const NotificationRouteId& route_id() const noexcept;

        [[nodiscard]] const std::string& name() const noexcept;

        [[nodiscard]] const NotificationTarget& target() const noexcept;

        [[nodiscard]] NotificationPriority minimum_priority() const noexcept;

        [[nodiscard]] const std::string& source_filter() const noexcept;

        [[nodiscard]] bool enabled() const noexcept;

        [[nodiscard]] bool disabled() const noexcept;

        [[nodiscard]] bool has_name() const noexcept;

        [[nodiscard]] bool has_source_filter() const noexcept;

        [[nodiscard]] bool valid() const noexcept;

        [[nodiscard]] bool accepts_priority(
            NotificationPriority priority
        ) const noexcept;

        [[nodiscard]] bool accepts_source(
            const std::string& source
        ) const noexcept;

        [[nodiscard]] NotificationRouteResult evaluate(
            const NotificationMessage& message
        ) const;

    private:
        NotificationRouteId route_id_;
        std::string name_;
        NotificationTarget target_;
        NotificationPriority minimum_priority_{ NotificationPriority::Low };
        std::string source_filter_;
        bool enabled_{ true };
    };
}