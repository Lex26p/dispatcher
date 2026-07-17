#include <dispatcher/domain/operator_authorization_result.hpp>

#include <utility>

namespace dispatcher::domain
{
    OperatorAuthorizationResult OperatorAuthorizationResult::allowed(
        OperatorId operator_id,
        std::string username,
        OperatorRole role,
        OperatorPermission permission
    )
    {
        return OperatorAuthorizationResult(
            std::move(operator_id),
            std::move(username),
            role,
            OperatorStatus::Active,
            permission,
            OperatorAuthorizationStatus::Allowed,
            {}
        );
    }

    OperatorAuthorizationResult OperatorAuthorizationResult::denied(
        OperatorId operator_id,
        std::string username,
        OperatorRole role,
        OperatorStatus operator_status,
        OperatorPermission permission,
        OperatorAuthorizationStatus authorization_status,
        std::string reason
    )
    {
        if (authorization_status == OperatorAuthorizationStatus::Allowed)
        {
            authorization_status =
                OperatorAuthorizationStatus::DeniedInsufficientRole;
        }

        return OperatorAuthorizationResult(
            std::move(operator_id),
            std::move(username),
            role,
            operator_status,
            permission,
            authorization_status,
            std::move(reason)
        );
    }

    bool OperatorAuthorizationResult::authorized() const noexcept
    {
        return is_authorized(authorization_status_);
    }

    bool OperatorAuthorizationResult::denied() const noexcept
    {
        return !authorized();
    }

    OperatorAuthorizationStatus OperatorAuthorizationResult::status()
        const noexcept
    {
        return authorization_status_;
    }

    const OperatorId& OperatorAuthorizationResult::operator_id()
        const noexcept
    {
        return operator_id_;
    }

    const std::string& OperatorAuthorizationResult::username() const noexcept
    {
        return username_;
    }

    OperatorRole OperatorAuthorizationResult::role() const noexcept
    {
        return role_;
    }

    OperatorStatus OperatorAuthorizationResult::operator_status()
        const noexcept
    {
        return operator_status_;
    }

    OperatorPermission OperatorAuthorizationResult::permission()
        const noexcept
    {
        return permission_;
    }

    const std::string& OperatorAuthorizationResult::reason() const noexcept
    {
        return reason_;
    }

    bool OperatorAuthorizationResult::has_reason() const noexcept
    {
        return !reason_.empty();
    }

    OperatorAuthorizationResult::OperatorAuthorizationResult(
        OperatorId operator_id,
        std::string username,
        OperatorRole role,
        OperatorStatus operator_status,
        OperatorPermission permission,
        OperatorAuthorizationStatus authorization_status,
        std::string reason
    )
        : operator_id_(std::move(operator_id))
        , username_(std::move(username))
        , role_(role)
        , operator_status_(operator_status)
        , permission_(permission)
        , authorization_status_(authorization_status)
        , reason_(std::move(reason))
    {
    }
}