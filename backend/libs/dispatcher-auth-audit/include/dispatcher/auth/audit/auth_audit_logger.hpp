#pragma once

#include <dispatcher/auth/audit/auth_audit_sink.hpp>

#include <vector>

namespace dispatcher::auth::audit
{
    class AuthAuditLogger final
    {
    public:
        explicit AuthAuditLogger(
            IAuthAuditSink& sink,
            bool enabled = true
        );

        void set_enabled(
            bool enabled
        ) noexcept;

        [[nodiscard]] bool enabled() const noexcept;

        [[nodiscard]] std::string sink_name() const;

        [[nodiscard]] AuthAuditRecordResult record(
            const AuthAuditEvent& event
        );

        [[nodiscard]] std::vector<AuthAuditRecordResult> record_batch(
            const std::vector<AuthAuditEvent>& events
        );

    private:
        IAuthAuditSink* sink_;
        bool enabled_{ true };
    };
}