#include <dispatcher/auth/audit/auth_audit.hpp>

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace
{
    dispatcher::auth::audit::HttpAuthAuditRequestContext make_http_context(
        std::string method = "GET",
        std::string path = "/api/v1/runtime",
        dispatcher::auth::audit::AuthorizationPermission permission =
        dispatcher::auth::audit::AuthorizationPermission::runtime_read
    )
    {
        dispatcher::auth::audit::HttpAuthAuditRequestContext context;

        context.request_id = "http-request-1";
        context.correlation_id = "correlation-1";
        context.source = "http-api";

        context.method =
            std::move(
                method
            );

        context.path =
            std::move(
                path
            );

        context.actor_id = "operator-1";
        context.actor_display_name = "Operator One";
        context.actor_type =
            dispatcher::auth::audit::AuthAuditActorType::operator_user;

        context.roles.push_back(
            "operators"
        );

        context.permissions.push_back(
            permission
        );

        context.client_address = "127.0.0.1";
        context.user_agent = "dispatcher-test";

        context.attributes.emplace(
            "request.header.x-request-id",
            "http-request-1"
        );

        return context;
    }

    dispatcher::auth::audit::HttpAuthAuditEndpointRule make_runtime_rule()
    {
        dispatcher::auth::audit::HttpAuthAuditEndpointRule rule;

        rule.rule_id = "runtime-read";
        rule.method = "GET";
        rule.path_prefix = "/api/v1/runtime";
        rule.action =
            dispatcher::auth::audit::AuthAuditAction::runtime_read;

        rule.resource.resource_type = "runtime";
        rule.resource.resource_id = "dispatcher-runtime";
        rule.resource.display_name = "Dispatcher runtime";

        rule.required_permission =
            dispatcher::auth::audit::AuthorizationPermission::runtime_read;

        rule.enabled = true;
        rule.require_authenticated = true;

        return rule;
    }

    dispatcher::auth::audit::HttpAuthAuditEndpointRule make_alarm_ack_rule()
    {
        dispatcher::auth::audit::HttpAuthAuditEndpointRule rule;

        rule.rule_id = "alarm-acknowledge";
        rule.method = "POST";
        rule.path_prefix = "/api/v1/alarms";
        rule.action =
            dispatcher::auth::audit::AuthAuditAction::alarm_acknowledge;

        rule.resource.resource_type = "alarm";
        rule.resource.resource_id = "alarms";
        rule.resource.display_name = "Alarms";

        rule.required_permission =
            dispatcher::auth::audit::AuthorizationPermission::alarm_acknowledge;

        rule.enabled = true;
        rule.require_authenticated = true;

        return rule;
    }

    dispatcher::auth::audit::HttpAuthAuditEndpointRule make_alarm_ack_detail_rule()
    {
        auto rule =
            make_alarm_ack_rule();

        rule.rule_id = "alarm-acknowledge-detail";
        rule.path_prefix = "/api/v1/alarms/alarm-1";
        rule.resource.resource_id = "alarm-1";
        rule.resource.display_name = "Pump pressure high";

        return rule;
    }

    dispatcher::auth::audit::AuthorizationPolicy make_allow_operator_policy(
        dispatcher::auth::audit::AuthorizationPermission permission =
        dispatcher::auth::audit::AuthorizationPermission::runtime_read
    )
    {
        dispatcher::auth::audit::AuthorizationPolicy policy;

        policy.allow_direct_subject_permissions = false;

        dispatcher::auth::audit::AuthorizationPolicyRule rule;

        rule.rule_id = "allow-operator";
        rule.effect =
            dispatcher::auth::audit::AuthorizationDecisionEffect::allow;
        rule.permission =
            permission;
        rule.role = "operators";
        rule.resource_type = "";
        rule.resource_id = "";
        rule.enabled = true;

        policy.rules.push_back(
            rule
        );

        return policy;
    }

    void expect_adapter_construction_throws(
        const std::vector<dispatcher::auth::audit::HttpAuthAuditEndpointRule>& rules
    )
    {
        EXPECT_THROW(
            {
                dispatcher::auth::audit::HttpAuthAuditAdapter adapter{
                    rules
                };

                static_cast<void>(
                    adapter
                );
            },
            dispatcher::auth::audit::AuthAuditError
        );
    }

    void expect_context_validation_throws(
        const dispatcher::auth::audit::HttpAuthAuditRequestContext& context
    )
    {
        EXPECT_THROW(
            dispatcher::auth::audit::HttpAuthAuditAdapter::validate_request_context(
                context
            ),
            dispatcher::auth::audit::AuthAuditError
        );
    }

    void expect_rule_validation_throws(
        const dispatcher::auth::audit::HttpAuthAuditEndpointRule& rule
    )
    {
        EXPECT_THROW(
            dispatcher::auth::audit::HttpAuthAuditAdapter::validate_endpoint_rule(
                rule
            ),
            dispatcher::auth::audit::AuthAuditError
        );
    }

    void expect_map_request_throws(
        const dispatcher::auth::audit::HttpAuthAuditAdapter& adapter,
        const dispatcher::auth::audit::HttpAuthAuditRequestContext& context
    )
    {
        EXPECT_THROW(
            {
                const auto result =
                    adapter.map_request(
                        context
                    );

                static_cast<void>(
                    result
                );
            },
            dispatcher::auth::audit::AuthAuditError
        );
    }
}

TEST(AuthHttpAdapterTests, ValidatesHttpRequestContext)
{
    const auto context =
        make_http_context();

    EXPECT_NO_THROW(
        dispatcher::auth::audit::HttpAuthAuditAdapter::validate_request_context(
            context
        )
    );
}

TEST(AuthHttpAdapterTests, RejectsEmptyRequestId)
{
    auto context =
        make_http_context();

    context.request_id = "";

    expect_context_validation_throws(
        context
    );
}

TEST(AuthHttpAdapterTests, RejectsEmptySource)
{
    auto context =
        make_http_context();

    context.source = "";

    expect_context_validation_throws(
        context
    );
}

TEST(AuthHttpAdapterTests, RejectsEmptyMethod)
{
    auto context =
        make_http_context();

    context.method = "";

    expect_context_validation_throws(
        context
    );
}

TEST(AuthHttpAdapterTests, RejectsEmptyPath)
{
    auto context =
        make_http_context();

    context.path = "";

    expect_context_validation_throws(
        context
    );
}

TEST(AuthHttpAdapterTests, RejectsInvalidActorType)
{
    auto context =
        make_http_context();

    context.actor_type =
        static_cast<dispatcher::auth::audit::AuthAuditActorType>(
            999
            );

    expect_context_validation_throws(
        context
    );
}

TEST(AuthHttpAdapterTests, ValidatesEndpointRule)
{
    const auto rule =
        make_runtime_rule();

    EXPECT_NO_THROW(
        dispatcher::auth::audit::HttpAuthAuditAdapter::validate_endpoint_rule(
            rule
        )
    );
}

TEST(AuthHttpAdapterTests, RejectsEmptyRuleId)
{
    auto rule =
        make_runtime_rule();

    rule.rule_id = "";

    expect_rule_validation_throws(
        rule
    );
}

TEST(AuthHttpAdapterTests, RejectsEmptyRuleMethod)
{
    auto rule =
        make_runtime_rule();

    rule.method = "";

    expect_rule_validation_throws(
        rule
    );
}

TEST(AuthHttpAdapterTests, RejectsEmptyRulePathPrefix)
{
    auto rule =
        make_runtime_rule();

    rule.path_prefix = "";

    expect_rule_validation_throws(
        rule
    );
}

TEST(AuthHttpAdapterTests, RejectsUnknownRuleAction)
{
    auto rule =
        make_runtime_rule();

    rule.action =
        dispatcher::auth::audit::AuthAuditAction::unknown;

    expect_rule_validation_throws(
        rule
    );
}

TEST(AuthHttpAdapterTests, RejectsInvalidRulePermission)
{
    auto rule =
        make_runtime_rule();

    rule.required_permission =
        static_cast<dispatcher::auth::audit::AuthorizationPermission>(
            999
            );

    expect_rule_validation_throws(
        rule
    );
}

TEST(AuthHttpAdapterTests, RejectsEmptyRuleResource)
{
    auto rule =
        make_runtime_rule();

    rule.resource.resource_id = "";

    expect_rule_validation_throws(
        rule
    );
}

TEST(AuthHttpAdapterTests, ConstructorRejectsEmptyRules)
{
    expect_adapter_construction_throws(
        {}
    );
}

TEST(AuthHttpAdapterTests, ConstructorValidatesRules)
{
    auto rule =
        make_runtime_rule();

    rule.rule_id = "";

    expect_adapter_construction_throws(
        {
            rule
        }
    );
}

TEST(AuthHttpAdapterTests, MapsHttpContextToOperationContextAndAuthorizationRequest)
{
    dispatcher::auth::audit::HttpAuthAuditAdapter adapter{
        {
            make_runtime_rule()
        }
    };

    const auto mapping =
        adapter.map_request(
            make_http_context()
        );

    EXPECT_EQ(
        mapping.endpoint_rule.rule_id,
        "runtime-read"
    );

    EXPECT_EQ(
        mapping.operation_context.operation_id,
        "http-request-1"
    );

    EXPECT_EQ(
        mapping.operation_context.correlation_id,
        "correlation-1"
    );

    EXPECT_EQ(
        mapping.operation_context.source,
        "http-api"
    );

    EXPECT_EQ(
        mapping.operation_context.actor.actor_id,
        "operator-1"
    );

    EXPECT_EQ(
        mapping.operation_context.client_address,
        "127.0.0.1"
    );

    EXPECT_EQ(
        mapping.operation_context.attributes.at(
            "http.method"
        ),
        "GET"
    );

    EXPECT_EQ(
        mapping.operation_context.attributes.at(
            "http.path"
        ),
        "/api/v1/runtime"
    );

    EXPECT_EQ(
        mapping.authorization_request.request_id,
        "http-request-1:authorization"
    );

    EXPECT_EQ(
        mapping.authorization_request.subject.subject_id,
        "operator-1"
    );

    EXPECT_EQ(
        mapping.authorization_request.action,
        dispatcher::auth::audit::AuthAuditAction::runtime_read
    );

    EXPECT_EQ(
        mapping.authorization_request.required_permission,
        dispatcher::auth::audit::AuthorizationPermission::runtime_read
    );

    EXPECT_EQ(
        mapping.authorization_request.resource.resource_type,
        "runtime"
    );

    EXPECT_EQ(
        mapping.authorization_request.attributes.at(
            "http.endpoint_rule_id"
        ),
        "runtime-read"
    );
}

TEST(AuthHttpAdapterTests, RejectsNoMatchingRule)
{
    dispatcher::auth::audit::HttpAuthAuditAdapter adapter{
        {
            make_runtime_rule()
        }
    };

    expect_map_request_throws(
        adapter,
        make_http_context(
            "GET",
            "/api/v1/unknown"
        )
    );
}

TEST(AuthHttpAdapterTests, DisabledRuleIsIgnored)
{
    auto rule =
        make_runtime_rule();

    rule.enabled = false;

    dispatcher::auth::audit::HttpAuthAuditAdapter adapter{
        {
            rule
        }
    };

    expect_map_request_throws(
        adapter,
        make_http_context()
    );
}

TEST(AuthHttpAdapterTests, MethodMustMatch)
{
    dispatcher::auth::audit::HttpAuthAuditAdapter adapter{
        {
            make_runtime_rule()
        }
    };

    expect_map_request_throws(
        adapter,
        make_http_context(
            "POST",
            "/api/v1/runtime"
        )
    );
}

TEST(AuthHttpAdapterTests, LongestPathPrefixWins)
{
    dispatcher::auth::audit::HttpAuthAuditAdapter adapter{
        {
            make_alarm_ack_rule(),
            make_alarm_ack_detail_rule()
        }
    };

    auto context =
        make_http_context(
            "POST",
            "/api/v1/alarms/alarm-1/acknowledge",
            dispatcher::auth::audit::AuthorizationPermission::alarm_acknowledge
        );

    context.permissions.clear();

    context.permissions.push_back(
        dispatcher::auth::audit::AuthorizationPermission::alarm_acknowledge
    );

    const auto mapping =
        adapter.map_request(
            context
        );

    EXPECT_EQ(
        mapping.endpoint_rule.rule_id,
        "alarm-acknowledge-detail"
    );

    EXPECT_EQ(
        mapping.authorization_request.resource.resource_id,
        "alarm-1"
    );
}

TEST(AuthHttpAdapterTests, AuthenticatedEndpointRejectsAnonymousActor)
{
    dispatcher::auth::audit::HttpAuthAuditAdapter adapter{
        {
            make_runtime_rule()
        }
    };

    auto context =
        make_http_context();

    context.actor_id = "";
    context.actor_display_name = "";
    context.actor_type =
        dispatcher::auth::audit::AuthAuditActorType::anonymous;

    expect_map_request_throws(
        adapter,
        context
    );
}

TEST(AuthHttpAdapterTests, PublicEndpointCanMapAnonymousActor)
{
    auto rule =
        make_runtime_rule();

    rule.require_authenticated = false;

    dispatcher::auth::audit::HttpAuthAuditAdapter adapter{
        {
            rule
        }
    };

    auto context =
        make_http_context();

    context.actor_id = "";
    context.actor_display_name = "";
    context.actor_type =
        dispatcher::auth::audit::AuthAuditActorType::anonymous;
    context.roles.clear();
    context.permissions.clear();

    const auto mapping =
        adapter.map_request(
            context
        );

    EXPECT_EQ(
        mapping.operation_context.actor.actor_id,
        "anonymous"
    );

    EXPECT_EQ(
        mapping.authorization_request.subject.subject_id,
        "anonymous"
    );
}

TEST(AuthHttpAdapterTests, AuthorizeAndAuditAllowedRequest)
{
    dispatcher::auth::audit::HttpAuthAuditAdapter adapter{
        {
            make_runtime_rule()
        }
    };

    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator{
        make_allow_operator_policy(
            dispatcher::auth::audit::AuthorizationPermission::runtime_read
        )
    };

    const auto result =
        adapter.authorize_and_audit(
            evaluator,
            logger,
            make_http_context()
        );

    EXPECT_TRUE(
        result.allowed()
    );

    EXPECT_TRUE(
        result.audit_success()
    );

    EXPECT_EQ(
        result.mapping.endpoint_rule.rule_id,
        "runtime-read"
    );

    ASSERT_EQ(
        sink.recorded_events().size(),
        1U
    );

    EXPECT_EQ(
        sink.recorded_events()[0].action,
        dispatcher::auth::audit::AuthAuditAction::authorization_check
    );

    EXPECT_EQ(
        sink.recorded_events()[0].outcome,
        dispatcher::auth::audit::AuthAuditOutcome::success
    );

    EXPECT_EQ(
        sink.recorded_events()[0].attributes.at(
            "http.path"
        ),
        "/api/v1/runtime"
    );

    EXPECT_EQ(
        sink.recorded_events()[0].attributes.at(
            "required_permission"
        ),
        "runtime_read"
    );
}

TEST(AuthHttpAdapterTests, AuthorizeAndAuditDeniedRequest)
{
    dispatcher::auth::audit::HttpAuthAuditAdapter adapter{
        {
            make_alarm_ack_rule()
        }
    };

    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator;

    auto context =
        make_http_context(
            "POST",
            "/api/v1/alarms/alarm-2/acknowledge",
            dispatcher::auth::audit::AuthorizationPermission::runtime_read
        );

    context.permissions.clear();

    context.permissions.push_back(
        dispatcher::auth::audit::AuthorizationPermission::runtime_read
    );

    const auto result =
        adapter.authorize_and_audit(
            evaluator,
            logger,
            context
        );

    EXPECT_TRUE(
        result.denied()
    );

    EXPECT_TRUE(
        result.audit_success()
    );

    EXPECT_EQ(
        result.authorization_audit_record.decision.reason,
        "denied_by_default_policy"
    );

    ASSERT_EQ(
        sink.recorded_events().size(),
        1U
    );

    EXPECT_EQ(
        sink.recorded_events()[0].outcome,
        dispatcher::auth::audit::AuthAuditOutcome::denied
    );

    EXPECT_EQ(
        sink.recorded_events()[0].severity,
        dispatcher::auth::audit::AuthAuditSeverity::warning
    );

    EXPECT_EQ(
        sink.recorded_events()[0].attributes.at(
            "required_permission"
        ),
        "alarm_acknowledge"
    );
}

TEST(AuthHttpAdapterTests, AuthorizeAndAuditCanSkipAllowedAudit)
{
    dispatcher::auth::audit::HttpAuthAuditAdapter adapter{
        {
            make_runtime_rule()
        }
    };

    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator;

    dispatcher::auth::audit::AuthorizationAuditOptions options;

    options.record_allowed = false;
    options.record_denied = true;

    const auto result =
        adapter.authorize_and_audit(
            evaluator,
            logger,
            make_http_context(),
            options
        );

    EXPECT_TRUE(
        result.allowed()
    );

    EXPECT_FALSE(
        result.authorization_audit_record.audit_attempted
    );

    EXPECT_EQ(
        result.authorization_audit_record.audit_result.status,
        dispatcher::auth::audit::AuthAuditRecordStatus::skipped
    );

    EXPECT_TRUE(
        sink.recorded_events().empty()
    );
}

TEST(AuthHttpAdapterTests, AuthorizeAndAuditPropagatesAuditFailureAsResult)
{
    dispatcher::auth::audit::HttpAuthAuditAdapter adapter{
        {
            make_runtime_rule()
        }
    };

    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    sink.set_failure(
        "audit storage unavailable"
    );

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator;

    const auto result =
        adapter.authorize_and_audit(
            evaluator,
            logger,
            make_http_context()
        );

    EXPECT_TRUE(
        result.allowed()
    );

    EXPECT_TRUE(
        result.authorization_audit_record.audit_attempted
    );

    EXPECT_TRUE(
        result.authorization_audit_record.audit_result.failure()
    );

    EXPECT_EQ(
        result.authorization_audit_record.audit_result.error_message,
        "audit storage unavailable"
    );

    EXPECT_TRUE(
        sink.recorded_events().empty()
    );
}