#pragma once

#include <dispatcher/runtime/health_snapshot.hpp>
#include <dispatcher/runtime/health_status.hpp>
#include <dispatcher/runtime/readiness_status.hpp>
#include <dispatcher/runtime/runtime_consistency_snapshot.hpp>
#include <dispatcher/runtime/runtime_diagnostics_snapshot.hpp>
#include <dispatcher/runtime/runtime_hardening_status.hpp>

#include <chrono>
#include <cstddef>

namespace dispatcher::runtime
{
    class RuntimeHardeningSnapshot
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

        RuntimeHardeningSnapshot(
            HealthSnapshot health,
            RuntimeDiagnosticsSnapshot diagnostics,
            RuntimeConsistencySnapshot consistency,
            TimePoint created_at = Clock::now()
        );

        [[nodiscard]] static RuntimeHardeningSnapshot empty();

        [[nodiscard]] const HealthSnapshot& health() const noexcept;

        [[nodiscard]] const RuntimeDiagnosticsSnapshot& diagnostics()
            const noexcept;

        [[nodiscard]] const RuntimeConsistencySnapshot& consistency()
            const noexcept;

        [[nodiscard]] TimePoint created_at() const noexcept;

        [[nodiscard]] bool has_health_checks() const noexcept;

        [[nodiscard]] bool has_diagnostics() const noexcept;

        [[nodiscard]] bool has_consistency_issues() const noexcept;

        [[nodiscard]] HealthStatus health_status() const noexcept;

        [[nodiscard]] ReadinessStatus readiness_status() const noexcept;

        [[nodiscard]] RuntimeConsistencyStatus consistency_status()
            const noexcept;

        [[nodiscard]] RuntimeHardeningStatus status() const noexcept;

        [[nodiscard]] bool passing() const noexcept;

        [[nodiscard]] bool warning() const noexcept;

        [[nodiscard]] bool failing() const noexcept;

        [[nodiscard]] bool unknown() const noexcept;

        [[nodiscard]] bool production_ready() const noexcept;

        [[nodiscard]] bool release_blocked() const noexcept;

        [[nodiscard]] bool requires_attention() const noexcept;

        [[nodiscard]] std::size_t health_check_count() const noexcept;

        [[nodiscard]] std::size_t diagnostic_record_count() const noexcept;

        [[nodiscard]] std::size_t consistency_issue_count() const noexcept;

        [[nodiscard]] std::size_t attention_item_count() const noexcept;

        [[nodiscard]] std::size_t release_blocker_count() const noexcept;

    private:
        [[nodiscard]] std::size_t health_attention_count() const noexcept;

        [[nodiscard]] std::size_t health_release_blocker_count()
            const noexcept;

        HealthSnapshot health_;
        RuntimeDiagnosticsSnapshot diagnostics_;
        RuntimeConsistencySnapshot consistency_;
        TimePoint created_at_{ Clock::now() };
    };
}