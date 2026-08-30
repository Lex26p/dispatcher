#include "dispatcher/users_access/bootstrap.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <random>
#include <utility>

namespace dispatcher::users_access {
namespace {

constexpr std::size_t max_login_bytes = 256;
constexpr std::size_t max_display_name_bytes = 256;
constexpr std::size_t min_bootstrap_password_bytes = 15;
constexpr std::size_t max_password_bytes = 1024;
constexpr int id_generation_attempts = 16;

[[nodiscard]] bool has_non_whitespace(const std::string_view value) noexcept {
    return std::any_of(value.begin(), value.end(), [](const unsigned char character) {
        return std::isspace(character) == 0;
    });
}

[[nodiscard]] std::string default_id() {
    static constexpr char hex[] = "0123456789abcdef";
    std::random_device random;
    std::array<unsigned char, 16> bytes{};

    for (auto& byte : bytes) {
        byte = static_cast<unsigned char>(random());
    }

    std::string result;
    result.reserve(bytes.size() * 2);
    for (const auto byte : bytes) {
        result.push_back(hex[(byte >> 4U) & 0x0FU]);
        result.push_back(hex[byte & 0x0FU]);
    }
    return result;
}

[[nodiscard]] std::int64_t now_unix_ms() noexcept {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

}  // namespace

BootstrapService::BootstrapService(
    BootstrapStore& store,
    const PasswordHasher& password_hasher,
    BootstrapIdGenerator id_generator)
    : store_(store),
      password_hasher_(password_hasher),
      id_generator_(
          id_generator ? std::move(id_generator) : BootstrapIdGenerator{default_id}) {}

BootstrapResult BootstrapService::bootstrap_first_admin(
    const std::string_view login,
    const std::string_view display_name,
    const std::string_view password) {
    if (!has_non_whitespace(login)) {
        return BootstrapResult::failure(BootstrapError::invalid_login);
    }
    if (login.size() > max_login_bytes) {
        return BootstrapResult::failure(BootstrapError::login_too_long);
    }
    if (display_name.size() > max_display_name_bytes) {
        return BootstrapResult::failure(BootstrapError::display_name_too_long);
    }
    if (password.size() < min_bootstrap_password_bytes) {
        return BootstrapResult::failure(BootstrapError::password_too_short);
    }
    if (password.size() > max_password_bytes) {
        return BootstrapResult::failure(BootstrapError::password_too_long);
    }

    CredentialVerifier credential;
    const auto hash_status = password_hasher_.hash(password, credential);
    if (hash_status == PasswordHashStatus::invalid_password) {
        return BootstrapResult::failure(BootstrapError::password_too_long);
    }
    if (hash_status != PasswordHashStatus::ok) {
        return BootstrapResult::failure(BootstrapError::crypto_error);
    }

    for (int attempt = 0; attempt < id_generation_attempts; ++attempt) {
        User user{
            .id = id_generator_(),
            .login = std::string(login),
            .display_name = std::string(display_name),
            .enabled = true,
        };
        PermissionSet permission_set{
            .id = id_generator_(),
            .name = "Bootstrap administrators",
            .capabilities = {
                Capability::view,
                Capability::control,
                Capability::edit,
                Capability::admin,
            },
        };

        if (user.id.empty() || permission_set.id.empty()) {
            continue;
        }

        credential.user_id = user.id;

        BootstrapAdminRecord record{
            .user = user,
            .permission_set = std::move(permission_set),
            .assignment = AccessAssignment{
                .user_id = user.id,
                .permission_set_id = {},
                .scope = AccessScope::global(),
            },
            .credential = credential,
            .audit = SecurityAuditRecord{
                .sequence = 0,
                .occurred_at_unix_ms = now_unix_ms(),
                .event = SecurityAuditEventType::bootstrap_admin_created,
                .actor_user_id = {},
                .subject_user_id = user.id,
            },
        };
        record.assignment.permission_set_id = record.permission_set.id;

        const auto status = store_.bootstrap_first_admin(record);
        if (status == BootstrapStoreStatus::ok) {
            return BootstrapResult::success(std::move(user));
        }
        if (status == BootstrapStoreStatus::already_initialized) {
            return BootstrapResult::failure(BootstrapError::already_initialized);
        }
        if (status == BootstrapStoreStatus::error) {
            return BootstrapResult::failure(BootstrapError::storage_error);
        }
    }

    return BootstrapResult::failure(BootstrapError::id_generation_failed);
}

}  // namespace dispatcher::users_access
