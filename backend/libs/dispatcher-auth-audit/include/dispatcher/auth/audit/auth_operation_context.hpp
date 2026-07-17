#pragma once

#include <dispatcher/auth/audit/auth_audit_types.hpp>
#include <dispatcher/auth/audit/auth_authorization_policy.hpp>

#include <map>
#include <string>
#include <vector>

namespace dispatcher::auth::audit
{
    struct AuthenticatedOperationContext
    {
        std::string operation_id{};
        std::string correlation_id{};
        std::string source{};

        AuthAuditActor actor{};

        std::string client_address{};
        std::string user_agent{};

        std::map<std::string, std::string> attributes{};
    };

    class AuthenticatedOperationContextBuilder final
    {
    public:
        [[nodiscard]] static AuthenticatedOperationContext system_context(
            std::string operation_id,
            std::string source,
            std::string correlation_id = {}
        );

        [[nodiscard]] static AuthenticatedOperationContext operator_context(
            std::string operation_id,
            std::string source,
            std::string actor_id,
            std::string display_name,
            std::string correlation_id = {}
        );

        static void validate_context(
            const AuthenticatedOperationContext& context
        );

        [[nodiscard]] static AuthorizationSubject to_authorization_subject(
            const AuthenticatedOperationContext& context,
            std::vector<std::string> roles = {},
            std::vector<AuthorizationPermission> permissions = {}
        );
    };
}