#pragma once

#include <dispatcher/runtime/health_status.hpp>

#include <chrono>
#include <string>

namespace dispatcher::runtime
{
    class HealthCheckResult
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;
        using Duration = std::chrono::milliseconds;

        HealthCheckResult(
            std::string name,
            std::string component,
            HealthStatus status,
            bool mandatory = true,
            std::string message = {},
            Duration duration = Duration{ 0 },
            TimePoint checked_at = Clock::now()
        );

        [[nodiscard]] static HealthCheckResult healthy(
            std::string name,
            std::string component,
            std::string message = {},
            bool mandatory = true,
            Duration duration = Duration{ 0 }
        );

        [[nodiscard]] static HealthCheckResult degraded(
            std::string name,
            std::string component,
            std::string message = {},
            bool mandatory = true,
            Duration duration = Duration{ 0 }
        );

        [[nodiscard]] static HealthCheckResult unhealthy(
            std::string name,
            std::string component,
            std::string message = {},
            bool mandatory = true,
            Duration duration = Duration{ 0 }
        );

        [[nodiscard]] const std::string& name() const noexcept;

        [[nodiscard]] const std::string& component() const noexcept;

        [[nodiscard]] HealthStatus status() const noexcept;

        [[nodiscard]] bool mandatory() const noexcept;

        [[nodiscard]] bool optional() const noexcept;

        [[nodiscard]] const std::string& message() const noexcept;

        [[nodiscard]] Duration duration() const noexcept;

        [[nodiscard]] TimePoint checked_at() const noexcept;

        [[nodiscard]] bool has_name() const noexcept;

        [[nodiscard]] bool has_component() const noexcept;

        [[nodiscard]] bool has_message() const noexcept;

        [[nodiscard]] bool valid() const noexcept;

        [[nodiscard]] bool healthy() const noexcept;

        [[nodiscard]] bool degraded() const noexcept;

        [[nodiscard]] bool unhealthy() const noexcept;

        [[nodiscard]] bool requires_attention() const noexcept;

        [[nodiscard]] bool blocks_readiness() const noexcept;

    private:
        std::string name_;
        std::string component_;
        HealthStatus status_{ HealthStatus::Unknown };
        bool mandatory_{ true };
        std::string message_;
        Duration duration_{ 0 };
        TimePoint checked_at_{ Clock::now() };
    };
}