#pragma once

namespace dispatcher::domain
{
    enum class OperatorStatus
    {
        Active,
        Disabled,
        Locked,
        Expired
    };

    [[nodiscard]] const char* to_string(
        OperatorStatus status
    ) noexcept;

    [[nodiscard]] bool can_sign_in(
        OperatorStatus status
    ) noexcept;

    [[nodiscard]] bool is_active(
        OperatorStatus status
    ) noexcept;

    [[nodiscard]] bool is_disabled(
        OperatorStatus status
    ) noexcept;

    [[nodiscard]] bool is_locked(
        OperatorStatus status
    ) noexcept;

    [[nodiscard]] bool is_expired(
        OperatorStatus status
    ) noexcept;
}