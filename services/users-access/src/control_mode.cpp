#include "dispatcher/users_access/control_mode.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <limits>
#include <utility>

namespace dispatcher::users_access {
namespace {

[[nodiscard]] std::int64_t system_clock_unix_ms() noexcept {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

[[nodiscard]] ControlModeState inactive_state(
    const ControlModeReason reason = ControlModeReason::inactive) {
    return ControlModeState{
        .enabled = false,
        .reason = reason,
        .project_id = {},
        .expires_at_unix_ms = 0,
    };
}

}  // namespace

ControlModeService::ControlModeService(
    AuthenticationSessionService& authentication,
    const SessionTokenCodec& token_codec,
    UsersAccessManager& access_manager,
    ControlModeClock clock)
    : authentication_(authentication),
      token_codec_(token_codec),
      access_manager_(access_manager),
      clock_(clock ? std::move(clock) : ControlModeClock{system_clock_unix_ms}) {}

ControlModeResult ControlModeService::enable(
    const std::string_view token,
    const std::string_view project_id) {
    if (!valid_project_id(project_id)) {
        return ControlModeResult::failure(ControlModeError::invalid_project);
    }

    std::vector<unsigned char> key;
    const auto key_error = digest_key(token, key);
    if (key_error != ControlModeError::none) {
        return ControlModeResult::failure(key_error);
    }

    auto session = authentication_.validate(token);
    if (!session.ok()) {
        entries_.erase(key);
        return ControlModeResult::failure(map_session_error(session.error));
    }

    const auto evaluation = access_manager_.evaluate(
        session.value->user.id,
        AccessScope::project(std::string(project_id)),
        Capability::control);
    if (!evaluation.ok()) {
        entries_.erase(key);
        return ControlModeResult::failure(ControlModeError::storage_error);
    }
    if (!evaluation.allowed) {
        entries_.erase(key);
        return ControlModeResult::failure(ControlModeError::forbidden);
    }

    const std::int64_t now = clock_();
    if (now < 0 ||
        now > std::numeric_limits<std::int64_t>::max() - control_mode_lifetime_ms) {
        entries_.erase(key);
        return ControlModeResult::failure(ControlModeError::storage_error);
    }

    const std::int64_t expires_at = std::min(
        now + control_mode_lifetime_ms,
        session.value->absolute_expires_at_unix_ms);
    if (expires_at <= now) {
        entries_.erase(key);
        return ControlModeResult::failure(ControlModeError::session_expired);
    }

    entries_[key] = Entry{
        .project_id = std::string(project_id),
        .expires_at_unix_ms = expires_at,
    };

    return ControlModeResult::success(ControlModeState{
        .enabled = true,
        .reason = ControlModeReason::enabled,
        .project_id = std::string(project_id),
        .expires_at_unix_ms = expires_at,
    });
}

ControlModeResult ControlModeService::current(const std::string_view token) {
    std::vector<unsigned char> key;
    const auto key_error = digest_key(token, key);
    if (key_error != ControlModeError::none) {
        return ControlModeResult::failure(key_error);
    }

    auto session = authentication_.validate(token);
    if (!session.ok()) {
        entries_.erase(key);
        return ControlModeResult::failure(map_session_error(session.error));
    }

    const auto iterator = entries_.find(key);
    if (iterator == entries_.end()) {
        return ControlModeResult::success(inactive_state());
    }

    const std::int64_t now = clock_();
    if (now < 0) {
        return ControlModeResult::failure(ControlModeError::storage_error);
    }
    if (now >= iterator->second.expires_at_unix_ms) {
        entries_.erase(iterator);
        return ControlModeResult::success(inactive_state(ControlModeReason::expired));
    }

    const auto evaluation = access_manager_.evaluate(
        session.value->user.id,
        AccessScope::project(iterator->second.project_id),
        Capability::control);
    if (!evaluation.ok()) {
        entries_.erase(iterator);
        return ControlModeResult::failure(ControlModeError::storage_error);
    }
    if (!evaluation.allowed) {
        entries_.erase(iterator);
        return ControlModeResult::success(
            inactive_state(ControlModeReason::access_revoked));
    }

    return ControlModeResult::success(ControlModeState{
        .enabled = true,
        .reason = ControlModeReason::enabled,
        .project_id = iterator->second.project_id,
        .expires_at_unix_ms = iterator->second.expires_at_unix_ms,
    });
}

ControlModeResult ControlModeService::disable(const std::string_view token) {
    std::vector<unsigned char> key;
    const auto key_error = digest_key(token, key);
    if (key_error != ControlModeError::none) {
        return ControlModeResult::failure(key_error);
    }

    auto session = authentication_.validate(token);
    if (!session.ok()) {
        entries_.erase(key);
        return ControlModeResult::failure(map_session_error(session.error));
    }

    entries_.erase(key);
    return ControlModeResult::success(inactive_state());
}

void ControlModeService::forget(const std::string_view token) noexcept {
    std::vector<unsigned char> key;
    if (digest_key(token, key) == ControlModeError::none) {
        entries_.erase(key);
    }
}

ControlModeError ControlModeService::digest_key(
    const std::string_view token,
    std::vector<unsigned char>& key) const {
    switch (token_codec_.digest(token, key)) {
    case SessionTokenStatus::ok:
        return ControlModeError::none;
    case SessionTokenStatus::invalid_token:
        return ControlModeError::invalid_session;
    case SessionTokenStatus::crypto_error:
        return ControlModeError::crypto_error;
    }
    return ControlModeError::crypto_error;
}

ControlModeError ControlModeService::map_session_error(
    const AuthenticationSessionError error) noexcept {
    switch (error) {
    case AuthenticationSessionError::none:
        return ControlModeError::none;
    case AuthenticationSessionError::invalid_credentials:
    case AuthenticationSessionError::invalid_session:
    case AuthenticationSessionError::user_disabled:
        return ControlModeError::invalid_session;
    case AuthenticationSessionError::session_expired:
        return ControlModeError::session_expired;
    case AuthenticationSessionError::storage_error:
        return ControlModeError::storage_error;
    case AuthenticationSessionError::crypto_error:
    case AuthenticationSessionError::session_generation_failed:
        return ControlModeError::crypto_error;
    }
    return ControlModeError::storage_error;
}

bool ControlModeService::valid_project_id(
    const std::string_view project_id) noexcept {
    return std::any_of(
        project_id.begin(),
        project_id.end(),
        [](const unsigned char character) {
            return std::isspace(character) == 0;
        });
}

}  // namespace dispatcher::users_access
