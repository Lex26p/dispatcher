#pragma once

#include <dispatcher/domain/operator_authorization_result.hpp>
#include <dispatcher/domain/operator_identity.hpp>
#include <dispatcher/domain/operator_permission.hpp>

namespace dispatcher::domain
{
    class OperatorAuthorizer
    {
    public:
        [[nodiscard]] OperatorAuthorizationResult authorize(
            const OperatorIdentity& identity,
            OperatorPermission permission
        ) const;

        [[nodiscard]] bool is_authorized(
            const OperatorIdentity& identity,
            OperatorPermission permission
        ) const noexcept;
    };
}