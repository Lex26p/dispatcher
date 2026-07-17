#include <dispatcher/auth/audit/auth_authorization_policy.hpp>

#include <dispatcher/auth/audit/auth_audit_error.hpp>
#include <dispatcher/auth/audit/auth_audit_validator.hpp>

#include <utility>

namespace dispatcher::auth::audit
{
    AuthorizationDecision AuthorizationDecision::allowed(
        const AuthorizationRequest& request,
        std::string reason,
        std::string diagnostic_message
    )
    {
        AuthorizationDecision decision;

        decision.request_id =
            request.request_id;

        decision.correlation_id =
            request.correlation_id;

        decision.subject_id =
            request.subject.subject_id;

        decision.action =
            request.action;

        decision.resource =
            request.resource;

        decision.required_permission =
            request.required_permission;

        decision.effect =
            AuthorizationDecisionEffect::allow;

        decision.reason =
            std::move(
                reason
            );

        decision.diagnostic_message =
            std::move(
                diagnostic_message
            );

        decision.decided_at =
            std::chrono::system_clock::now();

        return decision;
    }

    AuthorizationDecision AuthorizationDecision::denied(
        const AuthorizationRequest& request,
        std::string reason,
        std::string diagnostic_message
    )
    {
        AuthorizationDecision decision;

        decision.request_id =
            request.request_id;

        decision.correlation_id =
            request.correlation_id;

        decision.subject_id =
            request.subject.subject_id;

        decision.action =
            request.action;

        decision.resource =
            request.resource;

        decision.required_permission =
            request.required_permission;

        decision.effect =
            AuthorizationDecisionEffect::deny;

        decision.reason =
            std::move(
                reason
            );

        decision.diagnostic_message =
            std::move(
                diagnostic_message
            );

        decision.decided_at =
            std::chrono::system_clock::now();

        return decision;
    }

    bool AuthorizationDecision::allowed() const noexcept
    {
        return effect == AuthorizationDecisionEffect::allow;
    }

    bool AuthorizationDecision::denied() const noexcept
    {
        return effect == AuthorizationDecisionEffect::deny;
    }

    AuthorizationPolicyEvaluator::AuthorizationPolicyEvaluator(
        AuthorizationPolicy policy
    )
        : policy_(
            std::move(
                policy
            )
        )
    {
        validate_policy(
            policy_
        );
    }

    const AuthorizationPolicy& AuthorizationPolicyEvaluator::policy() const noexcept
    {
        return policy_;
    }

    AuthorizationDecision AuthorizationPolicyEvaluator::evaluate(
        const AuthorizationRequest& request
    ) const
    {
        validate_request(
            request
        );

        for (const auto& rule : policy_.rules)
        {
            if (!rule.enabled)
            {
                continue;
            }

            if (rule.effect == AuthorizationDecisionEffect::deny
                && rule_matches(
                    rule,
                    request
                ))
            {
                return AuthorizationDecision::denied(
                    request,
                    "denied_by_policy_rule",
                    "Authorization denied by rule: "
                    + rule.rule_id
                    + "."
                );
            }
        }

        for (const auto& rule : policy_.rules)
        {
            if (!rule.enabled)
            {
                continue;
            }

            if (rule.effect == AuthorizationDecisionEffect::allow
                && rule_matches(
                    rule,
                    request
                ))
            {
                return AuthorizationDecision::allowed(
                    request,
                    "allowed_by_policy_rule",
                    "Authorization allowed by rule: "
                    + rule.rule_id
                    + "."
                );
            }
        }

        if (policy_.allow_direct_subject_permissions
            && subject_has_permission(
                request.subject,
                request.required_permission
            ))
        {
            return AuthorizationDecision::allowed(
                request,
                "allowed_by_subject_permission",
                "Authorization allowed by direct subject permission."
            );
        }

        if (policy_.default_effect == AuthorizationDecisionEffect::allow)
        {
            return AuthorizationDecision::allowed(
                request,
                "allowed_by_default_policy",
                "Authorization allowed by default policy."
            );
        }

        return AuthorizationDecision::denied(
            request,
            "denied_by_default_policy",
            "Authorization denied by default policy."
        );
    }

    void AuthorizationPolicyEvaluator::validate_policy(
        const AuthorizationPolicy& policy
    )
    {
        if (!is_valid_effect(
            policy.default_effect
        ))
        {
            throw AuthAuditError(
                "Authorization policy default_effect is invalid."
            );
        }

        for (const auto& rule : policy.rules)
        {
            validate_rule(
                rule
            );
        }
    }

    void AuthorizationPolicyEvaluator::validate_rule(
        const AuthorizationPolicyRule& rule
    )
    {
        if (rule.rule_id.empty())
        {
            throw AuthAuditError(
                "Authorization policy rule_id must not be empty."
            );
        }

        if (!is_valid_effect(
            rule.effect
        ))
        {
            throw AuthAuditError(
                "Authorization policy rule effect is invalid."
            );
        }

        if (!is_valid_permission(
            rule.permission
        ))
        {
            throw AuthAuditError(
                "Authorization policy rule permission is invalid."
            );
        }
    }

    void AuthorizationPolicyEvaluator::validate_request(
        const AuthorizationRequest& request
    )
    {
        if (request.request_id.empty())
        {
            throw AuthAuditError(
                "Authorization request_id must not be empty."
            );
        }

        if (request.source.empty())
        {
            throw AuthAuditError(
                "Authorization request source must not be empty."
            );
        }

        if (request.subject.subject_id.empty())
        {
            throw AuthAuditError(
                "Authorization subject_id must not be empty."
            );
        }

        if (AuthAuditValidator::actor_type_to_string(
            request.subject.subject_type
        ) == "unknown")
        {
            throw AuthAuditError(
                "Authorization subject_type is invalid."
            );
        }

        if (request.action == AuthAuditAction::unknown)
        {
            throw AuthAuditError(
                "Authorization action must not be unknown."
            );
        }

        if (AuthAuditValidator::action_to_string(
            request.action
        ) == "unknown")
        {
            throw AuthAuditError(
                "Authorization action is invalid."
            );
        }

        if (request.resource.resource_type.empty())
        {
            throw AuthAuditError(
                "Authorization resource_type must not be empty."
            );
        }

        if (request.resource.resource_id.empty())
        {
            throw AuthAuditError(
                "Authorization resource_id must not be empty."
            );
        }

        if (!is_valid_permission(
            request.required_permission
        ))
        {
            throw AuthAuditError(
                "Authorization required_permission is invalid."
            );
        }
    }

    std::string AuthorizationPolicyEvaluator::permission_to_string(
        AuthorizationPermission permission
    )
    {
        switch (permission)
        {
        case AuthorizationPermission::runtime_read:
            return "runtime_read";

        case AuthorizationPermission::runtime_control:
            return "runtime_control";

        case AuthorizationPermission::alarm_acknowledge:
            return "alarm_acknowledge";

        case AuthorizationPermission::alarm_shelve:
            return "alarm_shelve";

        case AuthorizationPermission::alarm_unshelve:
            return "alarm_unshelve";

        case AuthorizationPermission::configuration_import:
            return "configuration_import";

        case AuthorizationPermission::configuration_export:
            return "configuration_export";

        case AuthorizationPermission::notification_send:
            return "notification_send";

        case AuthorizationPermission::audit_read:
            return "audit_read";

        case AuthorizationPermission::audit_write:
            return "audit_write";

        case AuthorizationPermission::administrator:
            return "administrator";
        }

        return "unknown";
    }

    std::string AuthorizationPolicyEvaluator::effect_to_string(
        AuthorizationDecisionEffect effect
    )
    {
        switch (effect)
        {
        case AuthorizationDecisionEffect::allow:
            return "allow";

        case AuthorizationDecisionEffect::deny:
            return "deny";
        }

        return "unknown";
    }

    bool AuthorizationPolicyEvaluator::is_valid_permission(
        AuthorizationPermission permission
    )
    {
        switch (permission)
        {
        case AuthorizationPermission::runtime_read:
        case AuthorizationPermission::runtime_control:
        case AuthorizationPermission::alarm_acknowledge:
        case AuthorizationPermission::alarm_shelve:
        case AuthorizationPermission::alarm_unshelve:
        case AuthorizationPermission::configuration_import:
        case AuthorizationPermission::configuration_export:
        case AuthorizationPermission::notification_send:
        case AuthorizationPermission::audit_read:
        case AuthorizationPermission::audit_write:
        case AuthorizationPermission::administrator:
            return true;
        }

        return false;
    }

    bool AuthorizationPolicyEvaluator::is_valid_effect(
        AuthorizationDecisionEffect effect
    )
    {
        switch (effect)
        {
        case AuthorizationDecisionEffect::allow:
        case AuthorizationDecisionEffect::deny:
            return true;
        }

        return false;
    }

    bool AuthorizationPolicyEvaluator::subject_has_role(
        const AuthorizationSubject& subject,
        const std::string& role
    )
    {
        if (role.empty())
        {
            return true;
        }

        for (const auto& current_role : subject.roles)
        {
            if (current_role == role)
            {
                return true;
            }
        }

        return false;
    }

    bool AuthorizationPolicyEvaluator::subject_has_permission(
        const AuthorizationSubject& subject,
        AuthorizationPermission permission
    )
    {
        for (const auto current_permission : subject.permissions)
        {
            if (current_permission == AuthorizationPermission::administrator)
            {
                return true;
            }

            if (current_permission == permission)
            {
                return true;
            }
        }

        return false;
    }

    bool AuthorizationPolicyEvaluator::rule_matches(
        const AuthorizationPolicyRule& rule,
        const AuthorizationRequest& request
    )
    {
        return subject_has_role(
            request.subject,
            rule.role
        )
            && rule_permission_matches(
                rule.permission,
                request.required_permission
            )
            && rule_resource_matches(
                rule,
                request
            );
    }

    bool AuthorizationPolicyEvaluator::rule_permission_matches(
        AuthorizationPermission rule_permission,
        AuthorizationPermission requested_permission
    )
    {
        return rule_permission == AuthorizationPermission::administrator
            || rule_permission == requested_permission;
    }

    bool AuthorizationPolicyEvaluator::rule_resource_matches(
        const AuthorizationPolicyRule& rule,
        const AuthorizationRequest& request
    )
    {
        if (!rule.resource_type.empty()
            && rule.resource_type != request.resource.resource_type)
        {
            return false;
        }

        if (!rule.resource_id.empty()
            && rule.resource_id != request.resource.resource_id)
        {
            return false;
        }

        return true;
    }
}