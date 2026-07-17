#include <dispatcher/domain/notification_target.hpp>

#include <utility>

namespace dispatcher::domain
{
    NotificationTarget::NotificationTarget(
        NotificationChannel channel,
        std::string address,
        std::string display_name,
        bool enabled
    )
        : channel_(channel)
        , address_(std::move(address))
        , display_name_(std::move(display_name))
        , enabled_(enabled)
    {
    }

    NotificationChannel NotificationTarget::channel() const noexcept
    {
        return channel_;
    }

    const std::string& NotificationTarget::address() const noexcept
    {
        return address_;
    }

    const std::string& NotificationTarget::display_name() const noexcept
    {
        return display_name_;
    }

    bool NotificationTarget::enabled() const noexcept
    {
        return enabled_;
    }

    bool NotificationTarget::disabled() const noexcept
    {
        return !enabled_;
    }

    bool NotificationTarget::has_address() const noexcept
    {
        return !address_.empty();
    }

    bool NotificationTarget::has_display_name() const noexcept
    {
        return !display_name_.empty();
    }

    bool NotificationTarget::valid() const noexcept
    {
        return enabled_
            && is_known_channel(channel_)
            && (!requires_address(channel_) || has_address());
    }

    bool NotificationTarget::external() const noexcept
    {
        return is_external_channel(channel_);
    }

    bool NotificationTarget::internal() const noexcept
    {
        return is_internal_channel(channel_);
    }
}