#pragma once

namespace dispatcher::domain
{
    enum class OperatorSessionStatus
    {
        Active,
        SignedOut,
        Expired,
        NotFound,
        InvalidSession
    };

    [[nodiscard]] const char* to_string(
        OperatorSessionStatus status
    ) noexcept;

    [[nodiscard]] bool is_active(
        OperatorSessionStatus status
    ) noexcept;

    [[nodiscard]] bool is_terminal(
        OperatorSessionStatus status
    ) noexcept;

    [[nodiscard]] bool is_failure(
        OperatorSessionStatus status
    ) noexcept;
}