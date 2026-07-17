#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/operator_authorization_status.hpp>
#include <dispatcher/domain/operator_permission.hpp>
#include <dispatcher/domain/operator_role.hpp>
#include <dispatcher/domain/operator_status.hpp>

#include <string>

namespace dispatcher::domain
{
    class OperatorAuthorizationResult
    {
    public:
        [[nodiscard]] static OperatorAuthorizationResult allowed(
            OperatorId operator_id,
            std::string username,
            OperatorRole role,
            OperatorPermission permission
        );

        [[nodiscard]] static OperatorAuthorizationResult denied(
            OperatorId operator_id,
            std::string username,
            OperatorRole role,
            OperatorStatus operator_status,
            OperatorPermission permission,
            OperatorAuthorizationStatus authorization_status,
            std::string reason
        );

        [[nodiscard]] bool authorized() const noexcept;

        [[nodiscard]] bool denied() const noexcept;

        [[nodiscard]] OperatorAuthorizationStatus status() const noexcept;

        [[nodiscard]] const OperatorId& operator_id() const noexcept;

        [[nodiscard]] const std::string& username() const noexcept;

        [[nodiscard]] OperatorRole role() const noexcept;

        [[nodiscard]] OperatorStatus operator_status() const noexcept;

        [[nodiscard]] OperatorPermission permission() const noexcept;

        [[nodiscard]] const std::string& reason() const noexcept;

        [[nodiscard]] bool has_reason() const noexcept;

    private:
        OperatorAuthorizationResult(
            OperatorId operator_id,
            std::string username,
            OperatorRole role,
            OperatorStatus operator_status,
            OperatorPermission permission,
            OperatorAuthorizationStatus authorization_status,
            std::string reason
        );

        OperatorId operator_id_;
        std::string username_;
        OperatorRole role_{ OperatorRole::Viewer };
        OperatorStatus operator_status_{ OperatorStatus::Disabled };
        OperatorPermission permission_{ OperatorPermission::ViewRuntime };
        OperatorAuthorizationStatus authorization_status_{
            OperatorAuthorizationStatus::DeniedInsufficientRole
        };
        std::string reason_;
    };
}