#include "dispatcher/users_access/administration.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <random>
#include <utility>

namespace dispatcher::users_access {
namespace {

constexpr std::size_t max_login_bytes = 256;
constexpr std::size_t max_display_name_bytes = 256;
constexpr std::size_t min_password_bytes = 15;
constexpr std::size_t max_password_bytes = 1024;
constexpr int id_generation_attempts = 16;

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

}  // namespace

UsersAccessAdministrationService::UsersAccessAdministrationService(
    UsersAccessRepository& users_repository,
    CredentialRepository& credential_repository,
    UsersAccessAdministrationStore& administration_store,
    const PasswordHasher& password_hasher,
    UsersAccessManager& access_manager,
    AdministrationIdGenerator id_generator)
    : users_repository_(users_repository),
      credential_repository_(credential_repository),
      administration_store_(administration_store),
      password_hasher_(password_hasher),
      access_manager_(access_manager),
      id_generator_(id_generator ? std::move(id_generator) : AdministrationIdGenerator{default_id}) {}

UsersAccessAdministrationResult<std::vector<User>>
UsersAccessAdministrationService::list_users() const {
    std::vector<User> users;
    if (administration_store_.list_users(users) != AdministrationStoreStatus::ok) {
        return UsersAccessAdministrationResult<std::vector<User>>::failure(
            UsersAccessAdministrationError::storage_error);
    }
    return UsersAccessAdministrationResult<std::vector<User>>::success(
        std::move(users));
}

UsersAccessAdministrationResult<User> UsersAccessAdministrationService::create_user(
    const CreateAdministrationUserInput& input) {
    if (!has_non_whitespace(input.login)) {
        return UsersAccessAdministrationResult<User>::failure(
            UsersAccessAdministrationError::invalid_login);
    }
    if (input.login.size() > max_login_bytes) {
        return UsersAccessAdministrationResult<User>::failure(
            UsersAccessAdministrationError::login_too_long);
    }
    if (input.display_name.size() > max_display_name_bytes) {
        return UsersAccessAdministrationResult<User>::failure(
            UsersAccessAdministrationError::display_name_too_long);
    }
    if (!valid_password(input.password)) {
        return UsersAccessAdministrationResult<User>::failure(
            input.password.size() < min_password_bytes
                ? UsersAccessAdministrationError::password_too_short
                : UsersAccessAdministrationError::invalid_password);
    }

    User existing;
    const auto existing_status = users_repository_.find_user_by_login(
        input.login,
        existing);
    if (existing_status == UsersAccessRepositoryStatus::ok) {
        return UsersAccessAdministrationResult<User>::failure(
            UsersAccessAdministrationError::login_conflict);
    }
    if (existing_status != UsersAccessRepositoryStatus::not_found) {
        return UsersAccessAdministrationResult<User>::failure(
            UsersAccessAdministrationError::storage_error);
    }

    for (int attempt = 0; attempt < id_generation_attempts; ++attempt) {
        User user{
            .id = id_generator_(),
            .login = input.login,
            .display_name = input.display_name,
            .enabled = input.enabled,
        };
        if (user.id.empty()) {
            continue;
        }

        CredentialVerifier verifier;
        verifier.user_id = user.id;
        const auto hash_status = password_hasher_.hash(input.password, verifier);
        if (hash_status == PasswordHashStatus::invalid_password) {
            return UsersAccessAdministrationResult<User>::failure(
                UsersAccessAdministrationError::invalid_password);
        }
        if (hash_status != PasswordHashStatus::ok) {
            return UsersAccessAdministrationResult<User>::failure(
                UsersAccessAdministrationError::crypto_error);
        }

        const auto store_status = administration_store_.insert_user_with_credential(
            user,
            verifier);
        if (store_status == AdministrationStoreStatus::ok) {
            return UsersAccessAdministrationResult<User>::success(std::move(user));
        }
        if (store_status == AdministrationStoreStatus::error) {
            return UsersAccessAdministrationResult<User>::failure(
                UsersAccessAdministrationError::storage_error);
        }
        if (store_status == AdministrationStoreStatus::not_found) {
            return UsersAccessAdministrationResult<User>::failure(
                UsersAccessAdministrationError::storage_error);
        }

        const auto login_status = users_repository_.find_user_by_login(
            input.login,
            existing);
        if (login_status == UsersAccessRepositoryStatus::ok) {
            return UsersAccessAdministrationResult<User>::failure(
                UsersAccessAdministrationError::login_conflict);
        }
        if (login_status != UsersAccessRepositoryStatus::not_found) {
            return UsersAccessAdministrationResult<User>::failure(
                UsersAccessAdministrationError::storage_error);
        }
    }

    return UsersAccessAdministrationResult<User>::failure(
        UsersAccessAdministrationError::id_generation_failed);
}

UsersAccessAdministrationResult<User>
UsersAccessAdministrationService::set_user_enabled(
    const std::string_view user_id,
    const bool enabled) {
    auto result = access_manager_.set_user_enabled(user_id, enabled);
    if (!result.ok()) {
        return UsersAccessAdministrationResult<User>::failure(
            map_manager_error(result.error));
    }
    return UsersAccessAdministrationResult<User>::success(
        std::move(*result.value));
}

UsersAccessAdministrationError UsersAccessAdministrationService::set_user_password(
    const std::string_view user_id,
    const std::string_view password) {
    if (!valid_password(password)) {
        return password.size() < min_password_bytes
            ? UsersAccessAdministrationError::password_too_short
            : UsersAccessAdministrationError::invalid_password;
    }

    User user;
    const auto user_status = users_repository_.find_user_by_id(user_id, user);
    if (user_status == UsersAccessRepositoryStatus::not_found) {
        return UsersAccessAdministrationError::user_not_found;
    }
    if (user_status != UsersAccessRepositoryStatus::ok) {
        return UsersAccessAdministrationError::storage_error;
    }

    CredentialVerifier verifier;
    verifier.user_id = user.id;
    const auto hash_status = password_hasher_.hash(password, verifier);
    if (hash_status == PasswordHashStatus::invalid_password) {
        return UsersAccessAdministrationError::invalid_password;
    }
    if (hash_status != PasswordHashStatus::ok) {
        return UsersAccessAdministrationError::crypto_error;
    }

    return credential_repository_.set_credential_verifier(verifier) ==
            CredentialRepositoryStatus::ok
        ? UsersAccessAdministrationError::none
        : UsersAccessAdministrationError::storage_error;
}

UsersAccessAdministrationResult<std::vector<PermissionSet>>
UsersAccessAdministrationService::list_permission_sets() const {
    std::vector<PermissionSet> permission_sets;
    if (administration_store_.list_permission_sets(permission_sets) !=
        AdministrationStoreStatus::ok) {
        return UsersAccessAdministrationResult<std::vector<PermissionSet>>::failure(
            UsersAccessAdministrationError::storage_error);
    }
    return UsersAccessAdministrationResult<std::vector<PermissionSet>>::success(
        std::move(permission_sets));
}

UsersAccessAdministrationResult<PermissionSet>
UsersAccessAdministrationService::create_permission_set(
    const CreatePermissionSetInput& input) {
    auto result = access_manager_.create_permission_set(input);
    if (!result.ok()) {
        return UsersAccessAdministrationResult<PermissionSet>::failure(
            map_manager_error(result.error));
    }
    return UsersAccessAdministrationResult<PermissionSet>::success(
        std::move(*result.value));
}

UsersAccessAdministrationResult<std::vector<AccessAssignment>>
UsersAccessAdministrationService::list_assignments(
    const std::optional<std::string_view> user_id) const {
    if (user_id.has_value()) {
        User user;
        const auto user_status = users_repository_.find_user_by_id(*user_id, user);
        if (user_status == UsersAccessRepositoryStatus::not_found) {
            return UsersAccessAdministrationResult<std::vector<AccessAssignment>>::failure(
                UsersAccessAdministrationError::user_not_found);
        }
        if (user_status != UsersAccessRepositoryStatus::ok) {
            return UsersAccessAdministrationResult<std::vector<AccessAssignment>>::failure(
                UsersAccessAdministrationError::storage_error);
        }
    }

    std::vector<AccessAssignment> assignments;
    if (administration_store_.list_assignments(user_id, assignments) !=
        AdministrationStoreStatus::ok) {
        return UsersAccessAdministrationResult<std::vector<AccessAssignment>>::failure(
            UsersAccessAdministrationError::storage_error);
    }
    return UsersAccessAdministrationResult<std::vector<AccessAssignment>>::success(
        std::move(assignments));
}

UsersAccessAdministrationResult<AccessAssignment>
UsersAccessAdministrationService::assign(
    const CreateAccessAssignmentInput& input) {
    auto result = access_manager_.assign(input);
    if (!result.ok()) {
        return UsersAccessAdministrationResult<AccessAssignment>::failure(
            map_manager_error(result.error));
    }
    return UsersAccessAdministrationResult<AccessAssignment>::success(
        std::move(*result.value));
}

UsersAccessAdministrationError UsersAccessAdministrationService::remove_assignment(
    const AccessAssignment& assignment) {
    if (!valid_scope(assignment.scope)) {
        return UsersAccessAdministrationError::invalid_scope;
    }

    User user;
    const auto user_status = users_repository_.find_user_by_id(
        assignment.user_id,
        user);
    if (user_status == UsersAccessRepositoryStatus::not_found) {
        return UsersAccessAdministrationError::user_not_found;
    }
    if (user_status != UsersAccessRepositoryStatus::ok) {
        return UsersAccessAdministrationError::storage_error;
    }

    PermissionSet permission_set;
    const auto permission_status = users_repository_.find_permission_set_by_id(
        assignment.permission_set_id,
        permission_set);
    if (permission_status == UsersAccessRepositoryStatus::not_found) {
        return UsersAccessAdministrationError::permission_set_not_found;
    }
    if (permission_status != UsersAccessRepositoryStatus::ok) {
        return UsersAccessAdministrationError::storage_error;
    }

    const auto status = administration_store_.erase_assignment(assignment);
    if (status == AdministrationStoreStatus::ok) {
        return UsersAccessAdministrationError::none;
    }
    if (status == AdministrationStoreStatus::not_found) {
        return UsersAccessAdministrationError::assignment_not_found;
    }
    return UsersAccessAdministrationError::storage_error;
}

UsersAccessAdministrationError UsersAccessAdministrationService::map_manager_error(
    const UsersAccessManagerError error) noexcept {
    switch (error) {
    case UsersAccessManagerError::none:
        return UsersAccessAdministrationError::none;
    case UsersAccessManagerError::invalid_login:
        return UsersAccessAdministrationError::invalid_login;
    case UsersAccessManagerError::login_too_long:
        return UsersAccessAdministrationError::login_too_long;
    case UsersAccessManagerError::display_name_too_long:
        return UsersAccessAdministrationError::display_name_too_long;
    case UsersAccessManagerError::login_conflict:
        return UsersAccessAdministrationError::login_conflict;
    case UsersAccessManagerError::invalid_permission_set_name:
        return UsersAccessAdministrationError::invalid_permission_set_name;
    case UsersAccessManagerError::permission_set_name_too_long:
        return UsersAccessAdministrationError::permission_set_name_too_long;
    case UsersAccessManagerError::invalid_scope:
        return UsersAccessAdministrationError::invalid_scope;
    case UsersAccessManagerError::user_not_found:
        return UsersAccessAdministrationError::user_not_found;
    case UsersAccessManagerError::permission_set_not_found:
        return UsersAccessAdministrationError::permission_set_not_found;
    case UsersAccessManagerError::assignment_conflict:
        return UsersAccessAdministrationError::assignment_conflict;
    case UsersAccessManagerError::storage_error:
        return UsersAccessAdministrationError::storage_error;
    case UsersAccessManagerError::id_generation_failed:
        return UsersAccessAdministrationError::id_generation_failed;
    }
    return UsersAccessAdministrationError::storage_error;
}

bool UsersAccessAdministrationService::has_non_whitespace(
    const std::string_view value) noexcept {
    return std::any_of(
        value.begin(),
        value.end(),
        [](const unsigned char character) {
            return std::isspace(character) == 0;
        });
}

bool UsersAccessAdministrationService::valid_scope(
    const AccessScope& scope) noexcept {
    if (scope.kind == AccessScopeKind::global) {
        return scope.project_id.empty();
    }
    return has_non_whitespace(scope.project_id);
}

bool UsersAccessAdministrationService::valid_password(
    const std::string_view password) noexcept {
    return password.size() >= min_password_bytes &&
           password.size() <= max_password_bytes;
}

}  // namespace dispatcher::users_access
