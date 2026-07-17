#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/notification_priority.hpp>

#include <chrono>
#include <string>

namespace dispatcher::domain
{
    class NotificationMessage
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

        NotificationMessage(
            NotificationId notification_id,
            NotificationPriority priority,
            std::string subject,
            std::string body,
            std::string source = {},
            TimePoint created_at = Clock::now()
        );

        [[nodiscard]] static NotificationMessage create(
            NotificationId notification_id,
            NotificationPriority priority,
            std::string subject,
            std::string body,
            std::string source = {}
        );

        [[nodiscard]] const NotificationId& notification_id() const noexcept;

        [[nodiscard]] NotificationPriority priority() const noexcept;

        [[nodiscard]] const std::string& subject() const noexcept;

        [[nodiscard]] const std::string& body() const noexcept;

        [[nodiscard]] const std::string& source() const noexcept;

        [[nodiscard]] TimePoint created_at() const noexcept;

        [[nodiscard]] bool has_subject() const noexcept;

        [[nodiscard]] bool has_body() const noexcept;

        [[nodiscard]] bool has_source() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool valid() const noexcept;

        [[nodiscard]] bool urgent() const noexcept;

        [[nodiscard]] bool requires_attention() const noexcept;

    private:
        NotificationId notification_id_;
        NotificationPriority priority_{ NotificationPriority::Normal };
        std::string subject_;
        std::string body_;
        std::string source_;
        TimePoint created_at_{ Clock::now() };
    };
}