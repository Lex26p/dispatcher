#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/operator_identity.hpp>
#include <dispatcher/domain/operator_session_status.hpp>

#include <chrono>
#include <string>

namespace dispatcher::domain
{
    class OperatorSession
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

        OperatorSession(
            OperatorSessionId session_id,
            OperatorIdentity identity,
            TimePoint signed_in_at,
            TimePoint expires_at,
            std::string source = {}
        );

        [[nodiscard]] static OperatorSession create(
            OperatorSessionId session_id,
            OperatorIdentity identity,
            std::chrono::seconds ttl,
            std::string source = {}
        );

        [[nodiscard]] const OperatorSessionId& session_id() const noexcept;

        [[nodiscard]] const OperatorIdentity& identity() const noexcept;

        [[nodiscard]] const OperatorId& operator_id() const noexcept;

        [[nodiscard]] const std::string& username() const noexcept;

        [[nodiscard]] OperatorSessionStatus status() const noexcept;

        [[nodiscard]] TimePoint signed_in_at() const noexcept;

        [[nodiscard]] TimePoint signed_out_at() const noexcept;

        [[nodiscard]] TimePoint expires_at() const noexcept;

        [[nodiscard]] const std::string& source() const noexcept;

        [[nodiscard]] bool has_source() const noexcept;

        [[nodiscard]] bool active() const noexcept;

        [[nodiscard]] bool signed_out() const noexcept;

        [[nodiscard]] bool expired(TimePoint now = Clock::now()) const noexcept;

        [[nodiscard]] bool terminal() const noexcept;

        [[nodiscard]] bool usable(TimePoint now = Clock::now()) const noexcept;

        void sign_out(TimePoint signed_out_at = Clock::now()) noexcept;

        void expire(TimePoint expired_at = Clock::now()) noexcept;

    private:
        OperatorSessionId session_id_;
        OperatorIdentity identity_;
        OperatorSessionStatus status_{ OperatorSessionStatus::InvalidSession };
        TimePoint signed_in_at_{};
        TimePoint signed_out_at_{};
        TimePoint expires_at_{};
        std::string source_;
    };
}