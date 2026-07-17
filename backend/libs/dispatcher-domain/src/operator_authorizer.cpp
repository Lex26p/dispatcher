#include <dispatcher/domain/operator_authorizer.hpp>

namespace dispatcher::domain
{
    OperatorAuthorizationResult OperatorAuthorizer::authorize(
        const OperatorIdentity& identity,
        OperatorPermission permission
    ) const
    {
        if (!identity.active())
        {
            return OperatorAuthorizationResult::denied(
                OperatorId{ identity.operator_id().value() },
                identity.username(),
                identity.role(),
                identity.status(),
                permission,
                OperatorAuthorizationStatus::DeniedInactiveOperator,
                "operator is not active"
            );
        }

        if (!is_permission_allowed_for_role(
            identity.role(),
            permission
        ))
        {
            return OperatorAuthorizationResult::denied(
                OperatorId{ identity.operator_id().value() },
                identity.username(),
                identity.role(),
                identity.status(),
                permission,
                OperatorAuthorizationStatus::DeniedInsufficientRole,
                "operator role does not grant requested permission"
            );
        }

        return OperatorAuthorizationResult::allowed(
            OperatorId{ identity.operator_id().value() },
            identity.username(),
            identity.role(),
            permission
        );
    }

    bool OperatorAuthorizer::is_authorized(
        const OperatorIdentity& identity,
        OperatorPermission permission
    ) const noexcept
    {
        return identity.active()
            && is_permission_allowed_for_role(
                identity.role(),
                permission
            );
    }
}