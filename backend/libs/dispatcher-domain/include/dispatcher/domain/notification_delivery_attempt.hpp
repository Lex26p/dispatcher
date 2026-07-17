#pragma once

#include <dispatcher/domain/notification_delivery_status.hpp>
#include <dispatcher/domain/notification_message.hpp>
#include <dispatcher/domain/notification_route.hpp>
#include <dispatcher/domain/notification_target.hpp>

#include <chrono>
#include <string>

namespace dispatcher::domain
{
    class NotificationDeliveryAttempt
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

        NotificationDeliveryAttempt(
            NotificationMessage message,
            NotificationRoute route,
            NotificationTarget target,
            NotificationDeliveryStatus status = NotificationDeliveryStatus::Pending,
            std::string detail = {},
            TimePoint attempted_at = Clock::now()
        );

        [[nodiscard]] static NotificationDeliveryAttempt pending(
            NotificationMessage message,
            NotificationRoute route
        );

        [[nodiscard]] static NotificationDeliveryAttempt delivered(
            NotificationMessage message,
            NotificationRoute route,
            std::string detail = {}
        );

        [[nodiscard]] static NotificationDeliveryAttempt failed(
            NotificationMessage message,
            NotificationRoute route,
            std::string detail = {}
        );

        [[nodiscard]] static NotificationDeliveryAttempt skipped(
            NotificationMessage message,
            NotificationRoute route,
            std::string detail = {}
        );

        [[nodiscard]] const NotificationMessage& message() const noexcept;

        [[nodiscard]] const NotificationRoute& route() const noexcept;

        [[nodiscard]] const NotificationTarget& target() const noexcept;

        [[nodiscard]] NotificationDeliveryStatus status() const noexcept;

        [[nodiscard]] const std::string& detail() const noexcept;

        [[nodiscard]] TimePoint attempted_at() const noexcept;

        [[nodiscard]] bool has_detail() const noexcept;

        [[nodiscard]] bool terminal() const noexcept;

        [[nodiscard]] bool success() const noexcept;

        [[nodiscard]] bool failure() const noexcept;

        [[nodiscard]] bool retryable() const noexcept;

    private:
        NotificationMessage message_;
        NotificationRoute route_;
        NotificationTarget target_;
        NotificationDeliveryStatus status_{ NotificationDeliveryStatus::Pending };
        std::string detail_;
        TimePoint attempted_at_{ Clock::now() };
    };
}