#pragma once

#include <cstdint>

namespace dispatcher::domain
{
    enum class NotificationPriority
    {
        Low,
        Normal,
        High,
        Critical
    };

    [[nodiscard]] const char* to_string(
        NotificationPriority priority
    ) noexcept;

    [[nodiscard]] std::uint32_t delivery_order(
        NotificationPriority priority
    ) noexcept;

    [[nodiscard]] bool is_urgent(
        NotificationPriority priority
    ) noexcept;

    [[nodiscard]] bool requires_operator_attention(
        NotificationPriority priority
    ) noexcept;
}