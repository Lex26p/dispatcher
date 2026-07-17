#include <dispatcher/domain/operator_authorization_status.hpp>

namespace dispatcher::domain
{
    const char* to_string(
        OperatorAuthorizationStatus status
    ) noexcept
    {
        switch (status)
        {
        case OperatorAuthorizationStatus::Allowed:
            return "allowed";

        case OperatorAuthorizationStatus::DeniedInactiveOperator:
            return "denied_inactive_operator";

        case OperatorAuthorizationStatus::DeniedInsufficientRole:
            return "denied_insufficient_role";
        }

        return "denied_insufficient_role";
    }

    bool is_authorized(OperatorAuthorizationStatus status) noexcept
    {
        return status == OperatorAuthorizationStatus::Allowed;
    }

    bool is_denied(OperatorAuthorizationStatus status) noexcept
    {
        return !is_authorized(status);
    }
}