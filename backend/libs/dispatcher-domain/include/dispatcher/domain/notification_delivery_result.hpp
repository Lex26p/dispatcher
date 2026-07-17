#pragma once

#include <dispatcher/domain/notification_delivery_attempt.hpp>
#include <dispatcher/domain/notification_delivery_status.hpp>

#include <cstddef>
#include <vector>

namespace dispatcher::domain
{
    class NotificationDeliveryResult
    {
    public:
        void record(
            NotificationDeliveryAttempt attempt
        );

        [[nodiscard]] const std::vector<NotificationDeliveryAttempt>&
            attempts() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool has_attempts() const noexcept;

        [[nodiscard]] std::size_t attempt_count() const noexcept;

        [[nodiscard]] std::size_t delivered_count() const noexcept;

        [[nodiscard]] std::size_t failed_count() const noexcept;

        [[nodiscard]] std::size_t skipped_count() const noexcept;

        [[nodiscard]] std::size_t retryable_count() const noexcept;

        [[nodiscard]] bool all_delivered() const noexcept;

        [[nodiscard]] bool any_failed() const noexcept;

        [[nodiscard]] bool any_retryable() const noexcept;

    private:
        std::vector<NotificationDeliveryAttempt> attempts_;
    };
}