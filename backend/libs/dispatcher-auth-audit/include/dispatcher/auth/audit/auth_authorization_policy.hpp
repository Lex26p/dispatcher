#pragma once

#include <dispatcher/auth/audit/auth_audit_types.hpp>

#include <chrono>
#include <map>
#include <string>
#include <vector>

namespace dispatcher::auth::audit
{
    enum class AuthorizationPermission
    {
        runtime_read,
        runtime_control,
        alarm_acknowledge,
        alarm_shelve,
        alarm_unshelve,
        configuration_import,
        configuration_export,
        notification_send,
        audit_read,
        audit_write,
        administrator
    };

    enum class AuthorizationDecisionEffect
    {
        allow,
        deny
    };

    struct AuthorizationSubject
    {
        std::string subject_id{};
        std::string display_name{};

        AuthAuditActorType subject_type{
            AuthAuditActorType::anonymous
        };

        std::vector<std::string> roles{};
        std::vector<AuthorizationPermission> permissions{};
    };

    struct AuthorizationRequest
    {
        std::string request_id{};
        std::string correlation_id{};
        std::string source{};

        AuthorizationSubject subject{};

        AuthAuditAction action{
            AuthAuditAction::unknown
        };

        AuthAuditResource resource{};

        AuthorizationPermission required_permission{
            AuthorizationPermission::runtime_read
        };

        std::map<std::string, std::string> attributes{};
    };

    struct AuthorizationDecision
    {
        std::string request_id{};
        std::string correlation_id{};
        std::string subject_id{};

        AuthAuditAction action{
            AuthAuditAction::unknown
        };

        AuthAuditResource resource{};

        AuthorizationPermission required_permission{
            AuthorizationPermission::runtime_read
        };

        AuthorizationDecisionEffect effect{
            AuthorizationDecisionEffect::deny
        };

        std::string reason{};
        std::string diagnostic_message{};

        std::chrono::system_clock::time_point decided_at{
            std::chrono::system_clock::now()
        };

        [[nodiscard]] static AuthorizationDecision allowed(
            const AuthorizationRequest& request,
            std::string reason,
            std::string diagnostic_message = {}
        );

        [[nodiscard]] static AuthorizationDecision denied(
            const AuthorizationRequest& request,
            std::string reason,
            std::string diagnostic_message = {}
        );

        [[nodiscard]] bool allowed() const noexcept;

        [[nodiscard]] bool denied() const noexcept;
    };

    struct AuthorizationPolicyRule
    {
        std::string rule_id{};

        AuthorizationDecisionEffect effect{
            AuthorizationDecisionEffect::allow
        };

        AuthorizationPermission permission{
            AuthorizationPermission::runtime_read
        };

        std::string role{};
        std::string resource_type{};
        std::string resource_id{};

        bool enabled{ true };
    };

    struct AuthorizationPolicy
    {
        AuthorizationDecisionEffect default_effect{
            AuthorizationDecisionEffect::deny
        };

        bool allow_direct_subject_permissions{ true };

        std::vector<AuthorizationPolicyRule> rules{};
    };

    class AuthorizationPolicyEvaluator final
    {
    public:
        explicit AuthorizationPolicyEvaluator(
            AuthorizationPolicy policy = {}
        );

        [[nodiscard]] const AuthorizationPolicy& policy() const noexcept;

        [[nodiscard]] AuthorizationDecision evaluate(
            const AuthorizationRequest& request
        ) const;

        static void validate_policy(
            const AuthorizationPolicy& policy
        );

        static void validate_rule(
            const AuthorizationPolicyRule& rule
        );

        static void validate_request(
            const AuthorizationRequest& request
        );

        [[nodiscard]] static std::string permission_to_string(
            AuthorizationPermission permission
        );

        [[nodiscard]] static std::string effect_to_string(
            AuthorizationDecisionEffect effect
        );

    private:
        AuthorizationPolicy policy_{};

        [[nodiscard]] static bool is_valid_permission(
            AuthorizationPermission permission
        );

        [[nodiscard]] static bool is_valid_effect(
            AuthorizationDecisionEffect effect
        );

        [[nodiscard]] static bool subject_has_role(
            const AuthorizationSubject& subject,
            const std::string& role
        );

        [[nodiscard]] static bool subject_has_permission(
            const AuthorizationSubject& subject,
            AuthorizationPermission permission
        );

        [[nodiscard]] static bool rule_matches(
            const AuthorizationPolicyRule& rule,
            const AuthorizationRequest& request
        );

        [[nodiscard]] static bool rule_permission_matches(
            AuthorizationPermission rule_permission,
            AuthorizationPermission requested_permission
        );

        [[nodiscard]] static bool rule_resource_matches(
            const AuthorizationPolicyRule& rule,
            const AuthorizationRequest& request
        );
    };
}