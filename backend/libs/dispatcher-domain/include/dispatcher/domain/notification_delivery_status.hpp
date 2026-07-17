#pragma once

namespace dispatcher::domain
{
    enum class NotificationDeliveryStatus
    {
        Pending,
        Delivered,
        Failed,
        Skipped,
        RetryScheduled
    };

    [[nodiscard]] const char* to_string(
        NotificationDeliveryStatus status
    ) noexcept;

    [[nodiscard]] bool is_terminal(
        NotificationDeliveryStatus status
    ) noexcept;

    [[nodiscard]] bool is_success(
        NotificationDeliveryStatus status
    ) noexcept;

    [[nodiscard]] bool is_failure(
        NotificationDeliveryStatus status
    ) noexcept;

    [[nodiscard]] bool is_retryable(
        NotificationDeliveryStatus status
    ) noexcept;
}