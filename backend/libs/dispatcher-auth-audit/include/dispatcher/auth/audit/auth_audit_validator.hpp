#pragma once

#include <dispatcher/auth/audit/auth_audit_types.hpp>

#include <string>

namespace dispatcher::auth::audit
{
    class AuthAuditValidator final
    {
    public:
        static void validate_event(
            const AuthAuditEvent& event
        );

        static void validate_record_result(
            const AuthAuditRecordResult& result
        );

        [[nodiscard]] static std::string actor_type_to_string(
            AuthAuditActorType actor_type
        );

        [[nodiscard]] static std::string action_to_string(
            AuthAuditAction action
        );

        [[nodiscard]] static std::string outcome_to_string(
            AuthAuditOutcome outcome
        );

        [[nodiscard]] static std::string severity_to_string(
            AuthAuditSeverity severity
        );

        [[nodiscard]] static std::string record_status_to_string(
            AuthAuditRecordStatus status
        );

    private:
        [[nodiscard]] static bool is_valid_actor_type(
            AuthAuditActorType actor_type
        );

        [[nodiscard]] static bool is_valid_action(
            AuthAuditAction action
        );

        [[nodiscard]] static bool is_valid_outcome(
            AuthAuditOutcome outcome
        );

        [[nodiscard]] static bool is_valid_severity(
            AuthAuditSeverity severity
        );

        [[nodiscard]] static bool is_valid_record_status(
            AuthAuditRecordStatus status
        );
    };
}