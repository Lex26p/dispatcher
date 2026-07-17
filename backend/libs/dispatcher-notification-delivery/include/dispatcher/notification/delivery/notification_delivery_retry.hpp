#pragma once

#include <dispatcher/notification/delivery/notification_delivery_dispatcher.hpp>
#include <dispatcher/notification/delivery/notification_delivery_types.hpp>

#include <chrono>
#include <cstdint>
#include <vector>

namespace dispatcher::notification::delivery
{
    struct NotificationDeliveryRetryPolicy
    {
        std::uint32_t max_attempts{ 3 };

        bool retry_failed{ true };
        bool retry_skipped{ false };
    };

    struct NotificationDeliveryAttempt
    {
        std::uint32_t attempt_number{ 0 };

        NotificationDeliveryStatus status{
            NotificationDeliveryStatus::pending
        };

        std::string provider_message_id{};
        std::string error_message{};
        std::string diagnostic_message{};

        std::chrono::system_clock::time_point started_at{
            std::chrono::system_clock::now()
        };

        std::chrono::system_clock::time_point completed_at{
            std::chrono::system_clock::now()
        };
    };

    struct NotificationDeliveryExecutionResult
    {
        NotificationDeliveryResult final_result{};
        std::vector<NotificationDeliveryAttempt> attempts{};

        [[nodiscard]] bool success() const noexcept
        {
            return final_result.success();
        }

        [[nodiscard]] bool failure() const noexcept
        {
            return final_result.failure();
        }

        [[nodiscard]] std::size_t attempt_count() const noexcept
        {
            return attempts.size();
        }
    };

    class NotificationDeliveryRetryExecutor final
    {
    public:
        explicit NotificationDeliveryRetryExecutor(
            NotificationDeliveryDispatcher& dispatcher,
            NotificationDeliveryRetryPolicy policy = {}
        );

        [[nodiscard]] const NotificationDeliveryRetryPolicy& policy() const noexcept;

        static void validate_policy(
            const NotificationDeliveryRetryPolicy& policy
        );

        [[nodiscard]] NotificationDeliveryExecutionResult deliver_with_retry(
            const NotificationDeliveryMessage& message
        );

        [[nodiscard]] std::vector<NotificationDeliveryExecutionResult> deliver_batch_with_retry(
            const std::vector<NotificationDeliveryMessage>& messages
        );

    private:
        NotificationDeliveryDispatcher* dispatcher_;
        NotificationDeliveryRetryPolicy policy_;

        [[nodiscard]] bool should_retry(
            const NotificationDeliveryResult& result,
            std::uint32_t attempt_number
        ) const noexcept;

        [[nodiscard]] static NotificationDeliveryAttempt make_attempt(
            std::uint32_t attempt_number,
            std::chrono::system_clock::time_point started_at,
            const NotificationDeliveryResult& result
        );
    };
}