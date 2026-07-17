#include <dispatcher/domain/operator_session_store.hpp>

#include <algorithm>
#include <utility>

namespace dispatcher::domain
{
    OperatorSessionResult OperatorSessionStore::sign_in(
        OperatorSessionId session_id,
        OperatorIdentity identity,
        std::chrono::seconds ttl,
        std::string source
    )
    {
        const auto session_id_value = session_id.value();

        if (session_id_value.empty())
        {
            return OperatorSessionResult::failure(
                OperatorSessionStatus::InvalidSession,
                "session id is empty",
                "session_id",
                {}
            );
        }

        if (!identity.active())
        {
            return OperatorSessionResult::failure(
                OperatorSessionStatus::InvalidSession,
                "operator identity is not active",
                "operator_status",
                to_string(identity.status())
            );
        }

        if (ttl <= std::chrono::seconds::zero())
        {
            return OperatorSessionResult::failure(
                OperatorSessionStatus::InvalidSession,
                "session ttl must be positive",
                "ttl",
                {}
            );
        }

        if (sessions_by_id_.contains(session_id_value))
        {
            return OperatorSessionResult::failure(
                OperatorSessionStatus::InvalidSession,
                "session id is already registered",
                "session_id",
                session_id_value
            );
        }

        auto session = OperatorSession::create(
            std::move(session_id),
            std::move(identity),
            ttl,
            std::move(source)
        );

        const auto session_key = session.session_id().value();

        sessions_by_id_.emplace(
            session_key,
            session
        );

        return OperatorSessionResult::success(
            std::move(session),
            "operator signed in"
        );
    }

    OperatorSessionResult OperatorSessionStore::sign_out(
        const OperatorSessionId& session_id,
        TimePoint signed_out_at
    )
    {
        const auto session_id_value = session_id.value();

        if (session_id_value.empty())
        {
            return OperatorSessionResult::failure(
                OperatorSessionStatus::InvalidSession,
                "session id is empty",
                "session_id",
                {}
            );
        }

        auto iterator = sessions_by_id_.find(session_id_value);

        if (iterator == sessions_by_id_.end())
        {
            return OperatorSessionResult::failure(
                OperatorSessionStatus::NotFound,
                "session not found",
                "session_id",
                session_id_value
            );
        }

        iterator->second.sign_out(signed_out_at);

        return OperatorSessionResult::success(
            iterator->second,
            "operator signed out"
        );
    }

    std::optional<OperatorSession> OperatorSessionStore::find(
        const OperatorSessionId& session_id
    ) const
    {
        const auto iterator =
            sessions_by_id_.find(session_id.value());

        if (iterator == sessions_by_id_.end())
        {
            return std::nullopt;
        }

        return iterator->second;
    }

    bool OperatorSessionStore::contains(
        const OperatorSessionId& session_id
    ) const
    {
        return sessions_by_id_.contains(session_id.value());
    }

    bool OperatorSessionStore::active(
        const OperatorSessionId& session_id,
        TimePoint now
    ) const
    {
        const auto session = find(session_id);

        if (!session.has_value())
        {
            return false;
        }

        return session->usable(now);
    }

    std::vector<OperatorSession> OperatorSessionStore::sessions() const
    {
        std::vector<OperatorSession> result;

        result.reserve(sessions_by_id_.size());

        for (const auto& [id, session] : sessions_by_id_)
        {
            result.push_back(session);
        }

        std::sort(
            result.begin(),
            result.end(),
            [](const OperatorSession& left, const OperatorSession& right)
            {
                return left.session_id().value()
                    < right.session_id().value();
            }
        );

        return result;
    }

    std::vector<OperatorSession> OperatorSessionStore::active_sessions(
        TimePoint now
    ) const
    {
        std::vector<OperatorSession> result;

        for (const auto& [id, session] : sessions_by_id_)
        {
            if (session.usable(now))
            {
                result.push_back(session);
            }
        }

        std::sort(
            result.begin(),
            result.end(),
            [](const OperatorSession& left, const OperatorSession& right)
            {
                return left.session_id().value()
                    < right.session_id().value();
            }
        );

        return result;
    }

    std::size_t OperatorSessionStore::size() const noexcept
    {
        return sessions_by_id_.size();
    }

    bool OperatorSessionStore::empty() const noexcept
    {
        return sessions_by_id_.empty();
    }

    std::size_t OperatorSessionStore::expire_sessions(
        TimePoint now
    )
    {
        std::size_t expired_count = 0;

        for (auto& [id, session] : sessions_by_id_)
        {
            if (session.active() && session.expired(now))
            {
                session.expire(now);
                ++expired_count;
            }
        }

        return expired_count;
    }

    void OperatorSessionStore::clear() noexcept
    {
        sessions_by_id_.clear();
    }
}