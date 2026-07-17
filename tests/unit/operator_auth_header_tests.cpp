#include <dispatcher/domain/operator_auth.hpp>

#include <gtest/gtest.h>

#include <chrono>

TEST(OperatorAuthHeaderTests, UmbrellaHeaderCanBeIncluded)
{
    const dispatcher::domain::OperatorIdentity identity(
        dispatcher::domain::OperatorId{ "operator-1" },
        "operator.one",
        dispatcher::domain::OperatorRole::Operator,
        dispatcher::domain::OperatorStatus::Active
    );

    const auto context =
        dispatcher::domain::OperatorAuthContext::from_identity(
            identity
        );

    EXPECT_TRUE(
        context.is_authorized(
            dispatcher::domain::OperatorPermission::AcknowledgeAlarms
        )
    );

    dispatcher::domain::OperatorDirectory directory;

    EXPECT_TRUE(
        directory.add(identity).ok()
    );

    dispatcher::domain::OperatorSessionStore sessions;

    const auto sign_in_result =
        sessions.sign_in(
            dispatcher::domain::OperatorSessionId{ "session-1" },
            identity,
            std::chrono::seconds{ 60 },
            "api"
        );

    EXPECT_TRUE(sign_in_result.ok());

    const auto authorization_result =
        dispatcher::domain::OperatorAuthorizer{}.authorize(
            identity,
            dispatcher::domain::OperatorPermission::AcknowledgeAlarms
        );

    EXPECT_TRUE(authorization_result.authorized());
}