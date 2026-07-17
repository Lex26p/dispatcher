#include <dispatcher/runtime/runtime_hardening_snapshot.hpp>

#include <utility>

namespace dispatcher::runtime
{
    RuntimeHardeningSnapshot::RuntimeHardeningSnapshot(
        HealthSnapshot health,
        RuntimeDiagnosticsSnapshot diagnostics,
        RuntimeConsistencySnapshot consistency,
        TimePoint created_at
    )
        : health_(std::move(health))
        , diagnostics_(std::move(diagnostics))
        , consistency_(std::move(consistency))
        , created_at_(created_at)
    {
    }

    RuntimeHardeningSnapshot RuntimeHardeningSnapshot::empty()
    {
        return RuntimeHardeningSnapshot(
            HealthSnapshot{},
            RuntimeDiagnosticsSnapshot{},
            RuntimeConsistencySnapshot{}
        );
    }

    const HealthSnapshot& RuntimeHardeningSnapshot::health() const noexcept
    {
        return health_;
    }

    const RuntimeDiagnosticsSnapshot& RuntimeHardeningSnapshot::diagnostics()
        const noexcept
    {
        return diagnostics_;
    }

    const RuntimeConsistencySnapshot& RuntimeHardeningSnapshot::consistency()
        const noexcept
    {
        return consistency_;
    }

    RuntimeHardeningSnapshot::TimePoint
        RuntimeHardeningSnapshot::created_at() const noexcept
    {
        return created_at_;
    }

    bool RuntimeHardeningSnapshot::has_health_checks() const noexcept
    {
        return health_.has_checks();
    }

    bool RuntimeHardeningSnapshot::has_diagnostics() const noexcept
    {
        return diagnostics_.has_records();
    }

    bool RuntimeHardeningSnapshot::has_consistency_issues() const noexcept
    {
        return consistency_.has_issues();
    }

    HealthStatus RuntimeHardeningSnapshot::health_status() const noexcept
    {
        return health_.overall_status();
    }

    ReadinessStatus RuntimeHardeningSnapshot::readiness_status() const noexcept
    {
        return health_.readiness_status();
    }

    RuntimeConsistencyStatus RuntimeHardeningSnapshot::consistency_status()
        const noexcept
    {
        return consistency_.status();
    }

    RuntimeHardeningStatus RuntimeHardeningSnapshot::status() const noexcept
    {
        if (!has_health_checks()
            || health_status() == HealthStatus::Unknown
            || readiness_status() == ReadinessStatus::Unknown
            || consistency_status() == RuntimeConsistencyStatus::Unknown)
        {
            return RuntimeHardeningStatus::Unknown;
        }

        if (release_blocked())
        {
            return RuntimeHardeningStatus::Failing;
        }

        if (requires_attention())
        {
            return RuntimeHardeningStatus::Warning;
        }

        return RuntimeHardeningStatus::Passing;
    }

    bool RuntimeHardeningSnapshot::passing() const noexcept
    {
        return status() == RuntimeHardeningStatus::Passing;
    }

    bool RuntimeHardeningSnapshot::warning() const noexcept
    {
        return status() == RuntimeHardeningStatus::Warning;
    }

    bool RuntimeHardeningSnapshot::failing() const noexcept
    {
        return status() == RuntimeHardeningStatus::Failing;
    }

    bool RuntimeHardeningSnapshot::unknown() const noexcept
    {
        return status() == RuntimeHardeningStatus::Unknown;
    }

    bool RuntimeHardeningSnapshot::production_ready() const noexcept
    {
        return passing();
    }

    bool RuntimeHardeningSnapshot::release_blocked() const noexcept
    {
        return health_release_blocker_count() > 0
            || diagnostics_.has_invalid_records()
            || diagnostics_.has_critical_records()
            || consistency_.blocks_release();
    }

    bool RuntimeHardeningSnapshot::requires_attention() const noexcept
    {
        return health_attention_count() > 0
            || diagnostics_.requires_attention()
            || consistency_.requires_attention();
    }

    std::size_t RuntimeHardeningSnapshot::health_check_count() const noexcept
    {
        return health_.check_count();
    }

    std::size_t RuntimeHardeningSnapshot::diagnostic_record_count()
        const noexcept
    {
        return diagnostics_.record_count();
    }

    std::size_t RuntimeHardeningSnapshot::consistency_issue_count()
        const noexcept
    {
        return consistency_.issue_count();
    }

    std::size_t RuntimeHardeningSnapshot::attention_item_count() const noexcept
    {
        return health_attention_count()
            + diagnostics_.attention_count()
            + consistency_.attention_count();
    }

    std::size_t RuntimeHardeningSnapshot::release_blocker_count()
        const noexcept
    {
        return health_release_blocker_count()
            + diagnostics_.invalid_count()
            + diagnostics_.critical_count()
            + consistency_.blocking_issue_count();
    }

    std::size_t RuntimeHardeningSnapshot::health_attention_count()
        const noexcept
    {
        std::size_t count = 0;

        for (const auto& check : health_.checks())
        {
            if (check.requires_attention())
            {
                ++count;
            }
        }

        return count;
    }

    std::size_t RuntimeHardeningSnapshot::health_release_blocker_count()
        const noexcept
    {
        std::size_t count = 0;

        for (const auto& check : health_.checks())
        {
            if (check.blocks_readiness() || !check.valid())
            {
                ++count;
            }
        }

        return count;
    }
}