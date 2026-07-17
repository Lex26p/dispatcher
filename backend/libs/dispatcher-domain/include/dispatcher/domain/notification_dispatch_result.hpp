#pragma once

#include <dispatcher/domain/notification_delivery_result.hpp>
#include <dispatcher/domain/notification_dispatch_status.hpp>
#include <dispatcher/domain/notification_message.hpp>
#include <dispatcher/domain/notification_routing_result.hpp>

#include <cstddef>
#include <optional>
#include <string>

namespace dispatcher::domain
{
    class NotificationDispatchResult
    {
    public:
        [[nodiscard]] static NotificationDispatchResult dispatched(
            NotificationMessage message,
            NotificationRoutingResult routing_result,
            NotificationDeliveryResult delivery_result
        );

        [[nodiscard]] static NotificationDispatchResult not_dispatched(
            NotificationDispatchStatus status,
            NotificationMessage message,
            NotificationRoutingResult routing_result = {},
            NotificationDeliveryResult delivery_result = {},
            std::string reason = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] NotificationDispatchStatus status() const noexcept;

        [[nodiscard]] const NotificationMessage& message() const;

        [[nodiscard]] bool has_message() const noexcept;

        [[nodiscard]] const NotificationRoutingResult& routing_result()
            const noexcept;

        [[nodiscard]] const NotificationDeliveryResult& delivery_result()
            const noexcept;

        [[nodiscard]] const std::string& reason() const noexcept;

        [[nodiscard]] bool has_reason() const noexcept;

        [[nodiscard]] std::size_t accepted_route_count() const noexcept;

        [[nodiscard]] std::size_t rejected_route_count() const noexcept;

        [[nodiscard]] std::size_t delivery_attempt_count() const noexcept;

    private:
        NotificationDispatchResult(
            NotificationDispatchStatus status,
            std::optional<NotificationMessage> message,
            NotificationRoutingResult routing_result,
            NotificationDeliveryResult delivery_result,
            std::string reason
        );

        NotificationDispatchStatus status_{
            NotificationDispatchStatus::InvalidMessage
        };
        std::optional<NotificationMessage> message_;
        NotificationRoutingResult routing_result_;
        NotificationDeliveryResult delivery_result_;
        std::string reason_;
    };
}