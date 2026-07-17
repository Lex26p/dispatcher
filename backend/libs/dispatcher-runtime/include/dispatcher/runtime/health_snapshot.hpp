#pragma once

#include <dispatcher/runtime/health_check_result.hpp>
#include <dispatcher/runtime/health_status.hpp>
#include <dispatcher/runtime/readiness_status.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace dispatcher::runtime
{
    class HealthSnapshot
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

        explicit HealthSnapshot(
            TimePoint created_at = Clock::now()
        );

        void add_check(
            HealthCheckResult check
        );

        [[nodiscard]] const std::vector<HealthCheckResult>& checks()
            const noexcept;

        [[nodiscard]] TimePoint created_at() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool has_checks() const noexcept;

        [[nodiscard]] std::size_t check_count() const noexcept;

        [[nodiscard]] std::size_t healthy_count() const noexcept;

        [[nodiscard]] std::size_t degraded_count() const noexcept;

        [[nodiscard]] std::size_t unhealthy_count() const noexcept;

        [[nodiscard]] std::size_t invalid_count() const noexcept;

        [[nodiscard]] std::size_t mandatory_count() const noexcept;

        [[nodiscard]] std::size_t optional_count() const noexcept;

        [[nodiscard]] bool has_invalid_checks() const noexcept;

        [[nodiscard]] bool has_degraded_checks() const noexcept;

        [[nodiscard]] bool has_unhealthy_checks() const noexcept;

        [[nodiscard]] bool has_readiness_blockers() const noexcept;

        [[nodiscard]] HealthStatus overall_status() const noexcept;

        [[nodiscard]] ReadinessStatus readiness_status() const noexcept;

        [[nodiscard]] bool healthy() const noexcept;

        [[nodiscard]] bool ready() const noexcept;

        [[nodiscard]] std::vector<HealthCheckResult> checks_for_component(
            const std::string& component
        ) const;

        void clear() noexcept;

    private:
        TimePoint created_at_{ Clock::now() };
        std::vector<HealthCheckResult> checks_;
    };
}