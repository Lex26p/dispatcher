#include <dispatcher/auth/audit/runtime_auth_audit_adapter.hpp>

#include <dispatcher/auth/audit/auth_audit_error.hpp>
#include <dispatcher/auth/audit/auth_audit_validator.hpp>

#include <utility>

namespace dispatcher::auth::audit
{
    RuntimeAuthAuditAdapter::RuntimeAuthAuditAdapter(
        std::vector<RuntimeAuthAuditOperationRule> operation_rules
    )
        : operation_rules_(
            std::move(
                operation_rules
            )
        )
    {
        if (operation_rules_.empty())
        {
            throw AuthAuditError(
                "Runtime auth-audit adapter requires at least one operation rule."
            );
        }

        for (const auto& rule : operation_rules_)
        {
            validate_operation_rule(
                rule
            );
        }
    }

    const std::vector<RuntimeAuthAuditOperationRule>& RuntimeAuthAuditAdapter::operation_rules() const noexcept
    {
        return operation_rules_;
    }

    RuntimeAuthAuditMappingResult RuntimeAuthAuditAdapter::map_request(
        const RuntimeAuthAuditRequestContext& request_context
    ) const
    {
        validate_request_context(
            request_context
        );

        const auto* rule =
            find_matching_rule(
                request_context
            );

        if (rule == nullptr)
        {
            throw AuthAuditError(
                "No runtime auth-audit operation rule matches request."
            );
        }

        if (rule->require_authenticated
            && (request_context.actor_id.empty()
                || request_context.actor_type == AuthAuditActorType::anonymous))
        {
            throw AuthAuditError(
                "Runtime auth-audit operation requires an authenticated actor."
            );
        }

        RuntimeAuthAuditMappingResult result;

        result.operation_rule =
            *rule;

        result.operation_context =
            build_operation_context(
                request_context,
                *rule
            );

        result.authorization_request =
            build_authorization_request(
                request_context,
                result.operation_context,
                *rule
            );

        return result;
    }

    RuntimeAuthAuditAuthorizeResult RuntimeAuthAuditAdapter::authorize_and_audit(
        const AuthorizationPolicyEvaluator& evaluator,
        AuthAuditLogger& logger,
        const RuntimeAuthAuditRequestContext& request_context,
        AuthorizationAuditOptions options
    ) const
    {
        const auto mapping =
            map_request(
                request_context
            );

        const auto authorization_audit_record =
            AuthorizationAuditRecorder::evaluate_and_record(
                evaluator,
                logger,
                mapping.operation_context,
                mapping.authorization_request,
                options
            );

        RuntimeAuthAuditAuthorizeResult result;

        result.mapping =
            mapping;

        result.authorization_audit_record =
            authorization_audit_record;

        return result;
    }

    void RuntimeAuthAuditAdapter::validate_request_context(
        const RuntimeAuthAuditRequestContext& request_context
    )
    {
        if (request_context.operation_id.empty())
        {
            throw AuthAuditError(
                "Runtime auth-audit operation_id must not be empty."
            );
        }

        if (request_context.source.empty())
        {
            throw AuthAuditError(
                "Runtime auth-audit source must not be empty."
            );
        }

        if (operation_to_string(
            request_context.operation
        ) == "unknown")
        {
            throw AuthAuditError(
                "Runtime auth-audit operation is invalid."
            );
        }

        if (AuthAuditValidator::actor_type_to_string(
            request_context.actor_type
        ) == "unknown")
        {
            throw AuthAuditError(
                "Runtime auth-audit actor_type is invalid."
            );
        }
    }

    void RuntimeAuthAuditAdapter::validate_operation_rule(
        const RuntimeAuthAuditOperationRule& rule
    )
    {
        if (rule.rule_id.empty())
        {
            throw AuthAuditError(
                "Runtime auth-audit operation rule_id must not be empty."
            );
        }

        if (operation_to_string(
            rule.operation
        ) == "unknown")
        {
            throw AuthAuditError(
                "Runtime auth-audit operation rule operation is invalid."
            );
        }

        if (rule.action == AuthAuditAction::unknown)
        {
            throw AuthAuditError(
                "Runtime auth-audit operation rule action must not be unknown."
            );
        }

        if (AuthAuditValidator::action_to_string(
            rule.action
        ) == "unknown")
        {
            throw AuthAuditError(
                "Runtime auth-audit operation rule action is invalid."
            );
        }

        if (AuthorizationPolicyEvaluator::permission_to_string(
            rule.required_permission
        ) == "unknown")
        {
            throw AuthAuditError(
                "Runtime auth-audit operation rule required_permission is invalid."
            );
        }

        if (rule.resource.resource_type.empty())
        {
            throw AuthAuditError(
                "Runtime auth-audit operation rule resource_type must not be empty."
            );
        }

        if (rule.resource.resource_id.empty())
        {
            throw AuthAuditError(
                "Runtime auth-audit operation rule resource_id must not be empty."
            );
        }
    }

    std::string RuntimeAuthAuditAdapter::operation_to_string(
        RuntimeAuthAuditOperation operation
    )
    {
        switch (operation)
        {
        case RuntimeAuthAuditOperation::runtime_read:
            return "runtime_read";

        case RuntimeAuthAuditOperation::runtime_control:
            return "runtime_control";

        case RuntimeAuthAuditOperation::alarm_acknowledge:
            return "alarm_acknowledge";

        case RuntimeAuthAuditOperation::alarm_shelve:
            return "alarm_shelve";

        case RuntimeAuthAuditOperation::alarm_unshelve:
            return "alarm_unshelve";

        case RuntimeAuthAuditOperation::configuration_import:
            return "configuration_import";

        case RuntimeAuthAuditOperation::configuration_export:
            return "configuration_export";

        case RuntimeAuthAuditOperation::notification_send:
            return "notification_send";
        }

        return "unknown";
    }

    const RuntimeAuthAuditOperationRule* RuntimeAuthAuditAdapter::find_matching_rule(
        const RuntimeAuthAuditRequestContext& request_context
    ) const
    {
        for (const auto& rule : operation_rules_)
        {
            if (!rule.enabled)
            {
                continue;
            }

            if (operation_matches(
                rule,
                request_context
            ))
            {
                return &rule;
            }
        }

        return nullptr;
    }

    bool RuntimeAuthAuditAdapter::operation_matches(
        const RuntimeAuthAuditOperationRule& rule,
        const RuntimeAuthAuditRequestContext& request_context
    )
    {
        return rule.operation == request_context.operation;
    }

    AuthenticatedOperationContext RuntimeAuthAuditAdapter::build_operation_context(
        const RuntimeAuthAuditRequestContext& request_context,
        const RuntimeAuthAuditOperationRule&
    )
    {
        AuthenticatedOperationContext context;

        context.operation_id =
            request_context.operation_id;

        context.correlation_id =
            request_context.correlation_id;

        context.source =
            request_context.source;

        context.actor.actor_id =
            request_context.actor_id.empty()
            ? "anonymous"
            : request_context.actor_id;

        context.actor.display_name =
            request_context.actor_display_name;

        context.actor.actor_type =
            request_context.actor_type;

        context.client_address =
            request_context.client_address;

        context.user_agent =
            request_context.user_agent;

        context.attributes =
            request_context.attributes;

        context.attributes.insert_or_assign(
            "runtime.operation",
            operation_to_string(
                request_context.operation
            )
        );

        AuthenticatedOperationContextBuilder::validate_context(
            context
        );

        return context;
    }

    AuthorizationRequest RuntimeAuthAuditAdapter::build_authorization_request(
        const RuntimeAuthAuditRequestContext& request_context,
        const AuthenticatedOperationContext& operation_context,
        const RuntimeAuthAuditOperationRule& rule
    )
    {
        AuthorizationRequest request;

        request.request_id =
            request_context.operation_id
            + ":authorization";

        request.correlation_id =
            request_context.correlation_id;

        request.source =
            request_context.source;

        request.subject =
            AuthenticatedOperationContextBuilder::to_authorization_subject(
                operation_context,
                request_context.roles,
                request_context.permissions
            );

        request.action =
            rule.action;

        request.resource =
            effective_resource(
                request_context,
                rule
            );

        request.required_permission =
            rule.required_permission;

        request.attributes =
            request_context.attributes;

        request.attributes.insert_or_assign(
            "runtime.operation",
            operation_to_string(
                request_context.operation
            )
        );

        request.attributes.insert_or_assign(
            "runtime.operation_rule_id",
            rule.rule_id
        );

        AuthorizationPolicyEvaluator::validate_request(
            request
        );

        return request;
    }

    AuthAuditResource RuntimeAuthAuditAdapter::effective_resource(
        const RuntimeAuthAuditRequestContext& request_context,
        const RuntimeAuthAuditOperationRule& rule
    )
    {
        if (!request_context.resource.resource_type.empty()
            && !request_context.resource.resource_id.empty())
        {
            return request_context.resource;
        }

        return rule.resource;
    }
}