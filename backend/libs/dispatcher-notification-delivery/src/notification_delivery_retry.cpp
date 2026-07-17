#include <dispatcher/notification/delivery/notification_delivery_retry.hpp>

#include <dispatcher/notification/delivery/notification_delivery_error.hpp>
#include <dispatcher/notification/delivery/notification_delivery_validator.hpp>

namespace dispatcher::notification::delivery
{
    namespace
    {
        constexpr std::uint32_t max_supported_attempts = 10;
    }

    NotificationDeliveryRetryExecutor::NotificationDeliveryRetryExecutor(
        NotificationDeliveryDispatcher& dispatcher,
        NotificationDeliveryRetryPolicy policy
    )
        : dispatcher_(
            &dispatcher
        )
        , policy_(
            policy
        )
    {
        validate_policy(
            policy_
        );
    }

    const NotificationDeliveryRetryPolicy& NotificationDeliveryRetryExecutor::policy() const noexcept
    {
        return policy_;
    }

    void NotificationDeliveryRetryExecutor::validate_policy(
        const NotificationDeliveryRetryPolicy& policy
    )
    {
        if (policy.max_attempts == 0)
        {
            throw NotificationDeliveryError(
                "Notification delivery retry policy max_attempts must be greater than zero."
            );
        }

        if (policy.max_attempts > max_supported_attempts)
        {
            throw NotificationDeliveryError(
                "Notification delivery retry policy max_attempts exceeds supported maximum."
            );
        }
    }

    NotificationDeliveryExecutionResult NotificationDeliveryRetryExecutor::deliver_with_retry(
        const NotificationDeliveryMessage& message
    )
    {
        NotificationDeliveryValidator::validate_message(
            message
        );

        NotificationDeliveryExecutionResult execution_result;

        for (std::uint32_t attempt_number = 1;
            attempt_number <= policy_.max_attempts;
            ++attempt_number)
        {
            const auto started_at =
                std::chrono::system_clock::now();

            auto delivery_result =
                dispatcher_->deliver(
                    message
                );

            NotificationDeliveryValidator::validate_result(
                delivery_result
            );

            execution_result.attempts.push_back(
                make_attempt(
                    attempt_number,
                    started_at,
                    delivery_result
                )
            );

            execution_result.final_result =
                delivery_result;

            if (!should_retry(
                delivery_result,
                attempt_number
            ))
            {
                break;
            }
        }

        return execution_result;
    }

    std::vector<NotificationDeliveryExecutionResult> NotificationDeliveryRetryExecutor::deliver_batch_with_retry(
        const std::vector<NotificationDeliveryMessage>& messages
    )
    {
        std::vector<NotificationDeliveryExecutionResult> results;

        results.reserve(
            messages.size()
        );

        for (const auto& message : messages)
        {
            results.push_back(
                deliver_with_retry(
                    message
                )
            );
        }

        return results;
    }

    bool NotificationDeliveryRetryExecutor::should_retry(
        const NotificationDeliveryResult& result,
        std::uint32_t attempt_number
    ) const noexcept
    {
        if (attempt_number >= policy_.max_attempts)
        {
            return false;
        }

        if (result.status == NotificationDeliveryStatus::failed)
        {
            return policy_.retry_failed;
        }

        if (result.status == NotificationDeliveryStatus::skipped)
        {
            return policy_.retry_skipped;
        }

        return false;
    }

    NotificationDeliveryAttempt NotificationDeliveryRetryExecutor::make_attempt(
        std::uint32_t attempt_number,
        std::chrono::system_clock::time_point started_at,
        const NotificationDeliveryResult& result
    )
    {
        NotificationDeliveryAttempt attempt;

        attempt.attempt_number =
            attempt_number;

        attempt.status =
            result.status;

        attempt.provider_message_id =
            result.provider_message_id;

        attempt.error_message =
            result.error_message;

        attempt.diagnostic_message =
            result.diagnostic_message;

        attempt.started_at =
            started_at;

        attempt.completed_at =
            result.completed_at;

        return attempt;
    }
}