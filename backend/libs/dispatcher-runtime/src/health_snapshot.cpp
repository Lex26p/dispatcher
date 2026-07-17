#include <dispatcher/runtime/health_snapshot.hpp>

#include <algorithm>
#include <utility>

namespace dispatcher::runtime
{
    HealthSnapshot::HealthSnapshot(
        TimePoint created_at
    )
        : created_at_(created_at)
    {
    }

    void HealthSnapshot::add_check(
        HealthCheckResult check
    )
    {
        checks_.push_back(
            std::move(check)
        );
    }

    const std::vector<HealthCheckResult>& HealthSnapshot::checks()
        const noexcept
    {
        return checks_;
    }

    HealthSnapshot::TimePoint HealthSnapshot::created_at() const noexcept
    {
        return created_at_;
    }

    bool HealthSnapshot::empty() const noexcept
    {
        return checks_.empty();
    }

    bool HealthSnapshot::has_checks() const noexcept
    {
        return !empty();
    }

    std::size_t HealthSnapshot::check_count() const noexcept
    {
        return checks_.size();
    }

    std::size_t HealthSnapshot::healthy_count() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                checks_.begin(),
                checks_.end(),
                [](const HealthCheckResult& check)
                {
                    return check.healthy();
                }
            )
            );
    }

    std::size_t HealthSnapshot::degraded_count() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                checks_.begin(),
                checks_.end(),
                [](const HealthCheckResult& check)
                {
                    return check.degraded();
                }
            )
            );
    }

    std::size_t HealthSnapshot::unhealthy_count() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                checks_.begin(),
                checks_.end(),
                [](const HealthCheckResult& check)
                {
                    return check.unhealthy();
                }
            )
            );
    }

    std::size_t HealthSnapshot::invalid_count() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                checks_.begin(),
                checks_.end(),
                [](const HealthCheckResult& check)
                {
                    return !check.valid();
                }
            )
            );
    }

    std::size_t HealthSnapshot::mandatory_count() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                checks_.begin(),
                checks_.end(),
                [](const HealthCheckResult& check)
                {
                    return check.mandatory();
                }
            )
            );
    }

    std::size_t HealthSnapshot::optional_count() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                checks_.begin(),
                checks_.end(),
                [](const HealthCheckResult& check)
                {
                    return check.optional();
                }
            )
            );
    }

    bool HealthSnapshot::has_invalid_checks() const noexcept
    {
        return invalid_count() > 0;
    }

    bool HealthSnapshot::has_degraded_checks() const noexcept
    {
        return degraded_count() > 0;
    }

    bool HealthSnapshot::has_unhealthy_checks() const noexcept
    {
        return unhealthy_count() > 0;
    }

    bool HealthSnapshot::has_readiness_blockers() const noexcept
    {
        return std::any_of(
            checks_.begin(),
            checks_.end(),
            [](const HealthCheckResult& check)
            {
                return check.blocks_readiness();
            }
        );
    }

    HealthStatus HealthSnapshot::overall_status() const noexcept
    {
        if (checks_.empty())
        {
            return HealthStatus::Unknown;
        }

        if (has_invalid_checks())
        {
            return HealthStatus::Unknown;
        }

        if (has_unhealthy_checks())
        {
            return HealthStatus::Unhealthy;
        }

        if (has_degraded_checks())
        {
            return HealthStatus::Degraded;
        }

        return HealthStatus::Healthy;
    }

    ReadinessStatus HealthSnapshot::readiness_status() const noexcept
    {
        if (checks_.empty())
        {
            return ReadinessStatus::Unknown;
        }

        if (has_invalid_checks() || has_readiness_blockers())
        {
            return ReadinessStatus::NotReady;
        }

        return ReadinessStatus::Ready;
    }

    bool HealthSnapshot::healthy() const noexcept
    {
        return overall_status() == HealthStatus::Healthy;
    }

    bool HealthSnapshot::ready() const noexcept
    {
        return readiness_status() == ReadinessStatus::Ready;
    }

    std::vector<HealthCheckResult> HealthSnapshot::checks_for_component(
        const std::string& component
    ) const
    {
        std::vector<HealthCheckResult> result;

        for (const auto& check : checks_)
        {
            if (check.component() == component)
            {
                result.push_back(check);
            }
        }

        return result;
    }

    void HealthSnapshot::clear() noexcept
    {
        checks_.clear();
    }
}