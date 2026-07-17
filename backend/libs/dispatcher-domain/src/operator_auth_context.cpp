#include <dispatcher/domain/operator_auth_context.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::domain
{
    OperatorAuthContext::OperatorAuthContext(
        OperatorIdentity identity
    )
        : identity_(std::move(identity))
    {
    }

    OperatorAuthContext::OperatorAuthContext(
        OperatorIdentity identity,
        OperatorSession session
    )
        : identity_(std::move(identity))
        , session_(std::move(session))
    {
    }

    OperatorAuthContext OperatorAuthContext::from_identity(
        OperatorIdentity identity
    )
    {
        return OperatorAuthContext(
            std::move(identity)
        );
    }

    OperatorAuthContext OperatorAuthContext::from_session(
        OperatorSession session
    )
    {
        auto identity = session.identity();

        return OperatorAuthContext(
            std::move(identity),
            std::move(session)
        );
    }

    const OperatorIdentity& OperatorAuthContext::identity() const noexcept
    {
        return identity_;
    }

    const OperatorId& OperatorAuthContext::operator_id() const noexcept
    {
        return identity_.operator_id();
    }

    const std::string& OperatorAuthContext::username() const noexcept
    {
        return identity_.username();
    }

    bool OperatorAuthContext::has_session() const noexcept
    {
        return session_.has_value();
    }

    const OperatorSession& OperatorAuthContext::session() const
    {
        if (!session_.has_value())
        {
            throw std::logic_error(
                "OperatorAuthContext does not contain a session"
            );
        }

        return session_.value();
    }

    const OperatorSessionId& OperatorAuthContext::session_id() const
    {
        return session().session_id();
    }

    bool OperatorAuthContext::session_usable(
        OperatorSession::TimePoint now
    ) const noexcept
    {
        if (!session_.has_value())
        {
            return false;
        }

        return session_->usable(now);
    }

    bool OperatorAuthContext::active_operator() const noexcept
    {
        return identity_.active();
    }

    bool OperatorAuthContext::authenticated(
        OperatorSession::TimePoint now
    ) const noexcept
    {
        if (!active_operator())
        {
            return false;
        }

        if (!session_.has_value())
        {
            return true;
        }

        return session_->usable(now);
    }

    OperatorAuthorizationResult OperatorAuthContext::authorize(
        OperatorPermission permission,
        const OperatorAuthorizer& authorizer,
        OperatorSession::TimePoint now
    ) const
    {
        if (!authenticated(now))
        {
            return OperatorAuthorizationResult::denied(
                OperatorId{ identity_.operator_id().value() },
                identity_.username(),
                identity_.role(),
                identity_.status(),
                permission,
                OperatorAuthorizationStatus::DeniedInactiveOperator,
                "operator context is not authenticated"
            );
        }

        return authorizer.authorize(
            identity_,
            permission
        );
    }

    bool OperatorAuthContext::is_authorized(
        OperatorPermission permission,
        const OperatorAuthorizer& authorizer,
        OperatorSession::TimePoint now
    ) const noexcept
    {
        if (!authenticated(now))
        {
            return false;
        }

        return authorizer.is_authorized(
            identity_,
            permission
        );
    }
}