#pragma once

#include <dispatcher/auth/audit/auth_audit_sink.hpp>

#include <string>
#include <vector>

namespace dispatcher::auth::audit
{
    class InMemoryAuthAuditSink final
        : public IAuthAuditSink
    {
    public:
        explicit InMemoryAuthAuditSink(
            std::string sink_name = "in-memory-auth-audit-sink"
        );

        [[nodiscard]] std::string sink_name() const override;

        [[nodiscard]] AuthAuditRecordResult record(
            const AuthAuditEvent& event
        ) override;

        void set_failure(
            std::string error_message
        );

        void clear_failure();

        void set_recording_enabled(
            bool enabled
        ) noexcept;

        void clear();

        [[nodiscard]] bool recording_enabled() const noexcept;

        [[nodiscard]] int record_attempt_count() const noexcept;

        [[nodiscard]] const std::vector<AuthAuditEvent>& recorded_events() const noexcept;

    private:
        std::string sink_name_;
        std::string failure_message_{};
        bool recording_enabled_{ true };
        int record_attempt_count_{ 0 };

        std::vector<AuthAuditEvent> recorded_events_{};

        [[nodiscard]] std::string provider_record_id_for(
            const AuthAuditEvent& event
        ) const;
    };
}