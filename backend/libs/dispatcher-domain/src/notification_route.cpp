#include <dispatcher/domain/notification_route.hpp>

#include <utility>

namespace dispatcher::domain
{
    NotificationRoute::NotificationRoute(
        NotificationRouteId route_id,
        std::string name,
        NotificationTarget target,
        NotificationPriority minimum_priority,
        std::string source_filter,
        bool enabled
    )
        : route_id_(std::move(route_id))
        , name_(std::move(name))
        , target_(std::move(target))
        , minimum_priority_(minimum_priority)
        , source_filter_(std::move(source_filter))
        , enabled_(enabled)
    {
    }

    const NotificationRouteId& NotificationRoute::route_id() const noexcept
    {
        return route_id_;
    }

    const std::string& NotificationRoute::name() const noexcept
    {
        return name_;
    }

    const NotificationTarget& NotificationRoute::target() const noexcept
    {
        return target_;
    }

    NotificationPriority NotificationRoute::minimum_priority() const noexcept
    {
        return minimum_priority_;
    }

    const std::string& NotificationRoute::source_filter() const noexcept
    {
        return source_filter_;
    }

    bool NotificationRoute::enabled() const noexcept
    {
        return enabled_;
    }

    bool NotificationRoute::disabled() const noexcept
    {
        return !enabled_;
    }

    bool NotificationRoute::has_name() const noexcept
    {
        return !name_.empty();
    }

    bool NotificationRoute::has_source_filter() const noexcept
    {
        return !source_filter_.empty();
    }

    bool NotificationRoute::valid() const noexcept
    {
        return !route_id_.value().empty()
            && has_name()
            && target_.valid();
    }

    bool NotificationRoute::accepts_priority(
        NotificationPriority priority
    ) const noexcept
    {
        return delivery_order(priority)
            <= delivery_order(minimum_priority_);
    }

    bool NotificationRoute::accepts_source(
        const std::string& source
    ) const noexcept
    {
        return source_filter_.empty()
            || source_filter_ == source;
    }

    NotificationRouteResult NotificationRoute::evaluate(
        const NotificationMessage& message
    ) const
    {
        if (!valid())
        {
            return NotificationRouteResult::rejected(
                NotificationRouteId{ route_id_.value() },
                NotificationRouteStatus::RejectedInvalidRoute,
                "route is invalid",
                "route"
            );
        }

        if (!enabled_)
        {
            return NotificationRouteResult::rejected(
                NotificationRouteId{ route_id_.value() },
                NotificationRouteStatus::RejectedDisabledRoute,
                "route is disabled",
                "enabled"
            );
        }

        if (!target_.valid())
        {
            return NotificationRouteResult::rejected(
                NotificationRouteId{ route_id_.value() },
                NotificationRouteStatus::RejectedTargetInvalid,
                "route target is invalid",
                "target"
            );
        }

        if (!accepts_priority(message.priority()))
        {
            return NotificationRouteResult::rejected(
                NotificationRouteId{ route_id_.value() },
                NotificationRouteStatus::RejectedPriorityTooLow,
                "message priority is below route minimum",
                "priority"
            );
        }

        if (!accepts_source(message.source()))
        {
            return NotificationRouteResult::rejected(
                NotificationRouteId{ route_id_.value() },
                NotificationRouteStatus::RejectedSourceMismatch,
                "message source does not match route source filter",
                "source"
            );
        }

        return NotificationRouteResult::accepted(
            NotificationRouteId{ route_id_.value() },
            "route accepted notification message"
        );
    }
}