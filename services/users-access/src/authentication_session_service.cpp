#include "dispatcher/users_access/session.hpp"

#include "dispatcher/users_access/users_access_manager.hpp"
#include "dispatcher/users_access/users_access_repository.hpp"

#include <chrono>
#include <limits>
#include <string>
#include <utility>

namespace dispatcher::users_access {
namespace {

constexpr int session_generation_attempts = 16;
constexpr std::string_view dummy_password =
    "dispatcher-authentication-timing-dummy-verifier";

[[nodiscard]] std::int64_t system_clock_unix_ms() noexcept {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

[[nodiscard]] AuthenticationSessionError map_token_status(
    const SessionTokenStatus status) noexcept {
    switch (status) {
    case SessionTokenStatus::ok:
        return AuthenticationSessionError::none;
    case SessionTokenStatus::invalid_token:
        return AuthenticationSessionError::invalid_session;
    case SessionTokenStatus::crypto_error:
        return AuthenticationSessionError::crypto_error;
    }
    return AuthenticationSessionError::crypto_error;
}

}  // namespace

AuthenticationSessionService::AuthenticationSessionService(
    UsersAccessRepository& users_repository,
    CredentialRepository& credential_repository,
    SessionRepository& session_repository,
    SecurityAuditRepository& audit_repository,
    const PasswordHasher& password_hasher,
    const SessionTokenCodec& token_codec,
    UsersAccessManager& access_manager,
    SessionClock clock)
    : users_repository_(users_repository),
      credential_repository_(credential_repository),
      session_repository_(session_repository),
      audit_repository_(audit_repository),
      password_hasher_(password_hasher),
      token_codec_(token_codec),
      access_manager_(access_manager),
      clock_(clock ? std::move(clock) : SessionClock{system_clock_unix_ms}) {
    dummy_verifier_.user_id.clear();
    ready_ = password_hasher_.hash(dummy_password, dummy_verifier_) ==
        PasswordHashStatus::ok;
}

bool AuthenticationSessionService::ready() const noexcept {
    return ready_;
}

AuthenticationSessionResult<LoginSession> AuthenticationSessionService::login(
    const std::string_view login,
    const std::string_view password) {
    if (!ready_) {
        return AuthenticationSessionResult<LoginSession>::failure(
            AuthenticationSessionError::crypto_error);
    }

    User user;
    const auto user_status = users_repository_.find_user_by_login(login, user);
    if (user_status == UsersAccessRepositoryStatus::error ||
        user_status == UsersAccessRepositoryStatus::conflict) {
        return AuthenticationSessionResult<LoginSession>::failure(
            AuthenticationSessionError::storage_error);
    }

    CredentialVerifier verifier;
    bool use_dummy = user_status == UsersAccessRepositoryStatus::not_found;
    if (!use_dummy) {
        const auto credential_status =
            credential_repository_.find_credential_verifier(user.id, verifier);
        if (credential_status == CredentialRepositoryStatus::error ||
            credential_status == CredentialRepositoryStatus::conflict) {
            return AuthenticationSessionResult<LoginSession>::failure(
                AuthenticationSessionError::storage_error);
        }
        use_dummy = credential_status == CredentialRepositoryStatus::not_found;
    }

    bool matches = false;
    const auto verify_status = password_hasher_.verify(
        password,
        use_dummy ? dummy_verifier_ : verifier,
        matches);
    if (verify_status == PasswordHashStatus::invalid_password) {
        const auto audit_error = append_audit(
            SecurityAuditEventType::authentication_failed,
            {},
            use_dummy ? std::string_view{} : std::string_view{user.id});
        if (audit_error != AuthenticationSessionError::none) {
            return AuthenticationSessionResult<LoginSession>::failure(audit_error);
        }
        return AuthenticationSessionResult<LoginSession>::failure(
            AuthenticationSessionError::invalid_credentials);
    }
    if (verify_status == PasswordHashStatus::unsupported_verifier) {
        return AuthenticationSessionResult<LoginSession>::failure(
            AuthenticationSessionError::storage_error);
    }
    if (verify_status != PasswordHashStatus::ok) {
        return AuthenticationSessionResult<LoginSession>::failure(
            AuthenticationSessionError::crypto_error);
    }

    if (use_dummy || !matches || !user.enabled) {
        const auto audit_error = append_audit(
            SecurityAuditEventType::authentication_failed,
            {},
            use_dummy ? std::string_view{} : std::string_view{user.id});
        if (audit_error != AuthenticationSessionError::none) {
            return AuthenticationSessionResult<LoginSession>::failure(audit_error);
        }
        return AuthenticationSessionResult<LoginSession>::failure(
            AuthenticationSessionError::invalid_credentials);
    }

    const std::int64_t now = clock_();
    if (now < 0 ||
        now > (std::numeric_limits<std::int64_t>::max() - session_absolute_lifetime_ms)) {
        return AuthenticationSessionResult<LoginSession>::failure(
            AuthenticationSessionError::storage_error);
    }

    for (int attempt = 0; attempt < session_generation_attempts; ++attempt) {
        std::string token;
        std::vector<unsigned char> digest;
        const auto token_status = token_codec_.generate(token, digest);
        if (token_status != SessionTokenStatus::ok) {
            return AuthenticationSessionResult<LoginSession>::failure(
                map_token_status(token_status));
        }

        SessionRecord record{
            .token_digest = std::move(digest),
            .user_id = user.id,
            .issued_at_unix_ms = now,
            .last_activity_unix_ms = now,
            .absolute_expires_at_unix_ms = now + session_absolute_lifetime_ms,
        };

        const auto insert_status = session_repository_.insert_session(record);
        if (insert_status == SessionRepositoryStatus::conflict) {
            continue;
        }
        if (insert_status != SessionRepositoryStatus::ok) {
            return AuthenticationSessionResult<LoginSession>::failure(
                AuthenticationSessionError::storage_error);
        }

        const auto audit_error = append_audit(
            SecurityAuditEventType::authentication_succeeded,
            user.id,
            user.id);
        if (audit_error != AuthenticationSessionError::none) {
            (void)session_repository_.erase_session(record.token_digest);
            return AuthenticationSessionResult<LoginSession>::failure(audit_error);
        }

        return AuthenticationSessionResult<LoginSession>::success(LoginSession{
            .token = std::move(token),
            .session = AuthenticatedSession{
                .user = user,
                .issued_at_unix_ms = now,
                .absolute_expires_at_unix_ms = record.absolute_expires_at_unix_ms,
                .idle_timeout_ms = session_idle_timeout_ms,
            },
        });
    }

    return AuthenticationSessionResult<LoginSession>::failure(
        AuthenticationSessionError::session_generation_failed);
}

AuthenticationSessionResult<AuthenticatedSession>
AuthenticationSessionService::validate(const std::string_view token) {
    if (!ready_) {
        return AuthenticationSessionResult<AuthenticatedSession>::failure(
            AuthenticationSessionError::crypto_error);
    }

    std::vector<unsigned char> digest;
    const auto digest_status = token_codec_.digest(token, digest);
    if (digest_status != SessionTokenStatus::ok) {
        return AuthenticationSessionResult<AuthenticatedSession>::failure(
            map_token_status(digest_status));
    }

    SessionRecord record;
    const auto find_status = session_repository_.find_session_by_digest(digest, record);
    if (find_status == SessionRepositoryStatus::not_found) {
        return AuthenticationSessionResult<AuthenticatedSession>::failure(
            AuthenticationSessionError::invalid_session);
    }
    if (find_status != SessionRepositoryStatus::ok) {
        return AuthenticationSessionResult<AuthenticatedSession>::failure(
            AuthenticationSessionError::storage_error);
    }

    const std::int64_t now = clock_();
    if (now < record.issued_at_unix_ms ||
        now < record.last_activity_unix_ms ||
        record.last_activity_unix_ms < record.issued_at_unix_ms ||
        record.absolute_expires_at_unix_ms < record.issued_at_unix_ms) {
        (void)session_repository_.erase_session(record.token_digest);
        return AuthenticationSessionResult<AuthenticatedSession>::failure(
            AuthenticationSessionError::storage_error);
    }

    const bool absolute_expired = now >= record.absolute_expires_at_unix_ms;
    const bool idle_expired =
        now - record.last_activity_unix_ms >= session_idle_timeout_ms;
    if (absolute_expired || idle_expired) {
        const auto erase_error = erase_with_audit(
            record,
            SecurityAuditEventType::session_expired);
        if (erase_error != AuthenticationSessionError::none) {
            return AuthenticationSessionResult<AuthenticatedSession>::failure(
                erase_error);
        }
        return AuthenticationSessionResult<AuthenticatedSession>::failure(
            AuthenticationSessionError::session_expired);
    }

    User user;
    const auto user_status = users_repository_.find_user_by_id(record.user_id, user);
    if (user_status != UsersAccessRepositoryStatus::ok) {
        (void)session_repository_.erase_session(record.token_digest);
        return AuthenticationSessionResult<AuthenticatedSession>::failure(
            AuthenticationSessionError::storage_error);
    }
    if (!user.enabled) {
        const auto erase_error = erase_with_audit(
            record,
            SecurityAuditEventType::session_rejected_disabled_user);
        if (erase_error != AuthenticationSessionError::none) {
            return AuthenticationSessionResult<AuthenticatedSession>::failure(
                erase_error);
        }
        return AuthenticationSessionResult<AuthenticatedSession>::failure(
            AuthenticationSessionError::user_disabled);
    }

    if (session_repository_.update_session_activity(record.token_digest, now) !=
        SessionRepositoryStatus::ok) {
        return AuthenticationSessionResult<AuthenticatedSession>::failure(
            AuthenticationSessionError::storage_error);
    }

    return AuthenticationSessionResult<AuthenticatedSession>::success(
        AuthenticatedSession{
            .user = std::move(user),
            .issued_at_unix_ms = record.issued_at_unix_ms,
            .absolute_expires_at_unix_ms = record.absolute_expires_at_unix_ms,
            .idle_timeout_ms = session_idle_timeout_ms,
        });
}

AuthenticationSessionError AuthenticationSessionService::logout(
    const std::string_view token) {
    std::vector<unsigned char> digest;
    const auto digest_status = token_codec_.digest(token, digest);
    if (digest_status != SessionTokenStatus::ok) {
        return map_token_status(digest_status);
    }

    SessionRecord record;
    const auto find_status = session_repository_.find_session_by_digest(digest, record);
    if (find_status == SessionRepositoryStatus::not_found) {
        return AuthenticationSessionError::invalid_session;
    }
    if (find_status != SessionRepositoryStatus::ok) {
        return AuthenticationSessionError::storage_error;
    }

    return erase_with_audit(record, SecurityAuditEventType::session_logged_out);
}

AuthenticationSessionResult<AccessEvaluation>
AuthenticationSessionService::evaluate_access(
    const std::string_view token,
    const AccessScope& scope,
    const Capability capability) {
    const auto session = validate(token);
    if (!session.ok()) {
        return AuthenticationSessionResult<AccessEvaluation>::failure(session.error);
    }

    const auto evaluation = access_manager_.evaluate(
        session.value->user.id,
        scope,
        capability);
    if (!evaluation.ok()) {
        return AuthenticationSessionResult<AccessEvaluation>::failure(
            AuthenticationSessionError::storage_error);
    }
    return AuthenticationSessionResult<AccessEvaluation>::success(evaluation);
}

AuthenticationSessionError AuthenticationSessionService::append_audit(
    const SecurityAuditEventType event,
    const std::string_view actor_user_id,
    const std::string_view subject_user_id) {
    const SecurityAuditRecord record{
        .sequence = 0,
        .occurred_at_unix_ms = clock_(),
        .event = event,
        .actor_user_id = std::string(actor_user_id),
        .subject_user_id = std::string(subject_user_id),
    };
    return audit_repository_.append_security_audit(record) ==
            SecurityAuditRepositoryStatus::ok
        ? AuthenticationSessionError::none
        : AuthenticationSessionError::storage_error;
}

AuthenticationSessionError AuthenticationSessionService::erase_with_audit(
    const SessionRecord& session,
    const SecurityAuditEventType event) {
    if (session_repository_.erase_session(session.token_digest) !=
        SessionRepositoryStatus::ok) {
        return AuthenticationSessionError::storage_error;
    }
    return append_audit(event, session.user_id, session.user_id);
}

}  // namespace dispatcher::users_access
