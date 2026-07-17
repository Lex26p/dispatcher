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
    struct HttpAuthAuditRequestContext
    {
        std::string request_id{};
        std::string correlation_id{};
        std::string source{ "http-api" };

        std::string method{};
        std::string path{};

        std::string actor_id{};
        std::string actor_display_name{};

        AuthAuditActorType actor_type{
            AuthAuditActorType::anonymous
        };

        std::vector<std::string> roles{};
        std::vector<AuthorizationPermission> permissions{};

        std::string client_address{};
        std::string user_agent{};

        std::map<std::string, std::string> attributes{};
    };

    struct HttpAuthAuditEndpointRule
    {
        std::string rule_id{};

        std::string method{};
        std::string path_prefix{};

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

    struct HttpAuthAuditMappingResult
    {
        AuthenticatedOperationContext operation_context{};
        AuthorizationRequest authorization_request{};
        HttpAuthAuditEndpointRule endpoint_rule{};
    };

    struct HttpAuthAuditAuthorizeResult
    {
        HttpAuthAuditMappingResult mapping{};
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

    class HttpAuthAuditAdapter final
    {
    public:
        explicit HttpAuthAuditAdapter(
            std::vector<HttpAuthAuditEndpointRule> endpoint_rules
        );

        [[nodiscard]] const std::vector<HttpAuthAuditEndpointRule>& endpoint_rules() const noexcept;

        [[nodiscard]] HttpAuthAuditMappingResult map_request(
            const HttpAuthAuditRequestContext& request_context
        ) const;

        [[nodiscard]] HttpAuthAuditAuthorizeResult authorize_and_audit(
            const AuthorizationPolicyEvaluator& evaluator,
            AuthAuditLogger& logger,
            const HttpAuthAuditRequestContext& request_context,
            AuthorizationAuditOptions options = {}
        ) const;

        static void validate_request_context(
            const HttpAuthAuditRequestContext& request_context
        );

        static void validate_endpoint_rule(
            const HttpAuthAuditEndpointRule& rule
        );

    private:
        std::vector<HttpAuthAuditEndpointRule> endpoint_rules_{};

        [[nodiscard]] const HttpAuthAuditEndpointRule* find_matching_rule(
            const HttpAuthAuditRequestContext& request_context
        ) const;

        [[nodiscard]] static bool method_matches(
            const HttpAuthAuditEndpointRule& rule,
            const HttpAuthAuditRequestContext& request_context
        );

        [[nodiscard]] static bool path_matches(
            const HttpAuthAuditEndpointRule& rule,
            const HttpAuthAuditRequestContext& request_context
        );

        [[nodiscard]] static AuthenticatedOperationContext build_operation_context(
            const HttpAuthAuditRequestContext& request_context,
            const HttpAuthAuditEndpointRule& rule
        );

        [[nodiscard]] static AuthorizationRequest build_authorization_request(
            const HttpAuthAuditRequestContext& request_context,
            const AuthenticatedOperationContext& operation_context,
            const HttpAuthAuditEndpointRule& rule
        );
    };
}