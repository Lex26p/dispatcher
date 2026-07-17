#pragma once

namespace dispatcher::domain
{
    enum class OperatorAuthorizationStatus
    {
        Allowed,
        DeniedInactiveOperator,
        DeniedInsufficientRole
    };

    [[nodiscard]] const char* to_string(
        OperatorAuthorizationStatus status
    ) noexcept;

    [[nodiscard]] bool is_authorized(
        OperatorAuthorizationStatus status
    ) noexcept;

    [[nodiscard]] bool is_denied(
        OperatorAuthorizationStatus status
    ) noexcept;
}