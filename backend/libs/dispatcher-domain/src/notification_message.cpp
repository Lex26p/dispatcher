#include <dispatcher/domain/notification_message.hpp>

#include <utility>

namespace dispatcher::domain
{
    NotificationMessage::NotificationMessage(
        NotificationId notification_id,
        NotificationPriority priority,
        std::string subject,
        std::string body,
        std::string source,
        TimePoint created_at
    )
        : notification_id_(std::move(notification_id))
        , priority_(priority)
        , subject_(std::move(subject))
        , body_(std::move(body))
        , source_(std::move(source))
        , created_at_(created_at)
    {
    }

    NotificationMessage NotificationMessage::create(
        NotificationId notification_id,
        NotificationPriority priority,
        std::string subject,
        std::string body,
        std::string source
    )
    {
        return NotificationMessage(
            std::move(notification_id),
            priority,
            std::move(subject),
            std::move(body),
            std::move(source),
            Clock::now()
        );
    }

    const NotificationId& NotificationMessage::notification_id()
        const noexcept
    {
        return notification_id_;
    }

    NotificationPriority NotificationMessage::priority() const noexcept
    {
        return priority_;
    }

    const std::string& NotificationMessage::subject() const noexcept
    {
        return subject_;
    }

    const std::string& NotificationMessage::body() const noexcept
    {
        return body_;
    }

    const std::string& NotificationMessage::source() const noexcept
    {
        return source_;
    }

    NotificationMessage::TimePoint NotificationMessage::created_at()
        const noexcept
    {
        return created_at_;
    }

    bool NotificationMessage::has_subject() const noexcept
    {
        return !subject_.empty();
    }

    bool NotificationMessage::has_body() const noexcept
    {
        return !body_.empty();
    }

    bool NotificationMessage::has_source() const noexcept
    {
        return !source_.empty();
    }

    bool NotificationMessage::empty() const noexcept
    {
        return subject_.empty()
            && body_.empty();
    }

    bool NotificationMessage::valid() const noexcept
    {
        return !notification_id_.value().empty()
            && !empty();
    }

    bool NotificationMessage::urgent() const noexcept
    {
        return is_urgent(priority_);
    }

    bool NotificationMessage::requires_attention() const noexcept
    {
        return requires_operator_attention(priority_);
    }
}