#pragma once

#include <dispatcher/auth/audit/auth_audit_types.hpp>

#include <string>

namespace dispatcher::auth::audit
{
    class IAuthAuditSink
    {
    public:
        virtual ~IAuthAuditSink() = default;

        [[nodiscard]] virtual std::string sink_name() const = 0;

        [[nodiscard]] virtual AuthAuditRecordResult record(
            const AuthAuditEvent& event
        ) = 0;
    };
}