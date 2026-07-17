#pragma once

namespace dispatcher::domain
{
    enum class NotificationChannel
    {
        Unknown,
        Console,
        Email,
        Sms,
        Webhook,
        AuditLog
    };

    [[nodiscard]] const char* to_string(
        NotificationChannel channel
    ) noexcept;

    [[nodiscard]] bool is_known_channel(
        NotificationChannel channel
    ) noexcept;

    [[nodiscard]] bool requires_address(
        NotificationChannel channel
    ) noexcept;

    [[nodiscard]] bool is_external_channel(
        NotificationChannel channel
    ) noexcept;

    [[nodiscard]] bool is_internal_channel(
        NotificationChannel channel
    ) noexcept;
}