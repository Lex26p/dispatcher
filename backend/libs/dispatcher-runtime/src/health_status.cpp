#include <dispatcher/runtime/health_status.hpp>

namespace dispatcher::runtime
{
    const char* to_string(HealthStatus status) noexcept
    {
        switch (status)
        {
        case HealthStatus::Unknown:
            return "unknown";

        case HealthStatus::Healthy:
            return "healthy";

        case HealthStatus::Degraded:
            return "degraded";

        case HealthStatus::Unhealthy:
            return "unhealthy";
        }

        return "unknown";
    }

    bool is_known(HealthStatus status) noexcept
    {
        return status != HealthStatus::Unknown;
    }

    bool is_healthy(HealthStatus status) noexcept
    {
        return status == HealthStatus::Healthy;
    }

    bool is_degraded(HealthStatus status) noexcept
    {
        return status == HealthStatus::Degraded;
    }

    bool is_unhealthy(HealthStatus status) noexcept
    {
        return status == HealthStatus::Unhealthy;
    }

    bool requires_attention(HealthStatus status) noexcept
    {
        return status == HealthStatus::Degraded
            || status == HealthStatus::Unhealthy
            || status == HealthStatus::Unknown;
    }
}