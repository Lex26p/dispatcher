#include <dispatcher/auth/audit/auth_audit.hpp>

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace
{
    dispatcher::auth::audit::RuntimeAuthAuditOperationRule make_runtime_read_rule()
    {
        dispatcher::auth::audit::RuntimeAuthAuditOperationRule rule;

        rule.rule_id = "runtime-read";
        rule.operation =
            dispatcher::auth::audit::RuntimeAuthAuditOperation::runtime_read;
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

    dispatcher::auth::audit::RuntimeAuthAuditOperationRule make_runtime_control_rule()
    {
        dispatcher::auth::audit::RuntimeAuthAuditOperationRule rule;

        rule.rule_id = "runtime-control";
        rule.operation =
            dispatcher::auth::audit::RuntimeAuthAuditOperation::runtime_control;
        rule.action =
            dispatcher::auth::audit::AuthAuditAction::runtime_control;

        rule.resource.resource_type = "runtime";
        rule.resource.resource_id = "dispatcher-runtime";
        rule.resource.display_name = "Dispatcher runtime";

        rule.required_permission =
            dispatcher::auth::audit::AuthorizationPermission::runtime_control;

        rule.enabled = true;
        rule.require_authenticated = true;

        return rule;
    }

    dispatcher::auth::audit::RuntimeAuthAuditOperationRule make_alarm_ack_rule()
    {
        dispatcher::auth::audit::RuntimeAuthAuditOperationRule rule;

        rule.rule_id = "alarm-acknowledge";
        rule.operation =
            dispatcher::auth::audit::RuntimeAuthAuditOperation::alarm_acknowledge;
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

    dispatcher::auth::audit::RuntimeAuthAuditOperationRule make_notification_send_rule()
    {
        dispatcher::auth::audit::RuntimeAuthAuditOperationRule rule;

        rule.rule_id = "notification-send";
        rule.operation =
            dispatcher::auth::audit::RuntimeAuthAuditOperation::notification_send;
        rule.action =
            dispatcher::auth::audit::AuthAuditAction::notification_send;

        rule.resource.resource_type = "notification";
        rule.resource.resource_id = "notification-delivery";
        rule.resource.display_name = "Notification delivery";

        rule.required_permission =
            dispatcher::auth::audit::AuthorizationPermission::notification_send;

        rule.enabled = true;
        rule.require_authenticated = true;

        return rule;
    }

    dispatcher::auth::audit::RuntimeAuthAuditRequestContext make_runtime_context(
        dispatcher::auth::audit::RuntimeAuthAuditOperation operation =
        dispatcher::auth::audit::RuntimeAuthAuditOperation::runtime_read,
        dispatcher::auth::audit::AuthorizationPermission permission =
        dispatcher::auth::audit::AuthorizationPermission::runtime_read
    )
    {
        dispatcher::auth::audit::RuntimeAuthAuditRequestContext context;

        context.operation_id = "runtime-operation-1";
        context.correlation_id = "correlation-1";
        context.source = "runtime";

        context.operation =
            operation;

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
        context.user_agent = "dispatcher-runtime-test";

        context.attributes.emplace(
            "test_case",
            "runtime-auth-audit"
        );

        return context;
    }

    dispatcher::auth::audit::AuthorizationPolicy make_allow_operator_policy(
        dispatcher::auth::audit::AuthorizationPermission permission
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
        const std::vector<dispatcher::auth::audit::RuntimeAuthAuditOperationRule>& rules
    )
    {
        EXPECT_THROW(
            {
                dispatcher::auth::audit::RuntimeAuthAuditAdapter adapter{
                    rules
                };

                static_cast<void>(
                    adapter
                );
            },
            dispatcher::auth::audit::AuthAuditError
        );
    }

    void expect_request_validation_throws(
        const dispatcher::auth::audit::RuntimeAuthAuditRequestContext& context
    )
    {
        EXPECT_THROW(
            dispatcher::auth::audit::RuntimeAuthAuditAdapter::validate_request_context(
                context
            ),
            dispatcher::auth::audit::AuthAuditError
        );
    }

    void expect_rule_validation_throws(
        const dispatcher::auth::audit::RuntimeAuthAuditOperationRule& rule
    )
    {
        EXPECT_THROW(
            dispatcher::auth::audit::RuntimeAuthAuditAdapter::validate_operation_rule(
                rule
            ),
            dispatcher::auth::audit::AuthAuditError
        );
    }

    void expect_map_request_throws(
        const dispatcher::auth::audit::RuntimeAuthAuditAdapter& adapter,
        const dispatcher::auth::audit::RuntimeAuthAuditRequestContext& context
    )
    {
        EXPECT_THROW(
            {
                const auto mapping =
                    adapter.map_request(
                        context
                    );

                static_cast<void>(
                    mapping
                );
            },
            dispatcher::auth::audit::AuthAuditError
        );
    }
}

TEST(AuthRuntimeIntegrationTests, ConvertsRuntimeOperationsToStrings)
{
    EXPECT_EQ(
        dispatcher::auth::audit::RuntimeAuthAuditAdapter::operation_to_string(
            dispatcher::auth::audit::RuntimeAuthAuditOperation::runtime_read
        ),
        "runtime_read"
    );

    EXPECT_EQ(
        dispatcher::auth::audit::RuntimeAuthAuditAdapter::operation_to_string(
            dispatcher::auth::audit::RuntimeAuthAuditOperation::runtime_control
        ),
        "runtime_control"
    );

    EXPECT_EQ(
        dispatcher::auth::audit::RuntimeAuthAuditAdapter::operation_to_string(
            dispatcher::auth::audit::RuntimeAuthAuditOperation::alarm_acknowledge
        ),
        "alarm_acknowledge"
    );

    EXPECT_EQ(
        dispatcher::auth::audit::RuntimeAuthAuditAdapter::operation_to_string(
            dispatcher::auth::audit::RuntimeAuthAuditOperation::notification_send
        ),
        "notification_send"
    );
}

TEST(AuthRuntimeIntegrationTests, UnknownRuntimeOperationConvertsToUnknown)
{
    EXPECT_EQ(
        dispatcher::auth::audit::RuntimeAuthAuditAdapter::operation_to_string(
            static_cast<dispatcher::auth::audit::RuntimeAuthAuditOperation>(
                999
                )
        ),
        "unknown"
    );
}

TEST(AuthRuntimeIntegrationTests, ValidatesRuntimeRequestContext)
{
    const auto context =
        make_runtime_context();

    EXPECT_NO_THROW(
        dispatcher::auth::audit::RuntimeAuthAuditAdapter::validate_request_context(
            context
        )
    );
}

TEST(AuthRuntimeIntegrationTests, RejectsEmptyOperationId)
{
    auto context =
        make_runtime_context();

    context.operation_id = "";

    expect_request_validation_throws(
        context
    );
}

TEST(AuthRuntimeIntegrationTests, RejectsEmptySource)
{
    auto context =
        make_runtime_context();

    context.source = "";

    expect_request_validation_throws(
        context
    );
}

TEST(AuthRuntimeIntegrationTests, RejectsInvalidOperation)
{
    auto context =
        make_runtime_context();

    context.operation =
        static_cast<dispatcher::auth::audit::RuntimeAuthAuditOperation>(
            999
            );

    expect_request_validation_throws(
        context
    );
}

TEST(AuthRuntimeIntegrationTests, RejectsInvalidActorType)
{
    auto context =
        make_runtime_context();

    context.actor_type =
        static_cast<dispatcher::auth::audit::AuthAuditActorType>(
            999
            );

    expect_request_validation_throws(
        context
    );
}

TEST(AuthRuntimeIntegrationTests, ValidatesRuntimeOperationRule)
{
    const auto rule =
        make_runtime_read_rule();

    EXPECT_NO_THROW(
        dispatcher::auth::audit::RuntimeAuthAuditAdapter::validate_operation_rule(
            rule
        )
    );
}

TEST(AuthRuntimeIntegrationTests, RejectsEmptyRuleId)
{
    auto rule =
        make_runtime_read_rule();

    rule.rule_id = "";

    expect_rule_validation_throws(
        rule
    );
}

TEST(AuthRuntimeIntegrationTests, RejectsInvalidRuleOperation)
{
    auto rule =
        make_runtime_read_rule();

    rule.operation =
        static_cast<dispatcher::auth::audit::RuntimeAuthAuditOperation>(
            999
            );

    expect_rule_validation_throws(
        rule
    );
}

TEST(AuthRuntimeIntegrationTests, RejectsUnknownRuleAction)
{
    auto rule =
        make_runtime_read_rule();

    rule.action =
        dispatcher::auth::audit::AuthAuditAction::unknown;

    expect_rule_validation_throws(
        rule
    );
}

TEST(AuthRuntimeIntegrationTests, RejectsInvalidRulePermission)
{
    auto rule =
        make_runtime_read_rule();

    rule.required_permission =
        static_cast<dispatcher::auth::audit::AuthorizationPermission>(
            999
            );

    expect_rule_validation_throws(
        rule
    );
}

TEST(AuthRuntimeIntegrationTests, RejectsEmptyRuleResource)
{
    auto rule =
        make_runtime_read_rule();

    rule.resource.resource_id = "";

    expect_rule_validation_throws(
        rule
    );
}

TEST(AuthRuntimeIntegrationTests, ConstructorRejectsEmptyRules)
{
    expect_adapter_construction_throws(
        {}
    );
}

TEST(AuthRuntimeIntegrationTests, ConstructorValidatesRules)
{
    auto rule =
        make_runtime_read_rule();

    rule.rule_id = "";

    expect_adapter_construction_throws(
        {
            rule
        }
    );
}

TEST(AuthRuntimeIntegrationTests, MapsRuntimeContextToOperationContextAndAuthorizationRequest)
{
    dispatcher::auth::audit::RuntimeAuthAuditAdapter adapter{
        {
            make_runtime_read_rule()
        }
    };

    const auto mapping =
        adapter.map_request(
            make_runtime_context()
        );

    EXPECT_EQ(
        mapping.operation_rule.rule_id,
        "runtime-read"
    );

    EXPECT_EQ(
        mapping.operation_context.operation_id,
        "runtime-operation-1"
    );

    EXPECT_EQ(
        mapping.operation_context.correlation_id,
        "correlation-1"
    );

    EXPECT_EQ(
        mapping.operation_context.source,
        "runtime"
    );

    EXPECT_EQ(
        mapping.operation_context.actor.actor_id,
        "operator-1"
    );

    EXPECT_EQ(
        mapping.operation_context.attributes.at(
            "runtime.operation"
        ),
        "runtime_read"
    );

    EXPECT_EQ(
        mapping.authorization_request.request_id,
        "runtime-operation-1:authorization"
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
        mapping.authorization_request.resource.resource_id,
        "dispatcher-runtime"
    );

    EXPECT_EQ(
        mapping.authorization_request.attributes.at(
            "runtime.operation_rule_id"
        ),
        "runtime-read"
    );
}

TEST(AuthRuntimeIntegrationTests, RequestResourceOverridesRuleResource)
{
    dispatcher::auth::audit::RuntimeAuthAuditAdapter adapter{
        {
            make_alarm_ack_rule()
        }
    };

    auto context =
        make_runtime_context(
            dispatcher::auth::audit::RuntimeAuthAuditOperation::alarm_acknowledge,
            dispatcher::auth::audit::AuthorizationPermission::alarm_acknowledge
        );

    context.resource.resource_type = "alarm";
    context.resource.resource_id = "alarm-1";
    context.resource.display_name = "Pump pressure high";

    const auto mapping =
        adapter.map_request(
            context
        );

    EXPECT_EQ(
        mapping.authorization_request.resource.resource_id,
        "alarm-1"
    );

    EXPECT_EQ(
        mapping.authorization_request.resource.display_name,
        "Pump pressure high"
    );
}

TEST(AuthRuntimeIntegrationTests, RejectsNoMatchingRule)
{
    dispatcher::auth::audit::RuntimeAuthAuditAdapter adapter{
        {
            make_runtime_read_rule()
        }
    };

    expect_map_request_throws(
        adapter,
        make_runtime_context(
            dispatcher::auth::audit::RuntimeAuthAuditOperation::runtime_control,
            dispatcher::auth::audit::AuthorizationPermission::runtime_control
        )
    );
}

TEST(AuthRuntimeIntegrationTests, DisabledRuleIsIgnored)
{
    auto rule =
        make_runtime_read_rule();

    rule.enabled = false;

    dispatcher::auth::audit::RuntimeAuthAuditAdapter adapter{
        {
            rule
        }
    };

    expect_map_request_throws(
        adapter,
        make_runtime_context()
    );
}

TEST(AuthRuntimeIntegrationTests, AuthenticatedRuntimeOperationRejectsAnonymousActor)
{
    dispatcher::auth::audit::RuntimeAuthAuditAdapter adapter{
        {
            make_runtime_read_rule()
        }
    };

    auto context =
        make_runtime_context();

    context.actor_id = "";
    context.actor_display_name = "";
    context.actor_type =
        dispatcher::auth::audit::AuthAuditActorType::anonymous;

    expect_map_request_throws(
        adapter,
        context
    );
}

TEST(AuthRuntimeIntegrationTests, PublicRuntimeOperationCanMapAnonymousActor)
{
    auto rule =
        make_runtime_read_rule();

    rule.require_authenticated = false;

    dispatcher::auth::audit::RuntimeAuthAuditAdapter adapter{
        {
            rule
        }
    };

    auto context =
        make_runtime_context();

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

TEST(AuthRuntimeIntegrationTests, AuthorizeAndAuditRuntimeReadAllowed)
{
    dispatcher::auth::audit::RuntimeAuthAuditAdapter adapter{
        {
            make_runtime_read_rule()
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
            make_runtime_context()
        );

    EXPECT_TRUE(
        result.allowed()
    );

    EXPECT_TRUE(
        result.audit_success()
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
            "runtime.operation"
        ),
        "runtime_read"
    );

    EXPECT_EQ(
        sink.recorded_events()[0].attributes.at(
            "required_permission"
        ),
        "runtime_read"
    );
}

TEST(AuthRuntimeIntegrationTests, AuthorizeAndAuditRuntimeControlDenied)
{
    dispatcher::auth::audit::RuntimeAuthAuditAdapter adapter{
        {
            make_runtime_control_rule()
        }
    };

    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator;

    auto context =
        make_runtime_context(
            dispatcher::auth::audit::RuntimeAuthAuditOperation::runtime_control,
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
        "runtime_control"
    );
}

TEST(AuthRuntimeIntegrationTests, AuthorizeAndAuditAlarmAcknowledgeAllowed)
{
    dispatcher::auth::audit::RuntimeAuthAuditAdapter adapter{
        {
            make_alarm_ack_rule()
        }
    };

    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator{
        make_allow_operator_policy(
            dispatcher::auth::audit::AuthorizationPermission::alarm_acknowledge
        )
    };

    auto context =
        make_runtime_context(
            dispatcher::auth::audit::RuntimeAuthAuditOperation::alarm_acknowledge,
            dispatcher::auth::audit::AuthorizationPermission::alarm_acknowledge
        );

    context.resource.resource_type = "alarm";
    context.resource.resource_id = "alarm-1";
    context.resource.display_name = "Pump pressure high";

    const auto result =
        adapter.authorize_and_audit(
            evaluator,
            logger,
            context
        );

    EXPECT_TRUE(
        result.allowed()
    );

    ASSERT_EQ(
        sink.recorded_events().size(),
        1U
    );

    EXPECT_EQ(
        sink.recorded_events()[0].resource.resource_id,
        "alarm-1"
    );

    EXPECT_EQ(
        sink.recorded_events()[0].attributes.at(
            "runtime.operation"
        ),
        "alarm_acknowledge"
    );

    EXPECT_EQ(
        sink.recorded_events()[0].attributes.at(
            "required_permission"
        ),
        "alarm_acknowledge"
    );
}

TEST(AuthRuntimeIntegrationTests, AuthorizeAndAuditNotificationSendAllowed)
{
    dispatcher::auth::audit::RuntimeAuthAuditAdapter adapter{
        {
            make_notification_send_rule()
        }
    };

    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator{
        make_allow_operator_policy(
            dispatcher::auth::audit::AuthorizationPermission::notification_send
        )
    };

    auto context =
        make_runtime_context(
            dispatcher::auth::audit::RuntimeAuthAuditOperation::notification_send,
            dispatcher::auth::audit::AuthorizationPermission::notification_send
        );

    context.attributes.insert_or_assign(
        "notification_id",
        "notification-1"
    );

    const auto result =
        adapter.authorize_and_audit(
            evaluator,
            logger,
            context
        );

    EXPECT_TRUE(
        result.allowed()
    );

    ASSERT_EQ(
        sink.recorded_events().size(),
        1U
    );

    EXPECT_EQ(
        sink.recorded_events()[0].attributes.at(
            "runtime.operation"
        ),
        "notification_send"
    );

    EXPECT_EQ(
        sink.recorded_events()[0].attributes.at(
            "notification_id"
        ),
        "notification-1"
    );
}

TEST(AuthRuntimeIntegrationTests, AuthorizeAndAuditCanSkipAllowedAudit)
{
    dispatcher::auth::audit::RuntimeAuthAuditAdapter adapter{
        {
            make_runtime_read_rule()
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
            make_runtime_context(),
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

TEST(AuthRuntimeIntegrationTests, AuthorizeAndAuditPreservesAuditSinkFailureAsResult)
{
    dispatcher::auth::audit::RuntimeAuthAuditAdapter adapter{
        {
            make_runtime_read_rule()
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
            make_runtime_context()
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

TEST(AuthRuntimeIntegrationTests, SmokeMultipleRuntimeOperationsWithOneAdapter)
{
    dispatcher::auth::audit::RuntimeAuthAuditAdapter adapter{
        {
            make_runtime_read_rule(),
            make_runtime_control_rule(),
            make_alarm_ack_rule(),
            make_notification_send_rule()
        }
    };

    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    dispatcher::auth::audit::AuthorizationPolicy policy;

    policy.allow_direct_subject_permissions = true;

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator{
        policy
    };

    auto runtime_read =
        make_runtime_context(
            dispatcher::auth::audit::RuntimeAuthAuditOperation::runtime_read,
            dispatcher::auth::audit::AuthorizationPermission::runtime_read
        );

    auto alarm_ack =
        make_runtime_context(
            dispatcher::auth::audit::RuntimeAuthAuditOperation::alarm_acknowledge,
            dispatcher::auth::audit::AuthorizationPermission::alarm_acknowledge
        );

    alarm_ack.operation_id = "runtime-operation-2";
    alarm_ack.resource.resource_type = "alarm";
    alarm_ack.resource.resource_id = "alarm-1";
    alarm_ack.resource.display_name = "Pump pressure high";
    alarm_ack.permissions.clear();
    alarm_ack.permissions.push_back(
        dispatcher::auth::audit::AuthorizationPermission::alarm_acknowledge
    );

    const auto read_result =
        adapter.authorize_and_audit(
            evaluator,
            logger,
            runtime_read
        );

    const auto ack_result =
        adapter.authorize_and_audit(
            evaluator,
            logger,
            alarm_ack
        );

    EXPECT_TRUE(
        read_result.allowed()
    );

    EXPECT_TRUE(
        ack_result.allowed()
    );

    ASSERT_EQ(
        sink.recorded_events().size(),
        2U
    );

    EXPECT_EQ(
        sink.recorded_events()[0].attributes.at(
            "runtime.operation"
        ),
        "runtime_read"
    );

    EXPECT_EQ(
        sink.recorded_events()[1].attributes.at(
            "runtime.operation"
        ),
        "alarm_acknowledge"
    );

    EXPECT_EQ(
        sink.recorded_events()[1].resource.resource_id,
        "alarm-1"
    );
}