#include <dispatcher/auth/audit/in_memory_auth_audit_sink.hpp>

#include <dispatcher/auth/audit/auth_audit_validator.hpp>

#include <utility>

namespace dispatcher::auth::audit
{
    InMemoryAuthAuditSink::InMemoryAuthAuditSink(
        std::string sink_name
    )
        : sink_name_(
            std::move(
                sink_name
            )
        )
    {
    }

    std::string InMemoryAuthAuditSink::sink_name() const
    {
        return sink_name_;
    }

    AuthAuditRecordResult InMemoryAuthAuditSink::record(
        const AuthAuditEvent& event
    )
    {
        ++record_attempt_count_;

        AuthAuditValidator::validate_event(
            event
        );

        if (!recording_enabled_)
        {
            return AuthAuditRecordResult::skipped(
                "In-memory auth audit sink is disabled."
            );
        }

        if (!failure_message_.empty())
        {
            return AuthAuditRecordResult::failed(
                failure_message_,
                "In-memory auth audit sink forced failure."
            );
        }

        recorded_events_.push_back(
            event
        );

        return AuthAuditRecordResult::accepted(
            provider_record_id_for(
                event
            ),
            "Recorded by in-memory auth audit sink."
        );
    }

    void InMemoryAuthAuditSink::set_failure(
        std::string error_message
    )
    {
        failure_message_ =
            std::move(
                error_message
            );
    }

    void InMemoryAuthAuditSink::clear_failure()
    {
        failure_message_.clear();
    }

    void InMemoryAuthAuditSink::set_recording_enabled(
        bool enabled
    ) noexcept
    {
        recording_enabled_ =
            enabled;
    }

    void InMemoryAuthAuditSink::clear()
    {
        failure_message_.clear();
        recording_enabled_ = true;
        record_attempt_count_ = 0;
        recorded_events_.clear();
    }

    bool InMemoryAuthAuditSink::recording_enabled() const noexcept
    {
        return recording_enabled_;
    }

    int InMemoryAuthAuditSink::record_attempt_count() const noexcept
    {
        return record_attempt_count_;
    }

    const std::vector<AuthAuditEvent>& InMemoryAuthAuditSink::recorded_events() const noexcept
    {
        return recorded_events_;
    }

    std::string InMemoryAuthAuditSink::provider_record_id_for(
        const AuthAuditEvent& event
    ) const
    {
        return sink_name_
            + ":"
            + event.event_id;
    }
}