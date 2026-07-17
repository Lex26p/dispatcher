#pragma once

namespace dispatcher::runtime
{
    enum class RuntimeHardeningStatus
    {
        Unknown,
        Passing,
        Warning,
        Failing
    };

    [[nodiscard]] const char* to_string(
        RuntimeHardeningStatus status
    ) noexcept;

    [[nodiscard]] bool is_known(
        RuntimeHardeningStatus status
    ) noexcept;

    [[nodiscard]] bool is_passing(
        RuntimeHardeningStatus status
    ) noexcept;

    [[nodiscard]] bool is_warning(
        RuntimeHardeningStatus status
    ) noexcept;

    [[nodiscard]] bool is_failing(
        RuntimeHardeningStatus status
    ) noexcept;

    [[nodiscard]] bool allows_release(
        RuntimeHardeningStatus status
    ) noexcept;

    [[nodiscard]] bool requires_action(
        RuntimeHardeningStatus status
    ) noexcept;
}