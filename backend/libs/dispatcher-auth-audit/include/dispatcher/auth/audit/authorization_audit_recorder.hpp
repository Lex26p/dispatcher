#pragma once

#include <dispatcher/auth/audit/auth_audit_logger.hpp>
#include <dispatcher/auth/audit/auth_authorization_policy.hpp>
#include <dispatcher/auth/audit/auth_operation_context.hpp>

namespace dispatcher::auth::audit
{
    struct AuthorizationAuditOptions
    {
        bool record_allowed{ true };
        bool record_denied{ true };
    };

    struct AuthorizationAuditRecord
    {
        AuthorizationDecision decision{};
        AuthAuditRecordResult audit_result{};
        bool audit_attempted{ false };

        [[nodiscard]] bool allowed() const noexcept
        {
            return decision.allowed();
        }

        [[nodiscard]] bool denied() const noexcept
        {
            return decision.denied();
        }

        [[nodiscard]] bool audit_success() const noexcept
        {
            return audit_result.success();
        }
    };

    class AuthorizationAuditRecorder final
    {
    public:
        static void validate_options(
            const AuthorizationAuditOptions& options
        );

        static void validate_decision(
            const AuthorizationDecision& decision
        );

        [[nodiscard]] static AuthAuditEvent build_event(
            const AuthenticatedOperationContext& context,
            const AuthorizationRequest& request,
            const AuthorizationDecision& decision
        );

        [[nodiscard]] static AuthAuditRecordResult record_decision(
            AuthAuditLogger& logger,
            const AuthenticatedOperationContext& context,
            const AuthorizationRequest& request,
            const AuthorizationDecision& decision,
            AuthorizationAuditOptions options = {}
        );

        [[nodiscard]] static AuthorizationAuditRecord evaluate_and_record(
            const AuthorizationPolicyEvaluator& evaluator,
            AuthAuditLogger& logger,
            const AuthenticatedOperationContext& context,
            const AuthorizationRequest& request,
            AuthorizationAuditOptions options = {}
        );

    private:
        [[nodiscard]] static bool should_record(
            const AuthorizationDecision& decision,
            const AuthorizationAuditOptions& options
        );

        [[nodiscard]] static AuthAuditOutcome outcome_for_decision(
            const AuthorizationDecision& decision
        );

        [[nodiscard]] static AuthAuditSeverity severity_for_decision(
            const AuthorizationDecision& decision
        );

        [[nodiscard]] static std::string audit_skip_reason(
            const AuthorizationDecision& decision
        );
    };
}