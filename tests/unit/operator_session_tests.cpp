#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/operator_identity.hpp>
#include <dispatcher/domain/operator_role.hpp>
#include <dispatcher/domain/operator_session.hpp>
#include <dispatcher/domain/operator_session_result.hpp>
#include <dispatcher/domain/operator_session_status.hpp>
#include <dispatcher/domain/operator_session_store.hpp>
#include <dispatcher/domain/operator_status.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    dispatcher::domain::OperatorIdentity make_session_identity(
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
}

TEST(OperatorSessionStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorSessionStatus::Active
        ),
        "active"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorSessionStatus::SignedOut
        ),
        "signed_out"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorSessionStatus::Expired
        ),
        "expired"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorSessionStatus::NotFound
        ),
        "not_found"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorSessionStatus::InvalidSession
        ),
        "invalid_session"
    );
}

TEST(OperatorSessionStatusTests, PredicatesWork)
{
    EXPECT_TRUE(
        dispatcher::domain::is_active(
            dispatcher::domain::OperatorSessionStatus::Active
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_active(
            dispatcher::domain::OperatorSessionStatus::SignedOut
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_terminal(
            dispatcher::domain::OperatorSessionStatus::SignedOut
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_terminal(
            dispatcher::domain::OperatorSessionStatus::Expired
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_terminal(
            dispatcher::domain::OperatorSessionStatus::Active
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_failure(
            dispatcher::domain::OperatorSessionStatus::NotFound
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_failure(
            dispatcher::domain::OperatorSessionStatus::InvalidSession
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_failure(
            dispatcher::domain::OperatorSessionStatus::Active
        )
    );
}

TEST(OperatorSessionTests, CreateBuildsActiveSession)
{
    const auto session =
        dispatcher::domain::OperatorSession::create(
            dispatcher::domain::OperatorSessionId{ "session-1" },
            make_session_identity(),
            std::chrono::seconds{ 60 },
            "api"
        );

    EXPECT_EQ(
        session.session_id(),
        dispatcher::domain::OperatorSessionId{ "session-1" }
    );

    EXPECT_EQ(
        session.operator_id(),
        dispatcher::domain::OperatorId{ "operator-1" }
    );

    EXPECT_EQ(session.username(), "operator.one");
    EXPECT_EQ(session.status(), dispatcher::domain::OperatorSessionStatus::Active);
    EXPECT_TRUE(session.active());
    EXPECT_FALSE(session.signed_out());
    EXPECT_FALSE(session.terminal());
    EXPECT_TRUE(session.usable());

    EXPECT_TRUE(session.has_source());
    EXPECT_EQ(session.source(), "api");

    EXPECT_GT(
        session.expires_at(),
        session.signed_in_at()
    );
}

TEST(OperatorSessionTests, ExpiredDetectsTimeBasedExpiration)
{
    const auto now = dispatcher::domain::OperatorSession::Clock::now();

    const dispatcher::domain::OperatorSession session(
        dispatcher::domain::OperatorSessionId{ "session-1" },
        make_session_identity(),
        now,
        now + std::chrono::seconds{ 10 },
        "api"
    );

    EXPECT_FALSE(
        session.expired(
            now + std::chrono::seconds{ 5 }
        )
    );

    EXPECT_TRUE(
        session.expired(
            now + std::chrono::seconds{ 10 }
        )
    );

    EXPECT_FALSE(
        session.usable(
            now + std::chrono::seconds{ 10 }
        )
    );
}

TEST(OperatorSessionTests, SignOutMarksSessionTerminal)
{
    auto session =
        dispatcher::domain::OperatorSession::create(
            dispatcher::domain::OperatorSessionId{ "session-1" },
            make_session_identity(),
            std::chrono::seconds{ 60 },
            "api"
        );

    session.sign_out();

    EXPECT_EQ(
        session.status(),
        dispatcher::domain::OperatorSessionStatus::SignedOut
    );

    EXPECT_TRUE(session.signed_out());
    EXPECT_TRUE(session.terminal());
    EXPECT_FALSE(session.usable());

    const auto signed_out_at = session.signed_out_at();

    session.expire();

    EXPECT_EQ(
        session.status(),
        dispatcher::domain::OperatorSessionStatus::SignedOut
    );

    EXPECT_EQ(session.signed_out_at(), signed_out_at);
}

TEST(OperatorSessionTests, ExpireMarksSessionTerminal)
{
    auto session =
        dispatcher::domain::OperatorSession::create(
            dispatcher::domain::OperatorSessionId{ "session-1" },
            make_session_identity(),
            std::chrono::seconds{ 60 },
            "api"
        );

    session.expire();

    EXPECT_EQ(
        session.status(),
        dispatcher::domain::OperatorSessionStatus::Expired
    );

    EXPECT_TRUE(session.terminal());
    EXPECT_FALSE(session.usable());

    const auto expired_at = session.signed_out_at();

    session.sign_out();

    EXPECT_EQ(
        session.status(),
        dispatcher::domain::OperatorSessionStatus::Expired
    );

    EXPECT_EQ(session.signed_out_at(), expired_at);
}

TEST(OperatorSessionResultTests, SuccessContainsSession)
{
    auto session =
        dispatcher::domain::OperatorSession::create(
            dispatcher::domain::OperatorSessionId{ "session-1" },
            make_session_identity(),
            std::chrono::seconds{ 60 },
            "api"
        );

    const auto result =
        dispatcher::domain::OperatorSessionResult::success(
            std::move(session),
            "operator signed in"
        );

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());
    EXPECT_TRUE(result.has_session());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorSessionStatus::Active
    );

    EXPECT_EQ(
        result.session().session_id(),
        dispatcher::domain::OperatorSessionId{ "session-1" }
    );

    EXPECT_TRUE(result.has_message());
    EXPECT_EQ(result.message(), "operator signed in");

    EXPECT_FALSE(result.has_field());
    EXPECT_FALSE(result.has_value());
}

TEST(OperatorSessionResultTests, FailureDoesNotContainSession)
{
    const auto result =
        dispatcher::domain::OperatorSessionResult::failure(
            dispatcher::domain::OperatorSessionStatus::NotFound,
            "session not found",
            "session_id",
            "missing"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());
    EXPECT_FALSE(result.has_session());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorSessionStatus::NotFound
    );

    EXPECT_TRUE(result.has_message());
    EXPECT_TRUE(result.has_field());
    EXPECT_TRUE(result.has_value());

    EXPECT_THROW(
        (void)result.session(),
        std::logic_error
    );
}

TEST(OperatorSessionStoreTests, DefaultStoreIsEmpty)
{
    const dispatcher::domain::OperatorSessionStore store;

    EXPECT_TRUE(store.empty());
    EXPECT_EQ(store.size(), 0);

    EXPECT_FALSE(
        store.contains(
            dispatcher::domain::OperatorSessionId{ "session-1" }
        )
    );

    EXPECT_FALSE(
        store.active(
            dispatcher::domain::OperatorSessionId{ "session-1" }
        )
    );

    EXPECT_TRUE(store.sessions().empty());
    EXPECT_TRUE(store.active_sessions().empty());
}

TEST(OperatorSessionStoreTests, SignInStoresActiveSession)
{
    dispatcher::domain::OperatorSessionStore store;

    const auto result =
        store.sign_in(
            dispatcher::domain::OperatorSessionId{ "session-1" },
            make_session_identity(),
            std::chrono::seconds{ 60 },
            "api"
        );

    EXPECT_TRUE(result.ok());
    EXPECT_TRUE(result.has_session());

    EXPECT_FALSE(store.empty());
    EXPECT_EQ(store.size(), 1);

    EXPECT_TRUE(
        store.contains(
            dispatcher::domain::OperatorSessionId{ "session-1" }
        )
    );

    EXPECT_TRUE(
        store.active(
            dispatcher::domain::OperatorSessionId{ "session-1" }
        )
    );

    const auto found =
        store.find(
            dispatcher::domain::OperatorSessionId{ "session-1" }
        );

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->username(), "operator.one");
    EXPECT_TRUE(found->usable());
}

TEST(OperatorSessionStoreTests, SignInRejectsEmptySessionId)
{
    dispatcher::domain::OperatorSessionStore store;

    const auto result =
        store.sign_in(
            dispatcher::domain::OperatorSessionId{ "" },
            make_session_identity(),
            std::chrono::seconds{ 60 }
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorSessionStatus::InvalidSession
    );

    EXPECT_EQ(result.field(), "session_id");
    EXPECT_TRUE(store.empty());
}

TEST(OperatorSessionStoreTests, SignInRejectsInactiveIdentity)
{
    dispatcher::domain::OperatorSessionStore store;

    const auto result =
        store.sign_in(
            dispatcher::domain::OperatorSessionId{ "session-1" },
            make_session_identity(
                "operator-1",
                "operator.one",
                dispatcher::domain::OperatorRole::Operator,
                dispatcher::domain::OperatorStatus::Disabled
            ),
            std::chrono::seconds{ 60 }
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorSessionStatus::InvalidSession
    );

    EXPECT_EQ(result.field(), "operator_status");
    EXPECT_EQ(result.value(), "disabled");
    EXPECT_TRUE(store.empty());
}

TEST(OperatorSessionStoreTests, SignInRejectsNonPositiveTtl)
{
    dispatcher::domain::OperatorSessionStore store;

    const auto result =
        store.sign_in(
            dispatcher::domain::OperatorSessionId{ "session-1" },
            make_session_identity(),
            std::chrono::seconds{ 0 }
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorSessionStatus::InvalidSession
    );

    EXPECT_EQ(result.field(), "ttl");
    EXPECT_TRUE(store.empty());
}

TEST(OperatorSessionStoreTests, SignInRejectsDuplicateSessionId)
{
    dispatcher::domain::OperatorSessionStore store;

    ASSERT_TRUE(
        store.sign_in(
            dispatcher::domain::OperatorSessionId{ "session-1" },
            make_session_identity(),
            std::chrono::seconds{ 60 }
        ).ok()
    );

    const auto result =
        store.sign_in(
            dispatcher::domain::OperatorSessionId{ "session-1" },
            make_session_identity(
                "operator-2",
                "operator.two"
            ),
            std::chrono::seconds{ 60 }
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorSessionStatus::InvalidSession
    );

    EXPECT_EQ(result.field(), "session_id");
    EXPECT_EQ(result.value(), "session-1");
    EXPECT_EQ(store.size(), 1);
}

TEST(OperatorSessionStoreTests, SignOutMarksSessionSignedOut)
{
    dispatcher::domain::OperatorSessionStore store;

    ASSERT_TRUE(
        store.sign_in(
            dispatcher::domain::OperatorSessionId{ "session-1" },
            make_session_identity(),
            std::chrono::seconds{ 60 }
        ).ok()
    );

    const auto result =
        store.sign_out(
            dispatcher::domain::OperatorSessionId{ "session-1" }
        );

    EXPECT_TRUE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorSessionStatus::SignedOut
    );

    EXPECT_TRUE(result.session().signed_out());

    EXPECT_FALSE(
        store.active(
            dispatcher::domain::OperatorSessionId{ "session-1" }
        )
    );

    const auto found =
        store.find(
            dispatcher::domain::OperatorSessionId{ "session-1" }
        );

    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->signed_out());
}

TEST(OperatorSessionStoreTests, SignOutRejectsMissingSession)
{
    dispatcher::domain::OperatorSessionStore store;

    const auto result =
        store.sign_out(
            dispatcher::domain::OperatorSessionId{ "missing" }
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorSessionStatus::NotFound
    );

    EXPECT_EQ(result.field(), "session_id");
    EXPECT_EQ(result.value(), "missing");
}

TEST(OperatorSessionStoreTests, SessionsAreReturnedSortedBySessionId)
{
    dispatcher::domain::OperatorSessionStore store;

    ASSERT_TRUE(
        store.sign_in(
            dispatcher::domain::OperatorSessionId{ "session-z" },
            make_session_identity(
                "operator-z",
                "zeta"
            ),
            std::chrono::seconds{ 60 }
        ).ok()
    );

    ASSERT_TRUE(
        store.sign_in(
            dispatcher::domain::OperatorSessionId{ "session-a" },
            make_session_identity(
                "operator-a",
                "alpha"
            ),
            std::chrono::seconds{ 60 }
        ).ok()
    );

    ASSERT_TRUE(
        store.sign_in(
            dispatcher::domain::OperatorSessionId{ "session-m" },
            make_session_identity(
                "operator-m",
                "middle"
            ),
            std::chrono::seconds{ 60 }
        ).ok()
    );

    const auto sessions = store.sessions();

    ASSERT_EQ(sessions.size(), 3);

    EXPECT_EQ(sessions[0].session_id().value(), "session-a");
    EXPECT_EQ(sessions[1].session_id().value(), "session-m");
    EXPECT_EQ(sessions[2].session_id().value(), "session-z");
}

TEST(OperatorSessionStoreTests, ActiveSessionsOnlyReturnUsableSessions)
{
    dispatcher::domain::OperatorSessionStore store;

    ASSERT_TRUE(
        store.sign_in(
            dispatcher::domain::OperatorSessionId{ "session-active" },
            make_session_identity(
                "operator-1",
                "operator.one"
            ),
            std::chrono::seconds{ 60 }
        ).ok()
    );

    ASSERT_TRUE(
        store.sign_in(
            dispatcher::domain::OperatorSessionId{ "session-signed-out" },
            make_session_identity(
                "operator-2",
                "operator.two"
            ),
            std::chrono::seconds{ 60 }
        ).ok()
    );

    ASSERT_TRUE(
        store.sign_out(
            dispatcher::domain::OperatorSessionId{ "session-signed-out" }
        ).ok()
    );

    const auto active_sessions = store.active_sessions();

    ASSERT_EQ(active_sessions.size(), 1);
    EXPECT_EQ(
        active_sessions.front().session_id(),
        dispatcher::domain::OperatorSessionId{ "session-active" }
    );
}

TEST(OperatorSessionStoreTests, ExpireSessionsMarksExpiredSessions)
{
    dispatcher::domain::OperatorSessionStore store;

    ASSERT_TRUE(
        store.sign_in(
            dispatcher::domain::OperatorSessionId{ "session-1" },
            make_session_identity(),
            std::chrono::seconds{ 1 }
        ).ok()
    );

    const auto session =
        store.find(
            dispatcher::domain::OperatorSessionId{ "session-1" }
        );

    ASSERT_TRUE(session.has_value());

    const auto expired_count =
        store.expire_sessions(
            session->expires_at()
        );

    EXPECT_EQ(expired_count, 1);

    const auto found =
        store.find(
            dispatcher::domain::OperatorSessionId{ "session-1" }
        );

    ASSERT_TRUE(found.has_value());

    EXPECT_EQ(
        found->status(),
        dispatcher::domain::OperatorSessionStatus::Expired
    );

    EXPECT_FALSE(
        store.active(
            dispatcher::domain::OperatorSessionId{ "session-1" }
        )
    );
}

TEST(OperatorSessionStoreTests, ClearRemovesEverything)
{
    dispatcher::domain::OperatorSessionStore store;

    ASSERT_TRUE(
        store.sign_in(
            dispatcher::domain::OperatorSessionId{ "session-1" },
            make_session_identity(),
            std::chrono::seconds{ 60 }
        ).ok()
    );

    ASSERT_EQ(store.size(), 1);

    store.clear();

    EXPECT_TRUE(store.empty());
    EXPECT_EQ(store.size(), 0);

    EXPECT_FALSE(
        store.contains(
            dispatcher::domain::OperatorSessionId{ "session-1" }
        )
    );
}