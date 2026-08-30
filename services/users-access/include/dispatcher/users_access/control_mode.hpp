#pragma once

#include "dispatcher/users_access/security_audit.hpp"
#include "dispatcher/users_access/session.hpp"
#include "dispatcher/users_access/users_access_manager.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dispatcher::users_access {

inline constexpr std::int64_t control_mode_lifetime_ms = 10LL * 60LL * 1000LL;

enum class ControlModeReason {
    enabled,
    inactive,
    expired,
    access_revoked,
};

[[nodiscard]] constexpr std::string_view control_mode_reason_name(
    const ControlModeReason reason) noexcept {
    switch (reason) {
    case ControlModeReason::enabled:
        return "enabled";
    case ControlModeReason::inactive:
        return "inactive";
    case ControlModeReason::expired:
        return "expired";
    case ControlModeReason::access_revoked:
        return "access_revoked";
    }
    return "inactive";
}

struct ControlModeState final {
    bool enabled{false};
    ControlModeReason reason{ControlModeReason::inactive};
    std::string project_id;
    std::int64_t expires_at_unix_ms{0};
};

enum class ControlModeError {
    none,
    invalid_project,
    forbidden,
    invalid_session,
    session_expired,
    storage_error,
    crypto_error,
};

struct ControlModeResult final {
    std::optional<ControlModeState> value;
    ControlModeError error{ControlModeError::none};

    [[nodiscard]] bool ok() const noexcept {
        return value.has_value() && error == ControlModeError::none;
    }

    [[nodiscard]] static ControlModeResult success(ControlModeState state) {
        return ControlModeResult{std::move(state), ControlModeError::none};
    }

    [[nodiscard]] static ControlModeResult failure(const ControlModeError error) {
        return ControlModeResult{std::nullopt, error};
    }
};

using ControlModeClock = std::function<std::int64_t()>;

class ControlModeService final {
public:
    ControlModeService(
        AuthenticationSessionService& authentication,
        const SessionTokenCodec& token_codec,
        UsersAccessManager& access_manager,
        SecurityAuditRepository& audit_repository,
        ControlModeClock clock = {});

    [[nodiscard]] ControlModeResult enable(
        std::string_view token,
        std::string_view project_id);

    [[nodiscard]] ControlModeResult current(std::string_view token);

    [[nodiscard]] ControlModeResult disable(std::string_view token);

    void forget(std::string_view token) noexcept;

private:
    struct Entry final {
        std::string project_id;
        std::int64_t expires_at_unix_ms{0};
    };

    [[nodiscard]] ControlModeError digest_key(
        std::string_view token,
        std::vector<unsigned char>& key) const;

    [[nodiscard]] static ControlModeError map_session_error(
        AuthenticationSessionError error) noexcept;

    [[nodiscard]] ControlModeError append_audit(
        SecurityAuditEventType event,
        std::string_view user_id,
        std::int64_t occurred_at_unix_ms);

    [[nodiscard]] static bool valid_project_id(std::string_view project_id) noexcept;

    AuthenticationSessionService& authentication_;
    const SessionTokenCodec& token_codec_;
    UsersAccessManager& access_manager_;
    SecurityAuditRepository& audit_repository_;
    ControlModeClock clock_;
    std::map<std::vector<unsigned char>, Entry> entries_;
};

}  // namespace dispatcher::users_access
