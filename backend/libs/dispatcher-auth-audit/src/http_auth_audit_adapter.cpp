#include <dispatcher/auth/audit/http_auth_audit_adapter.hpp>

#include <dispatcher/auth/audit/auth_audit_error.hpp>
#include <dispatcher/auth/audit/auth_audit_validator.hpp>

#include <cstddef>
#include <utility>

namespace dispatcher::auth::audit
{
    namespace
    {
        [[nodiscard]] bool starts_with(
            const std::string& value,
            const std::string& prefix
        )
        {
            return value.rfind(
                prefix,
                0
            ) == 0;
        }
    }

    HttpAuthAuditAdapter::HttpAuthAuditAdapter(
        std::vector<HttpAuthAuditEndpointRule> endpoint_rules
    )
        : endpoint_rules_(
            std::move(
                endpoint_rules
            )
        )
    {
        if (endpoint_rules_.empty())
        {
            throw AuthAuditError(
                "HTTP auth-audit adapter requires at least one endpoint rule."
            );
        }

        for (const auto& rule : endpoint_rules_)
        {
            validate_endpoint_rule(
                rule
            );
        }
    }

    const std::vector<HttpAuthAuditEndpointRule>& HttpAuthAuditAdapter::endpoint_rules() const noexcept
    {
        return endpoint_rules_;
    }

    HttpAuthAuditMappingResult HttpAuthAuditAdapter::map_request(
        const HttpAuthAuditRequestContext& request_context
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
                "No HTTP auth-audit endpoint rule matches request."
            );
        }

        if (rule->require_authenticated
            && (request_context.actor_id.empty()
                || request_context.actor_type == AuthAuditActorType::anonymous))
        {
            throw AuthAuditError(
                "HTTP auth-audit endpoint requires an authenticated actor."
            );
        }

        HttpAuthAuditMappingResult result;

        result.endpoint_rule =
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

    HttpAuthAuditAuthorizeResult HttpAuthAuditAdapter::authorize_and_audit(
        const AuthorizationPolicyEvaluator& evaluator,
        AuthAuditLogger& logger,
        const HttpAuthAuditRequestContext& request_context,
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

        HttpAuthAuditAuthorizeResult result;

        result.mapping =
            mapping;

        result.authorization_audit_record =
            authorization_audit_record;

        return result;
    }

    void HttpAuthAuditAdapter::validate_request_context(
        const HttpAuthAuditRequestContext& request_context
    )
    {
        if (request_context.request_id.empty())
        {
            throw AuthAuditError(
                "HTTP auth-audit request_id must not be empty."
            );
        }

        if (request_context.source.empty())
        {
            throw AuthAuditError(
                "HTTP auth-audit source must not be empty."
            );
        }

        if (request_context.method.empty())
        {
            throw AuthAuditError(
                "HTTP auth-audit method must not be empty."
            );
        }

        if (request_context.path.empty())
        {
            throw AuthAuditError(
                "HTTP auth-audit path must not be empty."
            );
        }

        if (AuthAuditValidator::actor_type_to_string(
            request_context.actor_type
        ) == "unknown")
        {
            throw AuthAuditError(
                "HTTP auth-audit actor_type is invalid."
            );
        }
    }

    void HttpAuthAuditAdapter::validate_endpoint_rule(
        const HttpAuthAuditEndpointRule& rule
    )
    {
        if (rule.rule_id.empty())
        {
            throw AuthAuditError(
                "HTTP auth-audit endpoint rule_id must not be empty."
            );
        }

        if (rule.method.empty())
        {
            throw AuthAuditError(
                "HTTP auth-audit endpoint rule method must not be empty."
            );
        }

        if (rule.path_prefix.empty())
        {
            throw AuthAuditError(
                "HTTP auth-audit endpoint rule path_prefix must not be empty."
            );
        }

        if (rule.action == AuthAuditAction::unknown)
        {
            throw AuthAuditError(
                "HTTP auth-audit endpoint rule action must not be unknown."
            );
        }

        if (AuthAuditValidator::action_to_string(
            rule.action
        ) == "unknown")
        {
            throw AuthAuditError(
                "HTTP auth-audit endpoint rule action is invalid."
            );
        }

        if (AuthorizationPolicyEvaluator::permission_to_string(
            rule.required_permission
        ) == "unknown")
        {
            throw AuthAuditError(
                "HTTP auth-audit endpoint rule required_permission is invalid."
            );
        }

        if (rule.resource.resource_type.empty())
        {
            throw AuthAuditError(
                "HTTP auth-audit endpoint rule resource_type must not be empty."
            );
        }

        if (rule.resource.resource_id.empty())
        {
            throw AuthAuditError(
                "HTTP auth-audit endpoint rule resource_id must not be empty."
            );
        }
    }

    const HttpAuthAuditEndpointRule* HttpAuthAuditAdapter::find_matching_rule(
        const HttpAuthAuditRequestContext& request_context
    ) const
    {
        const HttpAuthAuditEndpointRule* best_match = nullptr;
        std::size_t best_prefix_size = 0;

        for (const auto& rule : endpoint_rules_)
        {
            if (!rule.enabled)
            {
                continue;
            }

            if (!method_matches(
                rule,
                request_context
            ))
            {
                continue;
            }

            if (!path_matches(
                rule,
                request_context
            ))
            {
                continue;
            }

            if (rule.path_prefix.size() >= best_prefix_size)
            {
                best_match =
                    &rule;

                best_prefix_size =
                    rule.path_prefix.size();
            }
        }

        return best_match;
    }

    bool HttpAuthAuditAdapter::method_matches(
        const HttpAuthAuditEndpointRule& rule,
        const HttpAuthAuditRequestContext& request_context
    )
    {
        return rule.method == request_context.method;
    }

    bool HttpAuthAuditAdapter::path_matches(
        const HttpAuthAuditEndpointRule& rule,
        const HttpAuthAuditRequestContext& request_context
    )
    {
        return starts_with(
            request_context.path,
            rule.path_prefix
        );
    }

    AuthenticatedOperationContext HttpAuthAuditAdapter::build_operation_context(
        const HttpAuthAuditRequestContext& request_context,
        const HttpAuthAuditEndpointRule&
    )
    {
        AuthenticatedOperationContext context;

        context.operation_id =
            request_context.request_id;

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
            "http.method",
            request_context.method
        );

        context.attributes.insert_or_assign(
            "http.path",
            request_context.path
        );

        AuthenticatedOperationContextBuilder::validate_context(
            context
        );

        return context;
    }

    AuthorizationRequest HttpAuthAuditAdapter::build_authorization_request(
        const HttpAuthAuditRequestContext& request_context,
        const AuthenticatedOperationContext& operation_context,
        const HttpAuthAuditEndpointRule& rule
    )
    {
        AuthorizationRequest request;

        request.request_id =
            request_context.request_id
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
            rule.resource;

        request.required_permission =
            rule.required_permission;

        request.attributes =
            request_context.attributes;

        request.attributes.insert_or_assign(
            "http.method",
            request_context.method
        );

        request.attributes.insert_or_assign(
            "http.path",
            request_context.path
        );

        request.attributes.insert_or_assign(
            "http.endpoint_rule_id",
            rule.rule_id
        );

        AuthorizationPolicyEvaluator::validate_request(
            request
        );

        return request;
    }
}