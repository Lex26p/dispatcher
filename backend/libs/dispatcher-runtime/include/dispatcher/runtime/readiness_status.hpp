#pragma once

namespace dispatcher::runtime
{
    enum class ReadinessStatus
    {
        Unknown,
        Ready,
        NotReady
    };

    [[nodiscard]] const char* to_string(
        ReadinessStatus status
    ) noexcept;

    [[nodiscard]] bool is_known(
        ReadinessStatus status
    ) noexcept;

    [[nodiscard]] bool is_ready(
        ReadinessStatus status
    ) noexcept;

    [[nodiscard]] bool is_not_ready(
        ReadinessStatus status
    ) noexcept;
}