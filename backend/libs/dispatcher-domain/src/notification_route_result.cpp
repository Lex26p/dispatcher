#include <dispatcher/domain/notification_route_result.hpp>

#include <utility>

namespace dispatcher::domain
{
    NotificationRouteResult NotificationRouteResult::accepted(
        NotificationRouteId route_id,
        std::string message
    )
    {
        return NotificationRouteResult(
            std::move(route_id),
            NotificationRouteStatus::Accepted,
            std::move(message),
            {},
            {}
        );
    }

    NotificationRouteResult NotificationRouteResult::rejected(
        NotificationRouteId route_id,
        NotificationRouteStatus status,
        std::string reason,
        std::string field
    )
    {
        if (is_accepted(status))
        {
            status = NotificationRouteStatus::RejectedInvalidRoute;
        }

        return NotificationRouteResult(
            std::move(route_id),
            status,
            {},
            std::move(reason),
            std::move(field)
        );
    }

    bool NotificationRouteResult::accepted() const noexcept
    {
        return is_accepted(status_);
    }

    bool NotificationRouteResult::rejected() const noexcept
    {
        return !accepted();
    }

    NotificationRouteStatus NotificationRouteResult::status() const noexcept
    {
        return status_;
    }

    const NotificationRouteId& NotificationRouteResult::route_id()
        const noexcept
    {
        return route_id_;
    }

    const std::string& NotificationRouteResult::message() const noexcept
    {
        return message_;
    }

    const std::string& NotificationRouteResult::reason() const noexcept
    {
        return reason_;
    }

    const std::string& NotificationRouteResult::field() const noexcept
    {
        return field_;
    }

    bool NotificationRouteResult::has_message() const noexcept
    {
        return !message_.empty();
    }

    bool NotificationRouteResult::has_reason() const noexcept
    {
        return !reason_.empty();
    }

    bool NotificationRouteResult::has_field() const noexcept
    {
        return !field_.empty();
    }

    NotificationRouteResult::NotificationRouteResult(
        NotificationRouteId route_id,
        NotificationRouteStatus status,
        std::string message,
        std::string reason,
        std::string field
    )
        : route_id_(std::move(route_id))
        , status_(status)
        , message_(std::move(message))
        , reason_(std::move(reason))
        , field_(std::move(field))
    {
    }
}