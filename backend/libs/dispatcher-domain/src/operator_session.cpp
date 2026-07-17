#include <dispatcher/domain/operator_session.hpp>

#include <utility>

namespace dispatcher::domain
{
    OperatorSession::OperatorSession(
        OperatorSessionId session_id,
        OperatorIdentity identity,
        TimePoint signed_in_at,
        TimePoint expires_at,
        std::string source
    )
        : session_id_(std::move(session_id))
        , identity_(std::move(identity))
        , status_(OperatorSessionStatus::Active)
        , signed_in_at_(signed_in_at)
        , expires_at_(expires_at)
        , source_(std::move(source))
    {
    }

    OperatorSession OperatorSession::create(
        OperatorSessionId session_id,
        OperatorIdentity identity,
        std::chrono::seconds ttl,
        std::string source
    )
    {
        const auto now = Clock::now();

        return OperatorSession(
            std::move(session_id),
            std::move(identity),
            now,
            now + ttl,
            std::move(source)
        );
    }

    const OperatorSessionId& OperatorSession::session_id() const noexcept
    {
        return session_id_;
    }

    const OperatorIdentity& OperatorSession::identity() const noexcept
    {
        return identity_;
    }

    const OperatorId& OperatorSession::operator_id() const noexcept
    {
        return identity_.operator_id();
    }

    const std::string& OperatorSession::username() const noexcept
    {
        return identity_.username();
    }

    OperatorSessionStatus OperatorSession::status() const noexcept
    {
        return status_;
    }

    OperatorSession::TimePoint OperatorSession::signed_in_at() const noexcept
    {
        return signed_in_at_;
    }

    OperatorSession::TimePoint OperatorSession::signed_out_at() const noexcept
    {
        return signed_out_at_;
    }

    OperatorSession::TimePoint OperatorSession::expires_at() const noexcept
    {
        return expires_at_;
    }

    const std::string& OperatorSession::source() const noexcept
    {
        return source_;
    }

    bool OperatorSession::has_source() const noexcept
    {
        return !source_.empty();
    }

    bool OperatorSession::active() const noexcept
    {
        return status_ == OperatorSessionStatus::Active;
    }

    bool OperatorSession::signed_out() const noexcept
    {
        return status_ == OperatorSessionStatus::SignedOut;
    }

    bool OperatorSession::expired(TimePoint now) const noexcept
    {
        return status_ == OperatorSessionStatus::Expired
            || now >= expires_at_;
    }

    bool OperatorSession::terminal() const noexcept
    {
        return is_terminal(status_);
    }

    bool OperatorSession::usable(TimePoint now) const noexcept
    {
        return active()
            && !expired(now);
    }

    void OperatorSession::sign_out(TimePoint signed_out_at) noexcept
    {
        if (terminal())
        {
            return;
        }

        status_ = OperatorSessionStatus::SignedOut;
        signed_out_at_ = signed_out_at;
    }

    void OperatorSession::expire(TimePoint expired_at) noexcept
    {
        if (terminal())
        {
            return;
        }

        status_ = OperatorSessionStatus::Expired;
        signed_out_at_ = expired_at;
    }
}