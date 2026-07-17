#pragma once

#include <dispatcher/domain/notification_channel.hpp>

#include <string>

namespace dispatcher::domain
{
    class NotificationTarget
    {
    public:
        NotificationTarget(
            NotificationChannel channel,
            std::string address = {},
            std::string display_name = {},
            bool enabled = true
        );

        [[nodiscard]] NotificationChannel channel() const noexcept;

        [[nodiscard]] const std::string& address() const noexcept;

        [[nodiscard]] const std::string& display_name() const noexcept;

        [[nodiscard]] bool enabled() const noexcept;

        [[nodiscard]] bool disabled() const noexcept;

        [[nodiscard]] bool has_address() const noexcept;

        [[nodiscard]] bool has_display_name() const noexcept;

        [[nodiscard]] bool valid() const noexcept;

        [[nodiscard]] bool external() const noexcept;

        [[nodiscard]] bool internal() const noexcept;

    private:
        NotificationChannel channel_{ NotificationChannel::Unknown };
        std::string address_;
        std::string display_name_;
        bool enabled_{ true };
    };
}