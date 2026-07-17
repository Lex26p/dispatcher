#include <dispatcher/auth/audit/auth_audit_validator.hpp>

#include <dispatcher/auth/audit/auth_audit_error.hpp>

namespace dispatcher::auth::audit
{
    void AuthAuditValidator::validate_event(
        const AuthAuditEvent& event
    )
    {
        if (event.event_id.empty())
        {
            throw AuthAuditError(
                "Auth audit event_id must not be empty."
            );
        }

        if (event.source.empty())
        {
            throw AuthAuditError(
                "Auth audit source must not be empty."
            );
        }

        if (!is_valid_actor_type(
            event.actor.actor_type
        ))
        {
            throw AuthAuditError(
                "Auth audit actor_type is invalid."
            );
        }

        if (event.actor.actor_id.empty())
        {
            throw AuthAuditError(
                "Auth audit actor_id must not be empty."
            );
        }

        if (!is_valid_action(
            event.action
        ))
        {
            throw AuthAuditError(
                "Auth audit action is invalid."
            );
        }

        if (event.action == AuthAuditAction::unknown)
        {
            throw AuthAuditError(
                "Auth audit action must not be unknown."
            );
        }

        if (!is_valid_outcome(
            event.outcome
        ))
        {
            throw AuthAuditError(
                "Auth audit outcome is invalid."
            );
        }

        if (!is_valid_severity(
            event.severity
        ))
        {
            throw AuthAuditError(
                "Auth audit severity is invalid."
            );
        }

        if (event.resource.resource_type.empty())
        {
            throw AuthAuditError(
                "Auth audit resource_type must not be empty."
            );
        }

        if (event.resource.resource_id.empty())
        {
            throw AuthAuditError(
                "Auth audit resource_id must not be empty."
            );
        }

        if (event.outcome != AuthAuditOutcome::success
            && event.reason.empty())
        {
            throw AuthAuditError(
                "Auth audit non-success outcome must contain reason."
            );
        }
    }

    void AuthAuditValidator::validate_record_result(
        const AuthAuditRecordResult& result
    )
    {
        if (!is_valid_record_status(
            result.status
        ))
        {
            throw AuthAuditError(
                "Auth audit record result status is invalid."
            );
        }

        if (result.status == AuthAuditRecordStatus::failed
            && result.error_message.empty())
        {
            throw AuthAuditError(
                "Failed auth audit record result must contain error_message."
            );
        }

        if (result.status == AuthAuditRecordStatus::accepted
            && !result.error_message.empty())
        {
            throw AuthAuditError(
                "Accepted auth audit record result must not contain error_message."
            );
        }

        if (result.status == AuthAuditRecordStatus::skipped
            && result.diagnostic_message.empty())
        {
            throw AuthAuditError(
                "Skipped auth audit record result must contain diagnostic_message."
            );
        }
    }

    std::string AuthAuditValidator::actor_type_to_string(
        AuthAuditActorType actor_type
    )
    {
        switch (actor_type)
        {
        case AuthAuditActorType::system:
            return "system";

        case AuthAuditActorType::operator_user:
            return "operator_user";

        case AuthAuditActorType::service:
            return "service";

        case AuthAuditActorType::anonymous:
            return "anonymous";
        }

        return "unknown";
    }

    std::string AuthAuditValidator::action_to_string(
        AuthAuditAction action
    )
    {
        switch (action)
        {
        case AuthAuditAction::unknown:
            return "unknown";

        case AuthAuditAction::runtime_read:
            return "runtime_read";

        case AuthAuditAction::runtime_control:
            return "runtime_control";

        case AuthAuditAction::alarm_acknowledge:
            return "alarm_acknowledge";

        case AuthAuditAction::alarm_shelve:
            return "alarm_shelve";

        case AuthAuditAction::alarm_unshelve:
            return "alarm_unshelve";

        case AuthAuditAction::configuration_import:
            return "configuration_import";

        case AuthAuditAction::configuration_export:
            return "configuration_export";

        case AuthAuditAction::notification_send:
            return "notification_send";

        case AuthAuditAction::authorization_check:
            return "authorization_check";

        case AuthAuditAction::login:
            return "login";

        case AuthAuditAction::logout:
            return "logout";
        }

        return "unknown";
    }

    std::string AuthAuditValidator::outcome_to_string(
        AuthAuditOutcome outcome
    )
    {
        switch (outcome)
        {
        case AuthAuditOutcome::success:
            return "success";

        case AuthAuditOutcome::denied:
            return "denied";

        case AuthAuditOutcome::failed:
            return "failed";
        }

        return "unknown";
    }

    std::string AuthAuditValidator::severity_to_string(
        AuthAuditSeverity severity
    )
    {
        switch (severity)
        {
        case AuthAuditSeverity::info:
            return "info";

        case AuthAuditSeverity::warning:
            return "warning";

        case AuthAuditSeverity::critical:
            return "critical";
        }

        return "unknown";
    }

    std::string AuthAuditValidator::record_status_to_string(
        AuthAuditRecordStatus status
    )
    {
        switch (status)
        {
        case AuthAuditRecordStatus::accepted:
            return "accepted";

        case AuthAuditRecordStatus::failed:
            return "failed";

        case AuthAuditRecordStatus::skipped:
            return "skipped";
        }

        return "unknown";
    }

    bool AuthAuditValidator::is_valid_actor_type(
        AuthAuditActorType actor_type
    )
    {
        switch (actor_type)
        {
        case AuthAuditActorType::system:
        case AuthAuditActorType::operator_user:
        case AuthAuditActorType::service:
        case AuthAuditActorType::anonymous:
            return true;
        }

        return false;
    }

    bool AuthAuditValidator::is_valid_action(
        AuthAuditAction action
    )
    {
        switch (action)
        {
        case AuthAuditAction::unknown:
        case AuthAuditAction::runtime_read:
        case AuthAuditAction::runtime_control:
        case AuthAuditAction::alarm_acknowledge:
        case AuthAuditAction::alarm_shelve:
        case AuthAuditAction::alarm_unshelve:
        case AuthAuditAction::configuration_import:
        case AuthAuditAction::configuration_export:
        case AuthAuditAction::notification_send:
        case AuthAuditAction::authorization_check:
        case AuthAuditAction::login:
        case AuthAuditAction::logout:
            return true;
        }

        return false;
    }

    bool AuthAuditValidator::is_valid_outcome(
        AuthAuditOutcome outcome
    )
    {
        switch (outcome)
        {
        case AuthAuditOutcome::success:
        case AuthAuditOutcome::denied:
        case AuthAuditOutcome::failed:
            return true;
        }

        return false;
    }

    bool AuthAuditValidator::is_valid_severity(
        AuthAuditSeverity severity
    )
    {
        switch (severity)
        {
        case AuthAuditSeverity::info:
        case AuthAuditSeverity::warning:
        case AuthAuditSeverity::critical:
            return true;
        }

        return false;
    }

    bool AuthAuditValidator::is_valid_record_status(
        AuthAuditRecordStatus status
    )
    {
        switch (status)
        {
        case AuthAuditRecordStatus::accepted:
        case AuthAuditRecordStatus::failed:
        case AuthAuditRecordStatus::skipped:
            return true;
        }

        return false;
    }
}