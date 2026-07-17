#include <dispatcher/auth/audit/auth_audit.hpp>

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace
{
    dispatcher::auth::audit::AuthenticatedOperationContext make_context()
    {
        auto context =
            dispatcher::auth::audit::AuthenticatedOperationContextBuilder::operator_context(
                "operation-1",
                "http-api",
                "operator-1",
                "Operator One",
                "correlation-1"
            );

        context.client_address =
            "127.0.0.1";

        context.user_agent =
            "dispatcher-test";

        context.attributes.emplace(
            "endpoint",
            "/api/v1/runtime"
        );

        return context;
    }

    dispatcher::auth::audit::AuthorizationRequest make_request(
        dispatcher::auth::audit::AuthorizationPermission permission =
        dispatcher::auth::audit::AuthorizationPermission::runtime_read
    )
    {
        const auto context =
            make_context();

        dispatcher::auth::audit::AuthorizationRequest request;

        request.request_id = "authorization-request-1";
        request.correlation_id = "correlation-1";
        request.source = "http-api";

        request.subject =
            dispatcher::auth::audit::AuthenticatedOperationContextBuilder::to_authorization_subject(
                context,
                {
                    "operators"
                },
                {
                    dispatcher::auth::audit::AuthorizationPermission::runtime_read
                }
            );

        request.action =
            dispatcher::auth::audit::AuthAuditAction::runtime_read;

        request.resource.resource_type = "runtime";
        request.resource.resource_id = "dispatcher-runtime";
        request.resource.display_name = "Dispatcher runtime";

        request.required_permission =
            permission;

        request.attributes.emplace(
            "request.endpoint",
            "/api/v1/runtime"
        );

        return request;
    }

    dispatcher::auth::audit::AuthorizationDecision make_allowed_decision()
    {
        const auto request =
            make_request();

        return dispatcher::auth::audit::AuthorizationDecision::allowed(
            request,
            "allowed_by_subject_permission",
            "Authorization allowed by direct subject permission."
        );
    }

    dispatcher::auth::audit::AuthorizationDecision make_denied_decision()
    {
        const auto request =
            make_request(
                dispatcher::auth::audit::AuthorizationPermission::runtime_control
            );

        return dispatcher::auth::audit::AuthorizationDecision::denied(
            request,
            "denied_by_default_policy",
            "Authorization denied by default policy."
        );
    }

    void expect_context_validation_throws(
        const dispatcher::auth::audit::AuthenticatedOperationContext& context
    )
    {
        EXPECT_THROW(
            dispatcher::auth::audit::AuthenticatedOperationContextBuilder::validate_context(
                context
            ),
            dispatcher::auth::audit::AuthAuditError
        );
    }

    void expect_decision_validation_throws(
        const dispatcher::auth::audit::AuthorizationDecision& decision
    )
    {
        EXPECT_THROW(
            dispatcher::auth::audit::AuthorizationAuditRecorder::validate_decision(
                decision
            ),
            dispatcher::auth::audit::AuthAuditError
        );
    }

    void expect_build_event_throws(
        const dispatcher::auth::audit::AuthenticatedOperationContext& context,
        const dispatcher::auth::audit::AuthorizationRequest& request,
        const dispatcher::auth::audit::AuthorizationDecision& decision
    )
    {
        EXPECT_THROW(
            {
                const auto event =
                    dispatcher::auth::audit::AuthorizationAuditRecorder::build_event(
                        context,
                        request,
                        decision
                    );

                static_cast<void>(
                    event
                );
            },
            dispatcher::auth::audit::AuthAuditError
        );
    }

    dispatcher::auth::audit::AuthorizationPolicy make_runtime_control_allow_policy()
    {
        dispatcher::auth::audit::AuthorizationPolicy policy;

        policy.allow_direct_subject_permissions = false;

        dispatcher::auth::audit::AuthorizationPolicyRule rule;

        rule.rule_id = "allow-runtime-control";
        rule.effect =
            dispatcher::auth::audit::AuthorizationDecisionEffect::allow;
        rule.permission =
            dispatcher::auth::audit::AuthorizationPermission::runtime_control;
        rule.role = "operators";
        rule.resource_type = "runtime";
        rule.resource_id = "dispatcher-runtime";
        rule.enabled = true;

        policy.rules.push_back(
            rule
        );

        return policy;
    }
}

TEST(AuthAuthorizationAuditTests, BuildsSystemOperationContext)
{
    const auto context =
        dispatcher::auth::audit::AuthenticatedOperationContextBuilder::system_context(
            "operation-system-1",
            "scheduler",
            "correlation-system-1"
        );

    EXPECT_EQ(
        context.operation_id,
        "operation-system-1"
    );

    EXPECT_EQ(
        context.source,
        "scheduler"
    );

    EXPECT_EQ(
        context.correlation_id,
        "correlation-system-1"
    );

    EXPECT_EQ(
        context.actor.actor_id,
        "system"
    );

    EXPECT_EQ(
        context.actor.actor_type,
        dispatcher::auth::audit::AuthAuditActorType::system
    );
}

TEST(AuthAuthorizationAuditTests, BuildsOperatorOperationContext)
{
    const auto context =
        make_context();

    EXPECT_EQ(
        context.operation_id,
        "operation-1"
    );

    EXPECT_EQ(
        context.source,
        "http-api"
    );

    EXPECT_EQ(
        context.actor.actor_id,
        "operator-1"
    );

    EXPECT_EQ(
        context.actor.display_name,
        "Operator One"
    );

    EXPECT_EQ(
        context.actor.actor_type,
        dispatcher::auth::audit::AuthAuditActorType::operator_user
    );
}

TEST(AuthAuthorizationAuditTests, ValidatesOperationContext)
{
    const auto context =
        make_context();

    EXPECT_NO_THROW(
        dispatcher::auth::audit::AuthenticatedOperationContextBuilder::validate_context(
            context
        )
    );
}

TEST(AuthAuthorizationAuditTests, RejectsEmptyOperationId)
{
    auto context =
        make_context();

    context.operation_id = "";

    expect_context_validation_throws(
        context
    );
}

TEST(AuthAuthorizationAuditTests, RejectsEmptySource)
{
    auto context =
        make_context();

    context.source = "";

    expect_context_validation_throws(
        context
    );
}

TEST(AuthAuthorizationAuditTests, RejectsEmptyActorId)
{
    auto context =
        make_context();

    context.actor.actor_id = "";

    expect_context_validation_throws(
        context
    );
}

TEST(AuthAuthorizationAuditTests, RejectsInvalidActorType)
{
    auto context =
        make_context();

    context.actor.actor_type =
        static_cast<dispatcher::auth::audit::AuthAuditActorType>(
            999
            );

    expect_context_validation_throws(
        context
    );
}

TEST(AuthAuthorizationAuditTests, ConvertsContextToAuthorizationSubject)
{
    const auto context =
        make_context();

    const auto subject =
        dispatcher::auth::audit::AuthenticatedOperationContextBuilder::to_authorization_subject(
            context,
            {
                "operators",
                "engineers"
            },
            {
                dispatcher::auth::audit::AuthorizationPermission::runtime_read,
                dispatcher::auth::audit::AuthorizationPermission::alarm_acknowledge
            }
        );

    EXPECT_EQ(
        subject.subject_id,
        "operator-1"
    );

    EXPECT_EQ(
        subject.display_name,
        "Operator One"
    );

    ASSERT_EQ(
        subject.roles.size(),
        2U
    );

    ASSERT_EQ(
        subject.permissions.size(),
        2U
    );

    EXPECT_EQ(
        subject.roles[0],
        "operators"
    );

    EXPECT_EQ(
        subject.permissions[0],
        dispatcher::auth::audit::AuthorizationPermission::runtime_read
    );
}

TEST(AuthAuthorizationAuditTests, ValidatesAuthorizationDecision)
{
    const auto decision =
        make_allowed_decision();

    EXPECT_NO_THROW(
        dispatcher::auth::audit::AuthorizationAuditRecorder::validate_decision(
            decision
        )
    );
}

TEST(AuthAuthorizationAuditTests, RejectsDecisionWithoutRequestId)
{
    auto decision =
        make_allowed_decision();

    decision.request_id = "";

    expect_decision_validation_throws(
        decision
    );
}

TEST(AuthAuthorizationAuditTests, RejectsDecisionWithoutSubjectId)
{
    auto decision =
        make_allowed_decision();

    decision.subject_id = "";

    expect_decision_validation_throws(
        decision
    );
}

TEST(AuthAuthorizationAuditTests, RejectsDecisionWithUnknownAction)
{
    auto decision =
        make_allowed_decision();

    decision.action =
        dispatcher::auth::audit::AuthAuditAction::unknown;

    expect_decision_validation_throws(
        decision
    );
}

TEST(AuthAuthorizationAuditTests, RejectsDecisionWithInvalidPermission)
{
    auto decision =
        make_allowed_decision();

    decision.required_permission =
        static_cast<dispatcher::auth::audit::AuthorizationPermission>(
            999
            );

    expect_decision_validation_throws(
        decision
    );
}

TEST(AuthAuthorizationAuditTests, RejectsDecisionWithInvalidEffect)
{
    auto decision =
        make_allowed_decision();

    decision.effect =
        static_cast<dispatcher::auth::audit::AuthorizationDecisionEffect>(
            999
            );

    expect_decision_validation_throws(
        decision
    );
}

TEST(AuthAuthorizationAuditTests, RejectsDecisionWithoutResource)
{
    auto decision =
        make_allowed_decision();

    decision.resource.resource_id = "";

    expect_decision_validation_throws(
        decision
    );
}

TEST(AuthAuthorizationAuditTests, RejectsDecisionWithoutReason)
{
    auto decision =
        make_allowed_decision();

    decision.reason = "";

    expect_decision_validation_throws(
        decision
    );
}

TEST(AuthAuthorizationAuditTests, BuildsAuditEventForAllowedDecision)
{
    const auto context =
        make_context();

    const auto request =
        make_request();

    const auto decision =
        dispatcher::auth::audit::AuthorizationDecision::allowed(
            request,
            "allowed_by_subject_permission",
            "Authorization allowed by direct subject permission."
        );

    const auto event =
        dispatcher::auth::audit::AuthorizationAuditRecorder::build_event(
            context,
            request,
            decision
        );

    EXPECT_EQ(
        event.event_id,
        "authorization:authorization-request-1"
    );

    EXPECT_EQ(
        event.correlation_id,
        "correlation-1"
    );

    EXPECT_EQ(
        event.source,
        "http-api"
    );

    EXPECT_EQ(
        event.actor.actor_id,
        "operator-1"
    );

    EXPECT_EQ(
        event.action,
        dispatcher::auth::audit::AuthAuditAction::authorization_check
    );

    EXPECT_EQ(
        event.outcome,
        dispatcher::auth::audit::AuthAuditOutcome::success
    );

    EXPECT_EQ(
        event.severity,
        dispatcher::auth::audit::AuthAuditSeverity::info
    );

    EXPECT_EQ(
        event.resource.resource_type,
        "runtime"
    );

    EXPECT_EQ(
        event.reason,
        "allowed_by_subject_permission"
    );

    EXPECT_EQ(
        event.attributes.at(
            "authorization_action"
        ),
        "runtime_read"
    );

    EXPECT_EQ(
        event.attributes.at(
            "required_permission"
        ),
        "runtime_read"
    );

    EXPECT_EQ(
        event.attributes.at(
            "decision_effect"
        ),
        "allow"
    );

    EXPECT_EQ(
        event.attributes.at(
            "client_address"
        ),
        "127.0.0.1"
    );

    EXPECT_EQ(
        event.attributes.at(
            "context.endpoint"
        ),
        "/api/v1/runtime"
    );

    EXPECT_NO_THROW(
        dispatcher::auth::audit::AuthAuditValidator::validate_event(
            event
        )
    );
}

TEST(AuthAuthorizationAuditTests, BuildsAuditEventForDeniedDecision)
{
    const auto context =
        make_context();

    const auto request =
        make_request(
            dispatcher::auth::audit::AuthorizationPermission::runtime_control
        );

    const auto decision =
        dispatcher::auth::audit::AuthorizationDecision::denied(
            request,
            "denied_by_default_policy",
            "Authorization denied by default policy."
        );

    const auto event =
        dispatcher::auth::audit::AuthorizationAuditRecorder::build_event(
            context,
            request,
            decision
        );

    EXPECT_EQ(
        event.outcome,
        dispatcher::auth::audit::AuthAuditOutcome::denied
    );

    EXPECT_EQ(
        event.severity,
        dispatcher::auth::audit::AuthAuditSeverity::warning
    );

    EXPECT_EQ(
        event.reason,
        "denied_by_default_policy"
    );

    EXPECT_EQ(
        event.attributes.at(
            "decision_effect"
        ),
        "deny"
    );

    EXPECT_EQ(
        event.attributes.at(
            "required_permission"
        ),
        "runtime_control"
    );

    EXPECT_NO_THROW(
        dispatcher::auth::audit::AuthAuditValidator::validate_event(
            event
        )
    );
}

TEST(AuthAuthorizationAuditTests, BuildEventUsesContextCorrelationWhenRequestCorrelationIsEmpty)
{
    const auto context =
        make_context();

    auto request =
        make_request();

    request.correlation_id = "";

    const auto decision =
        dispatcher::auth::audit::AuthorizationDecision::allowed(
            request,
            "allowed_by_subject_permission"
        );

    const auto event =
        dispatcher::auth::audit::AuthorizationAuditRecorder::build_event(
            context,
            request,
            decision
        );

    EXPECT_EQ(
        event.correlation_id,
        "correlation-1"
    );
}

TEST(AuthAuthorizationAuditTests, BuildEventRejectsMismatchedDecisionRequestId)
{
    const auto context =
        make_context();

    const auto request =
        make_request();

    auto decision =
        dispatcher::auth::audit::AuthorizationDecision::allowed(
            request,
            "allowed_by_subject_permission"
        );

    decision.request_id =
        "other-request";

    expect_build_event_throws(
        context,
        request,
        decision
    );
}

TEST(AuthAuthorizationAuditTests, BuildEventRejectsMismatchedDecisionSubjectId)
{
    const auto context =
        make_context();

    const auto request =
        make_request();

    auto decision =
        dispatcher::auth::audit::AuthorizationDecision::allowed(
            request,
            "allowed_by_subject_permission"
        );

    decision.subject_id =
        "other-subject";

    expect_build_event_throws(
        context,
        request,
        decision
    );
}

TEST(AuthAuthorizationAuditTests, RecordsAllowedDecision)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    const auto context =
        make_context();

    const auto request =
        make_request();

    const auto decision =
        dispatcher::auth::audit::AuthorizationDecision::allowed(
            request,
            "allowed_by_subject_permission"
        );

    const auto result =
        dispatcher::auth::audit::AuthorizationAuditRecorder::record_decision(
            logger,
            context,
            request,
            decision
        );

    EXPECT_TRUE(
        result.success()
    );

    ASSERT_EQ(
        sink.recorded_events().size(),
        1U
    );

    EXPECT_EQ(
        sink.recorded_events()[0].event_id,
        "authorization:authorization-request-1"
    );

    EXPECT_EQ(
        sink.recorded_events()[0].outcome,
        dispatcher::auth::audit::AuthAuditOutcome::success
    );
}

TEST(AuthAuthorizationAuditTests, RecordsDeniedDecision)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    const auto context =
        make_context();

    const auto request =
        make_request(
            dispatcher::auth::audit::AuthorizationPermission::runtime_control
        );

    const auto decision =
        dispatcher::auth::audit::AuthorizationDecision::denied(
            request,
            "denied_by_default_policy"
        );

    const auto result =
        dispatcher::auth::audit::AuthorizationAuditRecorder::record_decision(
            logger,
            context,
            request,
            decision
        );

    EXPECT_TRUE(
        result.success()
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
        sink.recorded_events()[0].reason,
        "denied_by_default_policy"
    );
}

TEST(AuthAuthorizationAuditTests, CanSkipAllowedDecisionRecording)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    dispatcher::auth::audit::AuthorizationAuditOptions options;

    options.record_allowed = false;
    options.record_denied = true;

    const auto result =
        dispatcher::auth::audit::AuthorizationAuditRecorder::record_decision(
            logger,
            make_context(),
            make_request(),
            make_allowed_decision(),
            options
        );

    EXPECT_EQ(
        result.status,
        dispatcher::auth::audit::AuthAuditRecordStatus::skipped
    );

    EXPECT_TRUE(
        sink.recorded_events().empty()
    );

    EXPECT_EQ(
        sink.record_attempt_count(),
        0
    );
}

TEST(AuthAuthorizationAuditTests, CanSkipDeniedDecisionRecording)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    dispatcher::auth::audit::AuthorizationAuditOptions options;

    options.record_allowed = true;
    options.record_denied = false;

    const auto request =
        make_request(
            dispatcher::auth::audit::AuthorizationPermission::runtime_control
        );

    const auto decision =
        dispatcher::auth::audit::AuthorizationDecision::denied(
            request,
            "denied_by_default_policy"
        );

    const auto result =
        dispatcher::auth::audit::AuthorizationAuditRecorder::record_decision(
            logger,
            make_context(),
            request,
            decision,
            options
        );

    EXPECT_EQ(
        result.status,
        dispatcher::auth::audit::AuthAuditRecordStatus::skipped
    );

    EXPECT_TRUE(
        sink.recorded_events().empty()
    );

    EXPECT_EQ(
        sink.record_attempt_count(),
        0
    );
}

TEST(AuthAuthorizationAuditTests, EvaluateAndRecordAllowedDecision)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator;

    const auto record =
        dispatcher::auth::audit::AuthorizationAuditRecorder::evaluate_and_record(
            evaluator,
            logger,
            make_context(),
            make_request()
        );

    EXPECT_TRUE(
        record.allowed()
    );

    EXPECT_TRUE(
        record.audit_success()
    );

    EXPECT_TRUE(
        record.audit_attempted
    );

    EXPECT_EQ(
        record.decision.reason,
        "allowed_by_subject_permission"
    );

    ASSERT_EQ(
        sink.recorded_events().size(),
        1U
    );

    EXPECT_EQ(
        sink.recorded_events()[0].attributes.at(
            "decision_effect"
        ),
        "allow"
    );
}

TEST(AuthAuthorizationAuditTests, EvaluateAndRecordDeniedDecision)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator;

    const auto record =
        dispatcher::auth::audit::AuthorizationAuditRecorder::evaluate_and_record(
            evaluator,
            logger,
            make_context(),
            make_request(
                dispatcher::auth::audit::AuthorizationPermission::runtime_control
            )
        );

    EXPECT_TRUE(
        record.denied()
    );

    EXPECT_TRUE(
        record.audit_success()
    );

    EXPECT_TRUE(
        record.audit_attempted
    );

    EXPECT_EQ(
        record.decision.reason,
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
}

TEST(AuthAuthorizationAuditTests, EvaluateAndRecordAllowedByPolicyRule)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator{
        make_runtime_control_allow_policy()
    };

    const auto record =
        dispatcher::auth::audit::AuthorizationAuditRecorder::evaluate_and_record(
            evaluator,
            logger,
            make_context(),
            make_request(
                dispatcher::auth::audit::AuthorizationPermission::runtime_control
            )
        );

    EXPECT_TRUE(
        record.allowed()
    );

    EXPECT_TRUE(
        record.audit_success()
    );

    EXPECT_EQ(
        record.decision.reason,
        "allowed_by_policy_rule"
    );

    ASSERT_EQ(
        sink.recorded_events().size(),
        1U
    );

    EXPECT_EQ(
        sink.recorded_events()[0].attributes.at(
            "required_permission"
        ),
        "runtime_control"
    );
}

TEST(AuthAuthorizationAuditTests, EvaluateAndRecordCanSkipAllowedAudit)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator;

    dispatcher::auth::audit::AuthorizationAuditOptions options;

    options.record_allowed = false;

    const auto record =
        dispatcher::auth::audit::AuthorizationAuditRecorder::evaluate_and_record(
            evaluator,
            logger,
            make_context(),
            make_request(),
            options
        );

    EXPECT_TRUE(
        record.allowed()
    );

    EXPECT_FALSE(
        record.audit_attempted
    );

    EXPECT_EQ(
        record.audit_result.status,
        dispatcher::auth::audit::AuthAuditRecordStatus::skipped
    );

    EXPECT_TRUE(
        sink.recorded_events().empty()
    );
}

TEST(AuthAuthorizationAuditTests, EvaluateAndRecordPropagatesAuditSinkFailureAsResult)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    sink.set_failure(
        "audit storage unavailable"
    );

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator;

    const auto record =
        dispatcher::auth::audit::AuthorizationAuditRecorder::evaluate_and_record(
            evaluator,
            logger,
            make_context(),
            make_request()
        );

    EXPECT_TRUE(
        record.allowed()
    );

    EXPECT_TRUE(
        record.audit_attempted
    );

    EXPECT_TRUE(
        record.audit_result.failure()
    );

    EXPECT_EQ(
        record.audit_result.error_message,
        "audit storage unavailable"
    );

    EXPECT_TRUE(
        sink.recorded_events().empty()
    );
}