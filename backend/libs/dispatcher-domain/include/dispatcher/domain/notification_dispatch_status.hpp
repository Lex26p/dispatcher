#pragma once

namespace dispatcher::domain
{
    enum class NotificationDispatchStatus
    {
        Dispatched,
        NoRoutesConfigured,
        NoMatchingRoutes,
        InvalidMessage
    };

    [[nodiscard]] const char* to_string(
        NotificationDispatchStatus status
    ) noexcept;

    [[nodiscard]] bool is_success(
        NotificationDispatchStatus status
    ) noexcept;

    [[nodiscard]] bool is_failure(
        NotificationDispatchStatus status
    ) noexcept;
}