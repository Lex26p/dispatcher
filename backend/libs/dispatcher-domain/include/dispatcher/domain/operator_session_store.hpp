#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/operator_identity.hpp>
#include <dispatcher/domain/operator_session.hpp>
#include <dispatcher/domain/operator_session_result.hpp>

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace dispatcher::domain
{
    class OperatorSessionStore
    {
    public:
        using Clock = OperatorSession::Clock;
        using TimePoint = OperatorSession::TimePoint;

        [[nodiscard]] OperatorSessionResult sign_in(
            OperatorSessionId session_id,
            OperatorIdentity identity,
            std::chrono::seconds ttl,
            std::string source = {}
        );

        [[nodiscard]] OperatorSessionResult sign_out(
            const OperatorSessionId& session_id,
            TimePoint signed_out_at = Clock::now()
        );

        [[nodiscard]] std::optional<OperatorSession> find(
            const OperatorSessionId& session_id
        ) const;

        [[nodiscard]] bool contains(
            const OperatorSessionId& session_id
        ) const;

        [[nodiscard]] bool active(
            const OperatorSessionId& session_id,
            TimePoint now = Clock::now()
        ) const;

        [[nodiscard]] std::vector<OperatorSession> sessions() const;

        [[nodiscard]] std::vector<OperatorSession> active_sessions(
            TimePoint now = Clock::now()
        ) const;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        std::size_t expire_sessions(
            TimePoint now = Clock::now()
        );

        void clear() noexcept;

    private:
        std::unordered_map<std::string, OperatorSession> sessions_by_id_;
    };
}