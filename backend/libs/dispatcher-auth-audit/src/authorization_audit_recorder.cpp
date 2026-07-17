#include <dispatcher/auth/audit/authorization_audit_recorder.hpp>

#include <dispatcher/auth/audit/auth_audit_error.hpp>
#include <dispatcher/auth/audit/auth_audit_validator.hpp>

namespace dispatcher::auth::audit
{
    void AuthorizationAuditRecorder::validate_options(
        const AuthorizationAuditOptions&
    )
    {
        // All boolean combinations are valid.
    }

    void AuthorizationAuditRecorder::validate_decision(
        const AuthorizationDecision& decision
    )
    {
        if (decision.request_id.empty())
        {
            throw AuthAuditError(
                "Authorization decision request_id must not be empty."
            );
        }

        if (decision.subject_id.empty())
        {
            throw AuthAuditError(
                "Authorization decision subject_id must not be empty."
            );
        }

        if (AuthAuditValidator::action_to_string(
            decision.action
        ) == "unknown")
        {
            throw AuthAuditError(
                "Authorization decision action is invalid."
            );
        }

        if (decision.action == AuthAuditAction::unknown)
        {
            throw AuthAuditError(
                "Authorization decision action must not be unknown."
            );
        }

        if (AuthorizationPolicyEvaluator::permission_to_string(
            decision.required_permission
        ) == "unknown")
        {
            throw AuthAuditError(
                "Authorization decision required_permission is invalid."
            );
        }

        if (AuthorizationPolicyEvaluator::effect_to_string(
            decision.effect
        ) == "unknown")
        {
            throw AuthAuditError(
                "Authorization decision effect is invalid."
            );
        }

        if (decision.resource.resource_type.empty())
        {
            throw AuthAuditError(
                "Authorization decision resource_type must not be empty."
            );
        }

        if (decision.resource.resource_id.empty())
        {
            throw AuthAuditError(
                "Authorization decision resource_id must not be empty."
            );
        }

        if (decision.reason.empty())
        {
            throw AuthAuditError(
                "Authorization decision reason must not be empty."
            );
        }
    }

    AuthAuditEvent AuthorizationAuditRecorder::build_event(
        const AuthenticatedOperationContext& context,
        const AuthorizationRequest& request,
        const AuthorizationDecision& decision
    )
    {
        AuthenticatedOperationContextBuilder::validate_context(
            context
        );

        AuthorizationPolicyEvaluator::validate_request(
            request
        );

        validate_decision(
            decision
        );

        if (decision.request_id != request.request_id)
        {
            throw AuthAuditError(
                "Authorization decision request_id does not match request."
            );
        }

        if (decision.subject_id != request.subject.subject_id)
        {
            throw AuthAuditError(
                "Authorization decision subject_id does not match request."
            );
        }

        AuthAuditEvent event;

        event.event_id =
            "authorization:"
            + decision.request_id;

        event.correlation_id =
            request.correlation_id.empty()
            ? context.correlation_id
            : request.correlation_id;

        event.source =
            context.source;

        event.actor =
            context.actor;

        event.action =
            AuthAuditAction::authorization_check;

        event.outcome =
            outcome_for_decision(
                decision
            );

        event.severity =
            severity_for_decision(
                decision
            );

        event.resource =
            request.resource;

        event.reason =
            decision.reason;

        event.diagnostic_message =
            decision.diagnostic_message;

        event.attributes =
            request.attributes;

        event.attributes.insert_or_assign(
            "operation_id",
            context.operation_id
        );

        event.attributes.insert_or_assign(
            "authorization_request_id",
            request.request_id
        );

        event.attributes.insert_or_assign(
            "authorization_subject_id",
            request.subject.subject_id
        );

        event.attributes.insert_or_assign(
            "authorization_action",
            AuthAuditValidator::action_to_string(
                request.action
            )
        );

        event.attributes.insert_or_assign(
            "required_permission",
            AuthorizationPolicyEvaluator::permission_to_string(
                request.required_permission
            )
        );

        event.attributes.insert_or_assign(
            "decision_effect",
            AuthorizationPolicyEvaluator::effect_to_string(
                decision.effect
            )
        );

        event.attributes.insert_or_assign(
            "decision_reason",
            decision.reason
        );

        if (!context.client_address.empty())
        {
            event.attributes.insert_or_assign(
                "client_address",
                context.client_address
            );
        }

        if (!context.user_agent.empty())
        {
            event.attributes.insert_or_assign(
                "user_agent",
                context.user_agent
            );
        }

        for (const auto& [key, value] : context.attributes)
        {
            event.attributes.insert_or_assign(
                "context."
                + key,
                value
            );
        }

        AuthAuditValidator::validate_event(
            event
        );

        return event;
    }

    AuthAuditRecordResult AuthorizationAuditRecorder::record_decision(
        AuthAuditLogger& logger,
        const AuthenticatedOperationContext& context,
        const AuthorizationRequest& request,
        const AuthorizationDecision& decision,
        AuthorizationAuditOptions options
    )
    {
        validate_options(
            options
        );

        const auto event =
            build_event(
                context,
                request,
                decision
            );

        if (!should_record(
            decision,
            options
        ))
        {
            return AuthAuditRecordResult::skipped(
                audit_skip_reason(
                    decision
                )
            );
        }

        return logger.record(
            event
        );
    }

    AuthorizationAuditRecord AuthorizationAuditRecorder::evaluate_and_record(
        const AuthorizationPolicyEvaluator& evaluator,
        AuthAuditLogger& logger,
        const AuthenticatedOperationContext& context,
        const AuthorizationRequest& request,
        AuthorizationAuditOptions options
    )
    {
        const auto decision =
            evaluator.evaluate(
                request
            );

        const auto audit_result =
            record_decision(
                logger,
                context,
                request,
                decision,
                options
            );

        AuthorizationAuditRecord record;

        record.decision =
            decision;

        record.audit_result =
            audit_result;

        record.audit_attempted =
            audit_result.status != AuthAuditRecordStatus::skipped;

        return record;
    }

    bool AuthorizationAuditRecorder::should_record(
        const AuthorizationDecision& decision,
        const AuthorizationAuditOptions& options
    )
    {
        if (decision.allowed())
        {
            return options.record_allowed;
        }

        if (decision.denied())
        {
            return options.record_denied;
        }

        return true;
    }

    AuthAuditOutcome AuthorizationAuditRecorder::outcome_for_decision(
        const AuthorizationDecision& decision
    )
    {
        if (decision.allowed())
        {
            return AuthAuditOutcome::success;
        }

        return AuthAuditOutcome::denied;
    }

    AuthAuditSeverity AuthorizationAuditRecorder::severity_for_decision(
        const AuthorizationDecision& decision
    )
    {
        if (decision.allowed())
        {
            return AuthAuditSeverity::info;
        }

        return AuthAuditSeverity::warning;
    }

    std::string AuthorizationAuditRecorder::audit_skip_reason(
        const AuthorizationDecision& decision
    )
    {
        if (decision.allowed())
        {
            return "Authorization audit recording is disabled for allowed decisions.";
        }

        return "Authorization audit recording is disabled for denied decisions.";
    }
}