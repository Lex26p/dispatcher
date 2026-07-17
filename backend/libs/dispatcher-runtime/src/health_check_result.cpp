#include <dispatcher/runtime/health_check_result.hpp>

#include <utility>

namespace dispatcher::runtime
{
    HealthCheckResult::HealthCheckResult(
        std::string name,
        std::string component,
        HealthStatus status,
        bool mandatory,
        std::string message,
        Duration duration,
        TimePoint checked_at
    )
        : name_(std::move(name))
        , component_(std::move(component))
        , status_(status)
        , mandatory_(mandatory)
        , message_(std::move(message))
        , duration_(duration)
        , checked_at_(checked_at)
    {
    }

    HealthCheckResult HealthCheckResult::healthy(
        std::string name,
        std::string component,
        std::string message,
        bool mandatory,
        Duration duration
    )
    {
        return HealthCheckResult(
            std::move(name),
            std::move(component),
            HealthStatus::Healthy,
            mandatory,
            std::move(message),
            duration
        );
    }

    HealthCheckResult HealthCheckResult::degraded(
        std::string name,
        std::string component,
        std::string message,
        bool mandatory,
        Duration duration
    )
    {
        return HealthCheckResult(
            std::move(name),
            std::move(component),
            HealthStatus::Degraded,
            mandatory,
            std::move(message),
            duration
        );
    }

    HealthCheckResult HealthCheckResult::unhealthy(
        std::string name,
        std::string component,
        std::string message,
        bool mandatory,
        Duration duration
    )
    {
        return HealthCheckResult(
            std::move(name),
            std::move(component),
            HealthStatus::Unhealthy,
            mandatory,
            std::move(message),
            duration
        );
    }

    const std::string& HealthCheckResult::name() const noexcept
    {
        return name_;
    }

    const std::string& HealthCheckResult::component() const noexcept
    {
        return component_;
    }

    HealthStatus HealthCheckResult::status() const noexcept
    {
        return status_;
    }

    bool HealthCheckResult::mandatory() const noexcept
    {
        return mandatory_;
    }

    bool HealthCheckResult::optional() const noexcept
    {
        return !mandatory_;
    }

    const std::string& HealthCheckResult::message() const noexcept
    {
        return message_;
    }

    HealthCheckResult::Duration HealthCheckResult::duration() const noexcept
    {
        return duration_;
    }

    HealthCheckResult::TimePoint HealthCheckResult::checked_at()
        const noexcept
    {
        return checked_at_;
    }

    bool HealthCheckResult::has_name() const noexcept
    {
        return !name_.empty();
    }

    bool HealthCheckResult::has_component() const noexcept
    {
        return !component_.empty();
    }

    bool HealthCheckResult::has_message() const noexcept
    {
        return !message_.empty();
    }

    bool HealthCheckResult::valid() const noexcept
    {
        return has_name()
            && has_component()
            && is_known(status_);
    }

    bool HealthCheckResult::healthy() const noexcept
    {
        return is_healthy(status_);
    }

    bool HealthCheckResult::degraded() const noexcept
    {
        return is_degraded(status_);
    }

    bool HealthCheckResult::unhealthy() const noexcept
    {
        return is_unhealthy(status_);
    }

    bool HealthCheckResult::requires_attention() const noexcept
    {
        return dispatcher::runtime::requires_attention(status_);
    }

    bool HealthCheckResult::blocks_readiness() const noexcept
    {
        return mandatory_
            && (status_ == HealthStatus::Unhealthy
                || status_ == HealthStatus::Unknown);
    }
}