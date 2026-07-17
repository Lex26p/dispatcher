#pragma once

#include <dispatcher/auth/audit/auth_audit_logger.hpp>
#include <dispatcher/auth/audit/auth_authorization_policy.hpp>
#include <dispatcher/auth/audit/auth_operation_context.hpp>
#include <dispatcher/auth/audit/authorization_audit_recorder.hpp>

#include <map>
#include <string>
#include <vector>

namespace dispatcher::auth::audit
{
    enum class RuntimeAuthAuditOperation
    {
        runtime_read,
        runtime_control,
        alarm_acknowledge,
        alarm_shelve,
        alarm_unshelve,
        configuration_import,
        configuration_export,
        notification_send
    };

    struct RuntimeAuthAuditRequestContext
    {
        std::string operation_id{};
        std::string correlation_id{};
        std::string source{ "runtime" };

        RuntimeAuthAuditOperation operation{
            RuntimeAuthAuditOperation::runtime_read
        };

        std::string actor_id{};
        std::string actor_display_name{};

        AuthAuditActorType actor_type{
            AuthAuditActorType::anonymous
        };

        std::vector<std::string> roles{};
        std::vector<AuthorizationPermission> permissions{};

        AuthAuditResource resource{};

        std::string client_address{};
        std::string user_agent{};

        std::map<std::string, std::string> attributes{};
    };

    struct RuntimeAuthAuditOperationRule
    {
        std::string rule_id{};

        RuntimeAuthAuditOperation operation{
            RuntimeAuthAuditOperation::runtime_read
        };

        AuthAuditAction action{
            AuthAuditAction::runtime_read
        };

        AuthAuditResource resource{};

        AuthorizationPermission required_permission{
            AuthorizationPermission::runtime_read
        };

        bool enabled{ true };
        bool require_authenticated{ true };
    };

    struct RuntimeAuthAuditMappingResult
    {
        AuthenticatedOperationContext operation_context{};
        AuthorizationRequest authorization_request{};
        RuntimeAuthAuditOperationRule operation_rule{};
    };

    struct RuntimeAuthAuditAuthorizeResult
    {
        RuntimeAuthAuditMappingResult mapping{};
        AuthorizationAuditRecord authorization_audit_record{};

        [[nodiscard]] bool allowed() const noexcept
        {
            return authorization_audit_record.allowed();
        }

        [[nodiscard]] bool denied() const noexcept
        {
            return authorization_audit_record.denied();
        }

        [[nodiscard]] bool audit_success() const noexcept
        {
            return authorization_audit_record.audit_success();
        }
    };

    class RuntimeAuthAuditAdapter final
    {
    public:
        explicit RuntimeAuthAuditAdapter(
            std::vector<RuntimeAuthAuditOperationRule> operation_rules
        );

        [[nodiscard]] const std::vector<RuntimeAuthAuditOperationRule>& operation_rules() const noexcept;

        [[nodiscard]] RuntimeAuthAuditMappingResult map_request(
            const RuntimeAuthAuditRequestContext& request_context
        ) const;

        [[nodiscard]] RuntimeAuthAuditAuthorizeResult authorize_and_audit(
            const AuthorizationPolicyEvaluator& evaluator,
            AuthAuditLogger& logger,
            const RuntimeAuthAuditRequestContext& request_context,
            AuthorizationAuditOptions options = {}
        ) const;

        static void validate_request_context(
            const RuntimeAuthAuditRequestContext& request_context
        );

        static void validate_operation_rule(
            const RuntimeAuthAuditOperationRule& rule
        );

        [[nodiscard]] static std::string operation_to_string(
            RuntimeAuthAuditOperation operation
        );

    private:
        std::vector<RuntimeAuthAuditOperationRule> operation_rules_{};

        [[nodiscard]] const RuntimeAuthAuditOperationRule* find_matching_rule(
            const RuntimeAuthAuditRequestContext& request_context
        ) const;

        [[nodiscard]] static bool operation_matches(
            const RuntimeAuthAuditOperationRule& rule,
            const RuntimeAuthAuditRequestContext& request_context
        );

        [[nodiscard]] static AuthenticatedOperationContext build_operation_context(
            const RuntimeAuthAuditRequestContext& request_context,
            const RuntimeAuthAuditOperationRule& rule
        );

        [[nodiscard]] static AuthorizationRequest build_authorization_request(
            const RuntimeAuthAuditRequestContext& request_context,
            const AuthenticatedOperationContext& operation_context,
            const RuntimeAuthAuditOperationRule& rule
        );

        [[nodiscard]] static AuthAuditResource effective_resource(
            const RuntimeAuthAuditRequestContext& request_context,
            const RuntimeAuthAuditOperationRule& rule
        );
    };
}