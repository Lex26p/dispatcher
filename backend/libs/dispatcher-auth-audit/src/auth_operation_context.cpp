#include <dispatcher/auth/audit/auth_operation_context.hpp>

#include <dispatcher/auth/audit/auth_audit_error.hpp>
#include <dispatcher/auth/audit/auth_audit_validator.hpp>

#include <utility>

namespace dispatcher::auth::audit
{
    AuthenticatedOperationContext AuthenticatedOperationContextBuilder::system_context(
        std::string operation_id,
        std::string source,
        std::string correlation_id
    )
    {
        AuthenticatedOperationContext context;

        context.operation_id =
            std::move(
                operation_id
            );

        context.correlation_id =
            std::move(
                correlation_id
            );

        context.source =
            std::move(
                source
            );

        context.actor.actor_id =
            "system";

        context.actor.display_name =
            "System";

        context.actor.actor_type =
            AuthAuditActorType::system;

        validate_context(
            context
        );

        return context;
    }

    AuthenticatedOperationContext AuthenticatedOperationContextBuilder::operator_context(
        std::string operation_id,
        std::string source,
        std::string actor_id,
        std::string display_name,
        std::string correlation_id
    )
    {
        AuthenticatedOperationContext context;

        context.operation_id =
            std::move(
                operation_id
            );

        context.correlation_id =
            std::move(
                correlation_id
            );

        context.source =
            std::move(
                source
            );

        context.actor.actor_id =
            std::move(
                actor_id
            );

        context.actor.display_name =
            std::move(
                display_name
            );

        context.actor.actor_type =
            AuthAuditActorType::operator_user;

        validate_context(
            context
        );

        return context;
    }

    void AuthenticatedOperationContextBuilder::validate_context(
        const AuthenticatedOperationContext& context
    )
    {
        if (context.operation_id.empty())
        {
            throw AuthAuditError(
                "Authenticated operation context operation_id must not be empty."
            );
        }

        if (context.source.empty())
        {
            throw AuthAuditError(
                "Authenticated operation context source must not be empty."
            );
        }

        if (context.actor.actor_id.empty())
        {
            throw AuthAuditError(
                "Authenticated operation context actor_id must not be empty."
            );
        }

        if (AuthAuditValidator::actor_type_to_string(
            context.actor.actor_type
        ) == "unknown")
        {
            throw AuthAuditError(
                "Authenticated operation context actor_type is invalid."
            );
        }
    }

    AuthorizationSubject AuthenticatedOperationContextBuilder::to_authorization_subject(
        const AuthenticatedOperationContext& context,
        std::vector<std::string> roles,
        std::vector<AuthorizationPermission> permissions
    )
    {
        validate_context(
            context
        );

        AuthorizationSubject subject;

        subject.subject_id =
            context.actor.actor_id;

        subject.display_name =
            context.actor.display_name;

        subject.subject_type =
            context.actor.actor_type;

        subject.roles =
            std::move(
                roles
            );

        subject.permissions =
            std::move(
                permissions
            );

        return subject;
    }
}