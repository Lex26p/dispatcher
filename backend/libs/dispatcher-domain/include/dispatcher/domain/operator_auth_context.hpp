#pragma once

#include <dispatcher/domain/operator_authorization_result.hpp>
#include <dispatcher/domain/operator_authorizer.hpp>
#include <dispatcher/domain/operator_identity.hpp>
#include <dispatcher/domain/operator_permission.hpp>
#include <dispatcher/domain/operator_session.hpp>

#include <optional>
#include <string>

namespace dispatcher::domain
{
    class OperatorAuthContext
    {
    public:
        explicit OperatorAuthContext(
            OperatorIdentity identity
        );

        OperatorAuthContext(
            OperatorIdentity identity,
            OperatorSession session
        );

        [[nodiscard]] static OperatorAuthContext from_identity(
            OperatorIdentity identity
        );

        [[nodiscard]] static OperatorAuthContext from_session(
            OperatorSession session
        );

        [[nodiscard]] const OperatorIdentity& identity() const noexcept;

        [[nodiscard]] const OperatorId& operator_id() const noexcept;

        [[nodiscard]] const std::string& username() const noexcept;

        [[nodiscard]] bool has_session() const noexcept;

        [[nodiscard]] const OperatorSession& session() const;

        [[nodiscard]] const OperatorSessionId& session_id() const;

        [[nodiscard]] bool session_usable(
            OperatorSession::TimePoint now = OperatorSession::Clock::now()
        ) const noexcept;

        [[nodiscard]] bool active_operator() const noexcept;

        [[nodiscard]] bool authenticated(
            OperatorSession::TimePoint now = OperatorSession::Clock::now()
        ) const noexcept;

        [[nodiscard]] OperatorAuthorizationResult authorize(
            OperatorPermission permission,
            const OperatorAuthorizer& authorizer = OperatorAuthorizer{},
            OperatorSession::TimePoint now = OperatorSession::Clock::now()
        ) const;

        [[nodiscard]] bool is_authorized(
            OperatorPermission permission,
            const OperatorAuthorizer& authorizer = OperatorAuthorizer{},
            OperatorSession::TimePoint now = OperatorSession::Clock::now()
        ) const noexcept;

    private:
        OperatorIdentity identity_;
        std::optional<OperatorSession> session_;
    };
}