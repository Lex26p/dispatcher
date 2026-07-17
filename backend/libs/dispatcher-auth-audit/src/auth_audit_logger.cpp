#include <dispatcher/auth/audit/auth_audit_logger.hpp>

#include <dispatcher/auth/audit/auth_audit_error.hpp>
#include <dispatcher/auth/audit/auth_audit_validator.hpp>

#include <exception>

namespace dispatcher::auth::audit
{
    AuthAuditLogger::AuthAuditLogger(
        IAuthAuditSink& sink,
        bool enabled
    )
        : sink_(
            &sink
        )
        , enabled_(
            enabled
        )
    {
        if (sink.sink_name().empty())
        {
            throw AuthAuditError(
                "Auth audit sink_name must not be empty."
            );
        }
    }

    void AuthAuditLogger::set_enabled(
        bool enabled
    ) noexcept
    {
        enabled_ =
            enabled;
    }

    bool AuthAuditLogger::enabled() const noexcept
    {
        return enabled_;
    }

    std::string AuthAuditLogger::sink_name() const
    {
        return sink_->sink_name();
    }

    AuthAuditRecordResult AuthAuditLogger::record(
        const AuthAuditEvent& event
    )
    {
        AuthAuditValidator::validate_event(
            event
        );

        if (!enabled_)
        {
            return AuthAuditRecordResult::skipped(
                "Auth audit logger is disabled."
            );
        }

        try
        {
            auto result =
                sink_->record(
                    event
                );

            AuthAuditValidator::validate_record_result(
                result
            );

            return result;
        }
        catch (const std::exception& exception)
        {
            return AuthAuditRecordResult::failed(
                exception.what(),
                "Auth audit sink threw an exception."
            );
        }
    }

    std::vector<AuthAuditRecordResult> AuthAuditLogger::record_batch(
        const std::vector<AuthAuditEvent>& events
    )
    {
        std::vector<AuthAuditRecordResult> results;

        results.reserve(
            events.size()
        );

        for (const auto& event : events)
        {
            results.push_back(
                record(
                    event
                )
            );
        }

        return results;
    }
}