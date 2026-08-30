#pragma once

#include "dispatcher/users_access/access.hpp"
#include "dispatcher/users_access/credential.hpp"
#include "dispatcher/users_access/user.hpp"
#include "dispatcher/users_access/users_access_manager.hpp"
#include "dispatcher/users_access/users_access_repository.hpp"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dispatcher::users_access {

enum class AdministrationStoreStatus {
    ok,
    not_found,
    conflict,
    error,
};

class UsersAccessAdministrationStore {
public:
    virtual ~UsersAccessAdministrationStore() = default;

    virtual AdministrationStoreStatus list_users(std::vector<User>& users) const = 0;
    virtual AdministrationStoreStatus insert_user_with_credential(
        const User& user,
        const CredentialVerifier& verifier) = 0;
    virtual AdministrationStoreStatus list_permission_sets(
        std::vector<PermissionSet>& permission_sets) const = 0;
    virtual AdministrationStoreStatus list_assignments(
        std::optional<std::string_view> user_id,
        std::vector<AccessAssignment>& assignments) const = 0;
    virtual AdministrationStoreStatus erase_assignment(
        const AccessAssignment& assignment) = 0;
};

enum class UsersAccessAdministrationError {
    none,
    invalid_login,
    login_too_long,
    display_name_too_long,
    invalid_password,
    password_too_short,
    login_conflict,
    invalid_permission_set_name,
    permission_set_name_too_long,
    invalid_scope,
    invalid_capability,
    user_not_found,
    permission_set_not_found,
    assignment_conflict,
    assignment_not_found,
    storage_error,
    crypto_error,
    id_generation_failed,
};

template <typename T>
struct UsersAccessAdministrationResult final {
    std::optional<T> value;
    UsersAccessAdministrationError error{UsersAccessAdministrationError::none};

    [[nodiscard]] bool ok() const noexcept {
        return value.has_value() && error == UsersAccessAdministrationError::none;
    }

    [[nodiscard]] static UsersAccessAdministrationResult success(T result) {
        return UsersAccessAdministrationResult{
            std::move(result),
            UsersAccessAdministrationError::none};
    }

    [[nodiscard]] static UsersAccessAdministrationResult failure(
        const UsersAccessAdministrationError error) {
        return UsersAccessAdministrationResult{std::nullopt, error};
    }
};

struct CreateAdministrationUserInput final {
    std::string login;
    std::string display_name;
    bool enabled{true};
    std::string password;
};

using AdministrationIdGenerator = std::function<std::string()>;

class UsersAccessAdministrationService final {
public:
    UsersAccessAdministrationService(
        UsersAccessRepository& users_repository,
        CredentialRepository& credential_repository,
        UsersAccessAdministrationStore& administration_store,
        const PasswordHasher& password_hasher,
        UsersAccessManager& access_manager,
        AdministrationIdGenerator id_generator = {});

    [[nodiscard]] UsersAccessAdministrationResult<std::vector<User>> list_users() const;
    [[nodiscard]] UsersAccessAdministrationResult<User> create_user(
        const CreateAdministrationUserInput& input);
    [[nodiscard]] UsersAccessAdministrationResult<User> set_user_enabled(
        std::string_view user_id,
        bool enabled);
    [[nodiscard]] UsersAccessAdministrationError set_user_password(
        std::string_view user_id,
        std::string_view password);

    [[nodiscard]] UsersAccessAdministrationResult<std::vector<PermissionSet>>
    list_permission_sets() const;
    [[nodiscard]] UsersAccessAdministrationResult<PermissionSet>
    create_permission_set(const CreatePermissionSetInput& input);

    [[nodiscard]] UsersAccessAdministrationResult<std::vector<AccessAssignment>>
    list_assignments(std::optional<std::string_view> user_id) const;
    [[nodiscard]] UsersAccessAdministrationResult<AccessAssignment> assign(
        const CreateAccessAssignmentInput& input);
    [[nodiscard]] UsersAccessAdministrationError remove_assignment(
        const AccessAssignment& assignment);

private:
    [[nodiscard]] static UsersAccessAdministrationError map_manager_error(
        UsersAccessManagerError error) noexcept;
    [[nodiscard]] static bool has_non_whitespace(std::string_view value) noexcept;
    [[nodiscard]] static bool valid_scope(const AccessScope& scope) noexcept;
    [[nodiscard]] static bool valid_password(std::string_view password) noexcept;

    UsersAccessRepository& users_repository_;
    CredentialRepository& credential_repository_;
    UsersAccessAdministrationStore& administration_store_;
    const PasswordHasher& password_hasher_;
    UsersAccessManager& access_manager_;
    AdministrationIdGenerator id_generator_;
};

}  // namespace dispatcher::users_access
