#include <dispatcher/domain/notification_delivery_attempt.hpp>

#include <utility>

namespace dispatcher::domain
{
    NotificationDeliveryAttempt::NotificationDeliveryAttempt(
        NotificationMessage message,
        NotificationRoute route,
        NotificationTarget target,
        NotificationDeliveryStatus status,
        std::string detail,
        TimePoint attempted_at
    )
        : message_(std::move(message))
        , route_(std::move(route))
        , target_(std::move(target))
        , status_(status)
        , detail_(std::move(detail))
        , attempted_at_(attempted_at)
    {
    }

    NotificationDeliveryAttempt NotificationDeliveryAttempt::pending(
        NotificationMessage message,
        NotificationRoute route
    )
    {
        auto target = route.target();

        return NotificationDeliveryAttempt(
            std::move(message),
            std::move(route),
            std::move(target),
            NotificationDeliveryStatus::Pending,
            {},
            Clock::now()
        );
    }

    NotificationDeliveryAttempt NotificationDeliveryAttempt::delivered(
        NotificationMessage message,
        NotificationRoute route,
        std::string detail
    )
    {
        auto target = route.target();

        return NotificationDeliveryAttempt(
            std::move(message),
            std::move(route),
            std::move(target),
            NotificationDeliveryStatus::Delivered,
            std::move(detail),
            Clock::now()
        );
    }

    NotificationDeliveryAttempt NotificationDeliveryAttempt::failed(
        NotificationMessage message,
        NotificationRoute route,
        std::string detail
    )
    {
        auto target = route.target();

        return NotificationDeliveryAttempt(
            std::move(message),
            std::move(route),
            std::move(target),
            NotificationDeliveryStatus::Failed,
            std::move(detail),
            Clock::now()
        );
    }

    NotificationDeliveryAttempt NotificationDeliveryAttempt::skipped(
        NotificationMessage message,
        NotificationRoute route,
        std::string detail
    )
    {
        auto target = route.target();

        return NotificationDeliveryAttempt(
            std::move(message),
            std::move(route),
            std::move(target),
            NotificationDeliveryStatus::Skipped,
            std::move(detail),
            Clock::now()
        );
    }

    const NotificationMessage& NotificationDeliveryAttempt::message()
        const noexcept
    {
        return message_;
    }

    const NotificationRoute& NotificationDeliveryAttempt::route()
        const noexcept
    {
        return route_;
    }

    const NotificationTarget& NotificationDeliveryAttempt::target()
        const noexcept
    {
        return target_;
    }

    NotificationDeliveryStatus NotificationDeliveryAttempt::status()
        const noexcept
    {
        return status_;
    }

    const std::string& NotificationDeliveryAttempt::detail() const noexcept
    {
        return detail_;
    }

    NotificationDeliveryAttempt::TimePoint
        NotificationDeliveryAttempt::attempted_at() const noexcept
    {
        return attempted_at_;
    }

    bool NotificationDeliveryAttempt::has_detail() const noexcept
    {
        return !detail_.empty();
    }

    bool NotificationDeliveryAttempt::terminal() const noexcept
    {
        return is_terminal(status_);
    }

    bool NotificationDeliveryAttempt::success() const noexcept
    {
        return is_success(status_);
    }

    bool NotificationDeliveryAttempt::failure() const noexcept
    {
        return is_failure(status_);
    }

    bool NotificationDeliveryAttempt::retryable() const noexcept
    {
        return is_retryable(status_);
    }
}