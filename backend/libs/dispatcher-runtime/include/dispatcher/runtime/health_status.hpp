#pragma once

namespace dispatcher::runtime
{
    enum class HealthStatus
    {
        Unknown,
        Healthy,
        Degraded,
        Unhealthy
    };

    [[nodiscard]] const char* to_string(
        HealthStatus status
    ) noexcept;

    [[nodiscard]] bool is_known(
        HealthStatus status
    ) noexcept;

    [[nodiscard]] bool is_healthy(
        HealthStatus status
    ) noexcept;

    [[nodiscard]] bool is_degraded(
        HealthStatus status
    ) noexcept;

    [[nodiscard]] bool is_unhealthy(
        HealthStatus status
    ) noexcept;

    [[nodiscard]] bool requires_attention(
        HealthStatus status
    ) noexcept;
}