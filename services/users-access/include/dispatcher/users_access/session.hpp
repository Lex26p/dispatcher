#pragma once

#include "dispatcher/users_access/access.hpp"
#include "dispatcher/users_access/credential.hpp"
#include "dispatcher/users_access/security_audit.hpp"
#include "dispatcher/users_access/user.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dispatcher::users_access {

inline constexpr std::int64_t session_idle_timeout_ms = 30LL * 60LL * 1000LL;
inline constexpr std::int64_t session_absolute_lifetime_ms = 12LL * 60LL * 60LL * 1000LL;
inline constexpr std::size_t session_token_bytes = 32;
inline constexpr std::size_t session_token_digest_bytes = 32;

struct SessionRecord final {
    std::vector<unsigned char> token_digest;
    std::string user_id;
    std::int64_t issued_at_unix_ms{0};
    std::int64_t last_activity_unix_ms{0};
    std::int64_t absolute_expires_at_unix_ms{0};
};

enum class SessionRepositoryStatus {
    ok,
    not_found,
    conflict,
    error,
};

class SessionRepository {
public:
    virtual ~SessionRepository() = default;

    virtual SessionRepositoryStatus insert_session(const SessionRecord& session) = 0;
    virtual SessionRepositoryStatus find_session_by_digest(
        const std::vector<unsigned char>& token_digest,
        SessionRecord& session) const = 0;
    virtual SessionRepositoryStatus update_session_activity(
        const std::vector<unsigned char>& token_digest,
        std::int64_t last_activity_unix_ms) = 0;
    virtual SessionRepositoryStatus erase_session(
        const std::vector<unsigned char>& token_digest) = 0;
};

enum class SessionTokenStatus {
    ok,
    invalid_token,
    crypto_error,
};

class SessionTokenCodec {
public:
    virtual ~SessionTokenCodec() = default;

    virtual SessionTokenStatus generate(
        std::string& token,
        std::vector<unsigned char>& token_digest) const = 0;
    virtual SessionTokenStatus digest(
        std::string_view token,
        std::vector<unsigned char>& token_digest) const = 0;
};

struct AuthenticatedSession final {
    User user;
    std::int64_t issued_at_unix_ms{0};
    std::int64_t absolute_expires_at_unix_ms{0};
    std::int64_t idle_timeout_ms{session_idle_timeout_ms};
};

struct LoginSession final {
    std::string token;
    AuthenticatedSession session;
};

enum class AuthenticationSessionError {
    none,
    invalid_credentials,
    invalid_session,
    session_expired,
    user_disabled,
    storage_error,
    crypto_error,
    session_generation_failed,
};

template <typename T>
struct AuthenticationSessionResult final {
    std::optional<T> value;
    AuthenticationSessionError error{AuthenticationSessionError::none};

    [[nodiscard]] bool ok() const noexcept {
        return value.has_value() && error == AuthenticationSessionError::none;
    }

    [[nodiscard]] static AuthenticationSessionResult success(T result) {
        return AuthenticationSessionResult{
            std::move(result),
            AuthenticationSessionError::none};
    }

    [[nodiscard]] static AuthenticationSessionResult failure(
        const AuthenticationSessionError error) {
        return AuthenticationSessionResult{std::nullopt, error};
    }
};

using SessionClock = std::function<std::int64_t()>;

class UsersAccessManager;
class UsersAccessRepository;

class AuthenticationSessionService final {
public:
    AuthenticationSessionService(
        UsersAccessRepository& users_repository,
        CredentialRepository& credential_repository,
        SessionRepository& session_repository,
        SecurityAuditRepository& audit_repository,
        const PasswordHasher& password_hasher,
        const SessionTokenCodec& token_codec,
        UsersAccessManager& access_manager,
        SessionClock clock = {});

    [[nodiscard]] bool ready() const noexcept;

    [[nodiscard]] AuthenticationSessionResult<LoginSession> login(
        std::string_view login,
        std::string_view password);

    [[nodiscard]] AuthenticationSessionResult<AuthenticatedSession> validate(
        std::string_view token);

    [[nodiscard]] AuthenticationSessionError logout(std::string_view token);

    [[nodiscard]] AuthenticationSessionResult<AccessEvaluation> evaluate_access(
        std::string_view token,
        const AccessScope& scope,
        Capability capability);

private:
    [[nodiscard]] AuthenticationSessionError append_audit(
        SecurityAuditEventType event,
        std::string_view actor_user_id,
        std::string_view subject_user_id);
    [[nodiscard]] AuthenticationSessionError erase_with_audit(
        const SessionRecord& session,
        SecurityAuditEventType event);

    UsersAccessRepository& users_repository_;
    CredentialRepository& credential_repository_;
    SessionRepository& session_repository_;
    SecurityAuditRepository& audit_repository_;
    const PasswordHasher& password_hasher_;
    const SessionTokenCodec& token_codec_;
    UsersAccessManager& access_manager_;
    SessionClock clock_;
    CredentialVerifier dummy_verifier_;
    bool ready_{false};
};

}  // namespace dispatcher::users_access
