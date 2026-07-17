#include <dispatcher/auth/audit/auth_audit.hpp>

#include <gtest/gtest.h>

#include <string>
#include <utility>

namespace
{
    dispatcher::auth::audit::AuthorizationSubject make_subject()
    {
        dispatcher::auth::audit::AuthorizationSubject subject;

        subject.subject_id = "operator-1";
        subject.display_name = "Operator One";
        subject.subject_type =
            dispatcher::auth::audit::AuthAuditActorType::operator_user;

        subject.roles.push_back(
            "operators"
        );

        subject.permissions.push_back(
            dispatcher::auth::audit::AuthorizationPermission::runtime_read
        );

        return subject;
    }

    dispatcher::auth::audit::AuthorizationRequest make_request(
        dispatcher::auth::audit::AuthorizationPermission permission =
        dispatcher::auth::audit::AuthorizationPermission::runtime_read
    )
    {
        dispatcher::auth::audit::AuthorizationRequest request;

        request.request_id = "authorization-request-1";
        request.correlation_id = "operation-1";
        request.source = "http-api";

        request.subject =
            make_subject();

        request.action =
            dispatcher::auth::audit::AuthAuditAction::runtime_read;

        request.resource.resource_type = "runtime";
        request.resource.resource_id = "dispatcher-runtime";
        request.resource.display_name = "Dispatcher runtime";

        request.required_permission =
            permission;

        request.attributes.emplace(
            "endpoint",
            "/api/v1/runtime"
        );

        return request;
    }

    dispatcher::auth::audit::AuthorizationPolicyRule make_rule(
        std::string rule_id,
        dispatcher::auth::audit::AuthorizationDecisionEffect effect,
        dispatcher::auth::audit::AuthorizationPermission permission
    )
    {
        dispatcher::auth::audit::AuthorizationPolicyRule rule;

        rule.rule_id =
            std::move(
                rule_id
            );

        rule.effect =
            effect;

        rule.permission =
            permission;

        rule.role = "operators";
        rule.resource_type = "runtime";
        rule.resource_id = "dispatcher-runtime";
        rule.enabled = true;

        return rule;
    }

    void expect_policy_construction_throws(
        const dispatcher::auth::audit::AuthorizationPolicy& policy
    )
    {
        EXPECT_THROW(
            {
                dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator{
                    policy
                };

                static_cast<void>(
                    evaluator
                );
            },
            dispatcher::auth::audit::AuthAuditError
        );
    }

    void expect_request_validation_throws(
        const dispatcher::auth::audit::AuthorizationRequest& request
    )
    {
        EXPECT_THROW(
            dispatcher::auth::audit::AuthorizationPolicyEvaluator::validate_request(
                request
            ),
            dispatcher::auth::audit::AuthAuditError
        );
    }

    void expect_rule_validation_throws(
        const dispatcher::auth::audit::AuthorizationPolicyRule& rule
    )
    {
        EXPECT_THROW(
            dispatcher::auth::audit::AuthorizationPolicyEvaluator::validate_rule(
                rule
            ),
            dispatcher::auth::audit::AuthAuditError
        );
    }

    void expect_evaluate_throws(
        dispatcher::auth::audit::AuthorizationPolicyEvaluator& evaluator,
        const dispatcher::auth::audit::AuthorizationRequest& request
    )
    {
        EXPECT_THROW(
            {
                const auto decision =
                    evaluator.evaluate(
                        request
                    );

                static_cast<void>(
                    decision
                );
            },
            dispatcher::auth::audit::AuthAuditError
        );
    }
}

TEST(AuthAuthorizationPolicyTests, ValidatesDefaultPolicy)
{
    const dispatcher::auth::audit::AuthorizationPolicy policy;

    EXPECT_NO_THROW(
        dispatcher::auth::audit::AuthorizationPolicyEvaluator::validate_policy(
            policy
        )
    );

    EXPECT_EQ(
        policy.default_effect,
        dispatcher::auth::audit::AuthorizationDecisionEffect::deny
    );

    EXPECT_TRUE(
        policy.allow_direct_subject_permissions
    );
}

TEST(AuthAuthorizationPolicyTests, ValidatesGoodRequest)
{
    const auto request =
        make_request();

    EXPECT_NO_THROW(
        dispatcher::auth::audit::AuthorizationPolicyEvaluator::validate_request(
            request
        )
    );
}

TEST(AuthAuthorizationPolicyTests, RejectsEmptyRequestId)
{
    auto request =
        make_request();

    request.request_id = "";

    expect_request_validation_throws(
        request
    );
}

TEST(AuthAuthorizationPolicyTests, RejectsEmptySource)
{
    auto request =
        make_request();

    request.source = "";

    expect_request_validation_throws(
        request
    );
}

TEST(AuthAuthorizationPolicyTests, RejectsEmptySubjectId)
{
    auto request =
        make_request();

    request.subject.subject_id = "";

    expect_request_validation_throws(
        request
    );
}

TEST(AuthAuthorizationPolicyTests, RejectsInvalidSubjectType)
{
    auto request =
        make_request();

    request.subject.subject_type =
        static_cast<dispatcher::auth::audit::AuthAuditActorType>(
            999
            );

    expect_request_validation_throws(
        request
    );
}

TEST(AuthAuthorizationPolicyTests, RejectsUnknownAction)
{
    auto request =
        make_request();

    request.action =
        dispatcher::auth::audit::AuthAuditAction::unknown;

    expect_request_validation_throws(
        request
    );
}

TEST(AuthAuthorizationPolicyTests, RejectsInvalidAction)
{
    auto request =
        make_request();

    request.action =
        static_cast<dispatcher::auth::audit::AuthAuditAction>(
            999
            );

    expect_request_validation_throws(
        request
    );
}

TEST(AuthAuthorizationPolicyTests, RejectsEmptyResourceType)
{
    auto request =
        make_request();

    request.resource.resource_type = "";

    expect_request_validation_throws(
        request
    );
}

TEST(AuthAuthorizationPolicyTests, RejectsEmptyResourceId)
{
    auto request =
        make_request();

    request.resource.resource_id = "";

    expect_request_validation_throws(
        request
    );
}

TEST(AuthAuthorizationPolicyTests, RejectsInvalidRequiredPermission)
{
    auto request =
        make_request();

    request.required_permission =
        static_cast<dispatcher::auth::audit::AuthorizationPermission>(
            999
            );

    expect_request_validation_throws(
        request
    );
}

TEST(AuthAuthorizationPolicyTests, ValidatesGoodRule)
{
    const auto rule =
        make_rule(
            "allow-runtime-read",
            dispatcher::auth::audit::AuthorizationDecisionEffect::allow,
            dispatcher::auth::audit::AuthorizationPermission::runtime_read
        );

    EXPECT_NO_THROW(
        dispatcher::auth::audit::AuthorizationPolicyEvaluator::validate_rule(
            rule
        )
    );
}

TEST(AuthAuthorizationPolicyTests, RejectsEmptyRuleId)
{
    auto rule =
        make_rule(
            "allow-runtime-read",
            dispatcher::auth::audit::AuthorizationDecisionEffect::allow,
            dispatcher::auth::audit::AuthorizationPermission::runtime_read
        );

    rule.rule_id = "";

    expect_rule_validation_throws(
        rule
    );
}

TEST(AuthAuthorizationPolicyTests, RejectsInvalidRuleEffect)
{
    auto rule =
        make_rule(
            "allow-runtime-read",
            dispatcher::auth::audit::AuthorizationDecisionEffect::allow,
            dispatcher::auth::audit::AuthorizationPermission::runtime_read
        );

    rule.effect =
        static_cast<dispatcher::auth::audit::AuthorizationDecisionEffect>(
            999
            );

    expect_rule_validation_throws(
        rule
    );
}

TEST(AuthAuthorizationPolicyTests, RejectsInvalidRulePermission)
{
    auto rule =
        make_rule(
            "allow-runtime-read",
            dispatcher::auth::audit::AuthorizationDecisionEffect::allow,
            dispatcher::auth::audit::AuthorizationPermission::runtime_read
        );

    rule.permission =
        static_cast<dispatcher::auth::audit::AuthorizationPermission>(
            999
            );

    expect_rule_validation_throws(
        rule
    );
}

TEST(AuthAuthorizationPolicyTests, ConstructorValidatesPolicy)
{
    dispatcher::auth::audit::AuthorizationPolicy policy;

    policy.rules.push_back(
        make_rule(
            "",
            dispatcher::auth::audit::AuthorizationDecisionEffect::allow,
            dispatcher::auth::audit::AuthorizationPermission::runtime_read
        )
    );

    expect_policy_construction_throws(
        policy
    );
}

TEST(AuthAuthorizationPolicyTests, AllowsByDirectSubjectPermission)
{
    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator;

    const auto decision =
        evaluator.evaluate(
            make_request(
                dispatcher::auth::audit::AuthorizationPermission::runtime_read
            )
        );

    EXPECT_TRUE(
        decision.allowed()
    );

    EXPECT_FALSE(
        decision.denied()
    );

    EXPECT_EQ(
        decision.reason,
        "allowed_by_subject_permission"
    );

    EXPECT_EQ(
        decision.subject_id,
        "operator-1"
    );

    EXPECT_EQ(
        decision.required_permission,
        dispatcher::auth::audit::AuthorizationPermission::runtime_read
    );
}

TEST(AuthAuthorizationPolicyTests, DeniesByDefaultWhenPermissionMissing)
{
    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator;

    const auto decision =
        evaluator.evaluate(
            make_request(
                dispatcher::auth::audit::AuthorizationPermission::runtime_control
            )
        );

    EXPECT_TRUE(
        decision.denied()
    );

    EXPECT_EQ(
        decision.reason,
        "denied_by_default_policy"
    );
}

TEST(AuthAuthorizationPolicyTests, AllowsByPolicyRule)
{
    dispatcher::auth::audit::AuthorizationPolicy policy;

    policy.allow_direct_subject_permissions = false;

    policy.rules.push_back(
        make_rule(
            "allow-runtime-control",
            dispatcher::auth::audit::AuthorizationDecisionEffect::allow,
            dispatcher::auth::audit::AuthorizationPermission::runtime_control
        )
    );

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator{
        policy
    };

    const auto decision =
        evaluator.evaluate(
            make_request(
                dispatcher::auth::audit::AuthorizationPermission::runtime_control
            )
        );

    EXPECT_TRUE(
        decision.allowed()
    );

    EXPECT_EQ(
        decision.reason,
        "allowed_by_policy_rule"
    );

    EXPECT_NE(
        decision.diagnostic_message.find(
            "allow-runtime-control"
        ),
        std::string::npos
    );
}

TEST(AuthAuthorizationPolicyTests, DenyRuleOverridesAllowRule)
{
    dispatcher::auth::audit::AuthorizationPolicy policy;

    policy.rules.push_back(
        make_rule(
            "allow-runtime-control",
            dispatcher::auth::audit::AuthorizationDecisionEffect::allow,
            dispatcher::auth::audit::AuthorizationPermission::runtime_control
        )
    );

    policy.rules.push_back(
        make_rule(
            "deny-runtime-control",
            dispatcher::auth::audit::AuthorizationDecisionEffect::deny,
            dispatcher::auth::audit::AuthorizationPermission::runtime_control
        )
    );

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator{
        policy
    };

    const auto decision =
        evaluator.evaluate(
            make_request(
                dispatcher::auth::audit::AuthorizationPermission::runtime_control
            )
        );

    EXPECT_TRUE(
        decision.denied()
    );

    EXPECT_EQ(
        decision.reason,
        "denied_by_policy_rule"
    );

    EXPECT_NE(
        decision.diagnostic_message.find(
            "deny-runtime-control"
        ),
        std::string::npos
    );
}

TEST(AuthAuthorizationPolicyTests, DisabledRuleIsIgnored)
{
    dispatcher::auth::audit::AuthorizationPolicy policy;

    auto rule =
        make_rule(
            "allow-runtime-control",
            dispatcher::auth::audit::AuthorizationDecisionEffect::allow,
            dispatcher::auth::audit::AuthorizationPermission::runtime_control
        );

    rule.enabled = false;

    policy.rules.push_back(
        rule
    );

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator{
        policy
    };

    const auto decision =
        evaluator.evaluate(
            make_request(
                dispatcher::auth::audit::AuthorizationPermission::runtime_control
            )
        );

    EXPECT_TRUE(
        decision.denied()
    );

    EXPECT_EQ(
        decision.reason,
        "denied_by_default_policy"
    );
}

TEST(AuthAuthorizationPolicyTests, RuleRoleMustMatchWhenProvided)
{
    dispatcher::auth::audit::AuthorizationPolicy policy;

    auto rule =
        make_rule(
            "allow-runtime-control-for-engineers",
            dispatcher::auth::audit::AuthorizationDecisionEffect::allow,
            dispatcher::auth::audit::AuthorizationPermission::runtime_control
        );

    rule.role = "engineers";

    policy.rules.push_back(
        rule
    );

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator{
        policy
    };

    const auto decision =
        evaluator.evaluate(
            make_request(
                dispatcher::auth::audit::AuthorizationPermission::runtime_control
            )
        );

    EXPECT_TRUE(
        decision.denied()
    );
}

TEST(AuthAuthorizationPolicyTests, EmptyRuleRoleMatchesAnySubjectRole)
{
    dispatcher::auth::audit::AuthorizationPolicy policy;

    auto rule =
        make_rule(
            "allow-runtime-control-for-any-role",
            dispatcher::auth::audit::AuthorizationDecisionEffect::allow,
            dispatcher::auth::audit::AuthorizationPermission::runtime_control
        );

    rule.role = "";

    policy.rules.push_back(
        rule
    );

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator{
        policy
    };

    const auto decision =
        evaluator.evaluate(
            make_request(
                dispatcher::auth::audit::AuthorizationPermission::runtime_control
            )
        );

    EXPECT_TRUE(
        decision.allowed()
    );
}

TEST(AuthAuthorizationPolicyTests, RuleResourceMustMatchWhenProvided)
{
    dispatcher::auth::audit::AuthorizationPolicy policy;

    auto rule =
        make_rule(
            "allow-runtime-control-for-other-runtime",
            dispatcher::auth::audit::AuthorizationDecisionEffect::allow,
            dispatcher::auth::audit::AuthorizationPermission::runtime_control
        );

    rule.resource_id = "other-runtime";

    policy.rules.push_back(
        rule
    );

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator{
        policy
    };

    const auto decision =
        evaluator.evaluate(
            make_request(
                dispatcher::auth::audit::AuthorizationPermission::runtime_control
            )
        );

    EXPECT_TRUE(
        decision.denied()
    );
}

TEST(AuthAuthorizationPolicyTests, EmptyRuleResourceFieldsMatchAnyResource)
{
    dispatcher::auth::audit::AuthorizationPolicy policy;

    auto rule =
        make_rule(
            "allow-runtime-control-any-resource",
            dispatcher::auth::audit::AuthorizationDecisionEffect::allow,
            dispatcher::auth::audit::AuthorizationPermission::runtime_control
        );

    rule.resource_type = "";
    rule.resource_id = "";

    policy.rules.push_back(
        rule
    );

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator{
        policy
    };

    const auto decision =
        evaluator.evaluate(
            make_request(
                dispatcher::auth::audit::AuthorizationPermission::runtime_control
            )
        );

    EXPECT_TRUE(
        decision.allowed()
    );
}

TEST(AuthAuthorizationPolicyTests, AdministratorSubjectPermissionAllowsAnyPermission)
{
    auto request =
        make_request(
            dispatcher::auth::audit::AuthorizationPermission::configuration_import
        );

    request.subject.permissions.clear();

    request.subject.permissions.push_back(
        dispatcher::auth::audit::AuthorizationPermission::administrator
    );

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator;

    const auto decision =
        evaluator.evaluate(
            request
        );

    EXPECT_TRUE(
        decision.allowed()
    );

    EXPECT_EQ(
        decision.reason,
        "allowed_by_subject_permission"
    );
}

TEST(AuthAuthorizationPolicyTests, AdministratorRulePermissionMatchesAnyRequestedPermission)
{
    dispatcher::auth::audit::AuthorizationPolicy policy;

    auto rule =
        make_rule(
            "allow-admin-rule",
            dispatcher::auth::audit::AuthorizationDecisionEffect::allow,
            dispatcher::auth::audit::AuthorizationPermission::administrator
        );

    rule.role = "operators";
    rule.resource_type = "";
    rule.resource_id = "";

    policy.rules.push_back(
        rule
    );

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator{
        policy
    };

    const auto decision =
        evaluator.evaluate(
            make_request(
                dispatcher::auth::audit::AuthorizationPermission::configuration_export
            )
        );

    EXPECT_TRUE(
        decision.allowed()
    );

    EXPECT_EQ(
        decision.reason,
        "allowed_by_policy_rule"
    );
}

TEST(AuthAuthorizationPolicyTests, DefaultAllowPolicyAllowsWhenNothingMatches)
{
    dispatcher::auth::audit::AuthorizationPolicy policy;

    policy.allow_direct_subject_permissions = false;
    policy.default_effect =
        dispatcher::auth::audit::AuthorizationDecisionEffect::allow;

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator{
        policy
    };

    const auto decision =
        evaluator.evaluate(
            make_request(
                dispatcher::auth::audit::AuthorizationPermission::runtime_control
            )
        );

    EXPECT_TRUE(
        decision.allowed()
    );

    EXPECT_EQ(
        decision.reason,
        "allowed_by_default_policy"
    );
}

TEST(AuthAuthorizationPolicyTests, DirectSubjectPermissionsCanBeDisabled)
{
    dispatcher::auth::audit::AuthorizationPolicy policy;

    policy.allow_direct_subject_permissions = false;

    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator{
        policy
    };

    const auto decision =
        evaluator.evaluate(
            make_request(
                dispatcher::auth::audit::AuthorizationPermission::runtime_read
            )
        );

    EXPECT_TRUE(
        decision.denied()
    );

    EXPECT_EQ(
        decision.reason,
        "denied_by_default_policy"
    );
}

TEST(AuthAuthorizationPolicyTests, ConvertsEnumsToStrings)
{
    EXPECT_EQ(
        dispatcher::auth::audit::AuthorizationPolicyEvaluator::permission_to_string(
            dispatcher::auth::audit::AuthorizationPermission::alarm_acknowledge
        ),
        "alarm_acknowledge"
    );

    EXPECT_EQ(
        dispatcher::auth::audit::AuthorizationPolicyEvaluator::permission_to_string(
            dispatcher::auth::audit::AuthorizationPermission::administrator
        ),
        "administrator"
    );

    EXPECT_EQ(
        dispatcher::auth::audit::AuthorizationPolicyEvaluator::effect_to_string(
            dispatcher::auth::audit::AuthorizationDecisionEffect::allow
        ),
        "allow"
    );

    EXPECT_EQ(
        dispatcher::auth::audit::AuthorizationPolicyEvaluator::effect_to_string(
            dispatcher::auth::audit::AuthorizationDecisionEffect::deny
        ),
        "deny"
    );
}

TEST(AuthAuthorizationPolicyTests, UnknownEnumValuesConvertToUnknown)
{
    EXPECT_EQ(
        dispatcher::auth::audit::AuthorizationPolicyEvaluator::permission_to_string(
            static_cast<dispatcher::auth::audit::AuthorizationPermission>(
                999
                )
        ),
        "unknown"
    );

    EXPECT_EQ(
        dispatcher::auth::audit::AuthorizationPolicyEvaluator::effect_to_string(
            static_cast<dispatcher::auth::audit::AuthorizationDecisionEffect>(
                999
                )
        ),
        "unknown"
    );
}

TEST(AuthAuthorizationPolicyTests, InvalidEvaluateRequestThrows)
{
    dispatcher::auth::audit::AuthorizationPolicyEvaluator evaluator;

    auto request =
        make_request();

    request.resource.resource_id = "";

    expect_evaluate_throws(
        evaluator,
        request
    );
}