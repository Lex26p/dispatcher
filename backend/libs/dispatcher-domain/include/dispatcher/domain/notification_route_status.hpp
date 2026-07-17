#pragma once

namespace dispatcher::domain
{
    enum class NotificationRouteStatus
    {
        Accepted,
        RejectedInvalidRoute,
        RejectedDisabledRoute,
        RejectedPriorityTooLow,
        RejectedSourceMismatch,
        RejectedTargetInvalid
    };

    [[nodiscard]] const char* to_string(
        NotificationRouteStatus status
    ) noexcept;

    [[nodiscard]] bool is_accepted(
        NotificationRouteStatus status
    ) noexcept;

    [[nodiscard]] bool is_rejected(
        NotificationRouteStatus status
    ) noexcept;
}