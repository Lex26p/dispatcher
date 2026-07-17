#pragma once

namespace dispatcher::runtime
{
    enum class RuntimeConsistencyStatus
    {
        Unknown,
        Consistent,
        Inconsistent
    };

    [[nodiscard]] const char* to_string(
        RuntimeConsistencyStatus status
    ) noexcept;

    [[nodiscard]] bool is_known(
        RuntimeConsistencyStatus status
    ) noexcept;

    [[nodiscard]] bool is_consistent(
        RuntimeConsistencyStatus status
    ) noexcept;

    [[nodiscard]] bool is_inconsistent(
        RuntimeConsistencyStatus status
    ) noexcept;

    [[nodiscard]] bool requires_action(
        RuntimeConsistencyStatus status
    ) noexcept;
}