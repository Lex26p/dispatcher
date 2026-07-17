#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/operator_auth_context.hpp>
#include <dispatcher/domain/operator_authorization_status.hpp>
#include <dispatcher/domain/operator_authorizer.hpp>
#include <dispatcher/domain/operator_identity.hpp>
#include <dispatcher/domain/operator_permission.hpp>
#include <dispatcher/domain/operator_role.hpp>
#include <dispatcher/domain/operator_session.hpp>
#include <dispatcher/domain/operator_status.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    dispatcher::domain::OperatorIdentity make_context_identity(
        std::string operator_id = "operator-1",
        std::string username = "operator.one",
        dispatcher::domain::OperatorRole role =
        dispatcher::domain::OperatorRole::Operator,
        dispatcher::domain::OperatorStatus status =
        dispatcher::domain::OperatorStatus::Active
    )
    {
        return dispatcher::domain::OperatorIdentity(
            dispatcher::domain::OperatorId{ std::move(operator_id) },
            std::move(username),
            role,
            status
        );
    }

    dispatcher::domain::OperatorSession make_context_session(
        dispatcher::domain::OperatorIdentity identity =
        make_context_identity(),
        std::string session_id = "session-1",
        std::chrono::seconds ttl = std::chrono::seconds{ 60 },
        std::string source = "api"
    )
    {
        return dispatcher::domain::OperatorSession::create(
            dispatcher::domain::OperatorSessionId{ std::move(session_id) },
            std::move(identity),
            ttl,
            std::move(source)
        );
    }
}

TEST(OperatorAuthContextTests, FromIdentityCreatesSessionlessContext)
{
    const auto context =
        dispatcher::domain::OperatorAuthContext::from_identity(
            make_context_identity()
        );

    EXPECT_EQ(
        context.operator_id(),
        dispatcher::domain::OperatorId{ "operator-1" }
    );

    EXPECT_EQ(context.username(), "operator.one");
    EXPECT_FALSE(context.has_session());
    EXPECT_FALSE(context.session_usable());

    EXPECT_TRUE(context.active_operator());
    EXPECT_TRUE(context.authenticated());

    EXPECT_THROW(
        (void)context.session(),
        std::logic_error
    );

    EXPECT_THROW(
        (void)context.session_id(),
        std::logic_error
    );
}

TEST(OperatorAuthContextTests, FromSessionCreatesSessionContext)
{
    const auto session = make_context_session();

    const auto context =
        dispatcher::domain::OperatorAuthContext::from_session(
            session
        );

    EXPECT_TRUE(context.has_session());

    EXPECT_EQ(
        context.session_id(),
        dispatcher::domain::OperatorSessionId{ "session-1" }
    );

    EXPECT_EQ(
        context.operator_id(),
        dispatcher::domain::OperatorId{ "operator-1" }
    );

    EXPECT_EQ(context.username(), "operator.one");
    EXPECT_TRUE(context.session_usable());
    EXPECT_TRUE(context.authenticated());
}

TEST(OperatorAuthContextTests, ExpiredSessionIsNotAuthenticated)
{
    const auto now = dispatcher::domain::OperatorSession::Clock::now();

    const dispatcher::domain::OperatorSession session(
        dispatcher::domain::OperatorSessionId{ "session-1" },
        make_context_identity(),
        now,
        now + std::chrono::seconds{ 1 },
        "api"
    );

    const auto context =
        dispatcher::domain::OperatorAuthContext::from_session(
            session
        );

    EXPECT_TRUE(
        context.authenticated(
            now
        )
    );

    EXPECT_FALSE(
        context.authenticated(
            now + std::chrono::seconds{ 1 }
        )
    );

    EXPECT_FALSE(
        context.session_usable(
            now + std::chrono::seconds{ 1 }
        )
    );
}

TEST(OperatorAuthContextTests, SignedOutSessionIsNotAuthenticated)
{
    auto session = make_context_session();

    session.sign_out();

    const auto context =
        dispatcher::domain::OperatorAuthContext::from_session(
            session
        );

    EXPECT_TRUE(context.has_session());
    EXPECT_FALSE(context.session_usable());
    EXPECT_FALSE(context.authenticated());
}

TEST(OperatorAuthContextTests, InactiveIdentityIsNotAuthenticated)
{
    const auto context =
        dispatcher::domain::OperatorAuthContext::from_identity(
            make_context_identity(
                "operator-1",
                "operator.one",
                dispatcher::domain::OperatorRole::Operator,
                dispatcher::domain::OperatorStatus::Disabled
            )
        );

    EXPECT_FALSE(context.active_operator());
    EXPECT_FALSE(context.authenticated());
}

TEST(OperatorAuthContextTests, AuthorizeAllowsActiveOperatorWithPermission)
{
    const auto context =
        dispatcher::domain::OperatorAuthContext::from_identity(
            make_context_identity(
                "operator-1",
                "operator.one",
                dispatcher::domain::OperatorRole::Operator
            )
        );

    const auto result = context.authorize(
        dispatcher::domain::OperatorPermission::AcknowledgeAlarms
    );

    EXPECT_TRUE(result.authorized());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorAuthorizationStatus::Allowed
    );

    EXPECT_TRUE(
        context.is_authorized(
            dispatcher::domain::OperatorPermission::AcknowledgeAlarms
        )
    );
}

TEST(OperatorAuthContextTests, AuthorizeDeniesInsufficientRole)
{
    const auto context =
        dispatcher::domain::OperatorAuthContext::from_identity(
            make_context_identity(
                "viewer-1",
                "viewer.one",
                dispatcher::domain::OperatorRole::Viewer
            )
        );

    const auto result = context.authorize(
        dispatcher::domain::OperatorPermission::AcknowledgeAlarms
    );

    EXPECT_TRUE(result.denied());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorAuthorizationStatus::
        DeniedInsufficientRole
    );

    EXPECT_FALSE(
        context.is_authorized(
            dispatcher::domain::OperatorPermission::AcknowledgeAlarms
        )
    );
}

TEST(OperatorAuthContextTests, AuthorizeDeniesUnauthenticatedContext)
{
    auto session = make_context_session();

    session.sign_out();

    const auto context =
        dispatcher::domain::OperatorAuthContext::from_session(
            session
        );

    const auto result = context.authorize(
        dispatcher::domain::OperatorPermission::AcknowledgeAlarms
    );

    EXPECT_TRUE(result.denied());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorAuthorizationStatus::
        DeniedInactiveOperator
    );

    EXPECT_EQ(
        result.reason(),
        "operator context is not authenticated"
    );

    EXPECT_FALSE(
        context.is_authorized(
            dispatcher::domain::OperatorPermission::AcknowledgeAlarms
        )
    );
}

TEST(OperatorAuthContextTests, SessionContextAuthorizationUsesIdentityRole)
{
    const auto context =
        dispatcher::domain::OperatorAuthContext::from_session(
            make_context_session(
                make_context_identity(
                    "engineer-1",
                    "engineer.one",
                    dispatcher::domain::OperatorRole::Engineer
                ),
                "session-engineer"
            )
        );

    EXPECT_TRUE(
        context.is_authorized(
            dispatcher::domain::OperatorPermission::ManageConfiguration
        )
    );

    EXPECT_FALSE(
        context.is_authorized(
            dispatcher::domain::OperatorPermission::ManageOperators
        )
    );
}