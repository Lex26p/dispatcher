#include <dispatcher/domain/notification_dispatch_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::domain
{
    NotificationDispatchResult NotificationDispatchResult::dispatched(
        NotificationMessage message,
        NotificationRoutingResult routing_result,
        NotificationDeliveryResult delivery_result
    )
    {
        return NotificationDispatchResult(
            NotificationDispatchStatus::Dispatched,
            std::move(message),
            std::move(routing_result),
            std::move(delivery_result),
            {}
        );
    }

    NotificationDispatchResult NotificationDispatchResult::not_dispatched(
        NotificationDispatchStatus status,
        NotificationMessage message,
        NotificationRoutingResult routing_result,
        NotificationDeliveryResult delivery_result,
        std::string reason
    )
    {
        if (is_success(status))
        {
            status = NotificationDispatchStatus::InvalidMessage;
        }

        return NotificationDispatchResult(
            status,
            std::move(message),
            std::move(routing_result),
            std::move(delivery_result),
            std::move(reason)
        );
    }

    bool NotificationDispatchResult::ok() const noexcept
    {
        return is_success(status_);
    }

    bool NotificationDispatchResult::failed() const noexcept
    {
        return !ok();
    }

    NotificationDispatchStatus NotificationDispatchResult::status()
        const noexcept
    {
        return status_;
    }

    const NotificationMessage& NotificationDispatchResult::message() const
    {
        if (!message_.has_value())
        {
            throw std::logic_error(
                "NotificationDispatchResult does not contain a message"
            );
        }

        return message_.value();
    }

    bool NotificationDispatchResult::has_message() const noexcept
    {
        return message_.has_value();
    }

    const NotificationRoutingResult& NotificationDispatchResult::routing_result()
        const noexcept
    {
        return routing_result_;
    }

    const NotificationDeliveryResult& NotificationDispatchResult::delivery_result()
        const noexcept
    {
        return delivery_result_;
    }

    const std::string& NotificationDispatchResult::reason() const noexcept
    {
        return reason_;
    }

    bool NotificationDispatchResult::has_reason() const noexcept
    {
        return !reason_.empty();
    }

    std::size_t NotificationDispatchResult::accepted_route_count()
        const noexcept
    {
        return routing_result_.accepted_count();
    }

    std::size_t NotificationDispatchResult::rejected_route_count()
        const noexcept
    {
        return routing_result_.rejected_count();
    }

    std::size_t NotificationDispatchResult::delivery_attempt_count()
        const noexcept
    {
        return delivery_result_.attempt_count();
    }

    NotificationDispatchResult::NotificationDispatchResult(
        NotificationDispatchStatus status,
        std::optional<NotificationMessage> message,
        NotificationRoutingResult routing_result,
        NotificationDeliveryResult delivery_result,
        std::string reason
    )
        : status_(status)
        , message_(std::move(message))
        , routing_result_(std::move(routing_result))
        , delivery_result_(std::move(delivery_result))
        , reason_(std::move(reason))
    {
    }
}