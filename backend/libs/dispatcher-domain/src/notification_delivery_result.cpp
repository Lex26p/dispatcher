#include <dispatcher/domain/notification_delivery_result.hpp>

#include <algorithm>
#include <utility>

namespace dispatcher::domain
{
    void NotificationDeliveryResult::record(
        NotificationDeliveryAttempt attempt
    )
    {
        attempts_.push_back(
            std::move(attempt)
        );
    }

    const std::vector<NotificationDeliveryAttempt>&
        NotificationDeliveryResult::attempts() const noexcept
    {
        return attempts_;
    }

    bool NotificationDeliveryResult::empty() const noexcept
    {
        return attempts_.empty();
    }

    bool NotificationDeliveryResult::has_attempts() const noexcept
    {
        return !empty();
    }

    std::size_t NotificationDeliveryResult::attempt_count() const noexcept
    {
        return attempts_.size();
    }

    std::size_t NotificationDeliveryResult::delivered_count() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                attempts_.begin(),
                attempts_.end(),
                [](const NotificationDeliveryAttempt& attempt)
                {
                    return attempt.status()
                        == NotificationDeliveryStatus::Delivered;
                }
            )
            );
    }

    std::size_t NotificationDeliveryResult::failed_count() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                attempts_.begin(),
                attempts_.end(),
                [](const NotificationDeliveryAttempt& attempt)
                {
                    return attempt.status()
                        == NotificationDeliveryStatus::Failed;
                }
            )
            );
    }

    std::size_t NotificationDeliveryResult::skipped_count() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                attempts_.begin(),
                attempts_.end(),
                [](const NotificationDeliveryAttempt& attempt)
                {
                    return attempt.status()
                        == NotificationDeliveryStatus::Skipped;
                }
            )
            );
    }

    std::size_t NotificationDeliveryResult::retryable_count() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                attempts_.begin(),
                attempts_.end(),
                [](const NotificationDeliveryAttempt& attempt)
                {
                    return attempt.retryable();
                }
            )
            );
    }

    bool NotificationDeliveryResult::all_delivered() const noexcept
    {
        return has_attempts()
            && delivered_count() == attempt_count();
    }

    bool NotificationDeliveryResult::any_failed() const noexcept
    {
        return failed_count() > 0;
    }

    bool NotificationDeliveryResult::any_retryable() const noexcept
    {
        return retryable_count() > 0;
    }
}