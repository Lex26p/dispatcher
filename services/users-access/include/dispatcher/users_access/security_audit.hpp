#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dispatcher::users_access {

enum class SecurityAuditEventType {
    bootstrap_admin_created,
    user_created,
    user_enabled,
    user_disabled,
    user_password_reset,
    permission_set_created,
    access_assignment_added,
    access_assignment_removed,
    control_mode_enabled,
    control_mode_disabled,
    authentication_succeeded,
    authentication_failed,
    session_logged_out,
    session_expired,
    session_rejected_disabled_user,
};

[[nodiscard]] constexpr const char* security_audit_event_name(
    const SecurityAuditEventType event) noexcept {
    switch (event) {
    case SecurityAuditEventType::bootstrap_admin_created:
        return "bootstrap_admin_created";
    case SecurityAuditEventType::user_created:
        return "user_created";
    case SecurityAuditEventType::user_enabled:
        return "user_enabled";
    case SecurityAuditEventType::user_disabled:
        return "user_disabled";
    case SecurityAuditEventType::user_password_reset:
        return "user_password_reset";
    case SecurityAuditEventType::permission_set_created:
        return "permission_set_created";
    case SecurityAuditEventType::access_assignment_added:
        return "access_assignment_added";
    case SecurityAuditEventType::access_assignment_removed:
        return "access_assignment_removed";
    case SecurityAuditEventType::control_mode_enabled:
        return "control_mode_enabled";
    case SecurityAuditEventType::control_mode_disabled:
        return "control_mode_disabled";
    case SecurityAuditEventType::authentication_succeeded:
        return "authentication_succeeded";
    case SecurityAuditEventType::authentication_failed:
        return "authentication_failed";
    case SecurityAuditEventType::session_logged_out:
        return "session_logged_out";
    case SecurityAuditEventType::session_expired:
        return "session_expired";
    case SecurityAuditEventType::session_rejected_disabled_user:
        return "session_rejected_disabled_user";
    }
    return "unknown";
}

struct SecurityAuditRecord final {
    std::uint64_t sequence{0};
    std::int64_t occurred_at_unix_ms{0};
    SecurityAuditEventType event{SecurityAuditEventType::bootstrap_admin_created};
    std::string actor_user_id;
    std::string subject_user_id;
};

enum class SecurityAuditRepositoryStatus {
    ok,
    error,
};

class SecurityAuditRepository {
public:
    virtual ~SecurityAuditRepository() = default;

    virtual SecurityAuditRepositoryStatus append_security_audit(
        const SecurityAuditRecord& record) = 0;

    virtual SecurityAuditRepositoryStatus list_security_audit(
        std::vector<SecurityAuditRecord>& records) const = 0;
};

}  // namespace dispatcher::users_access
