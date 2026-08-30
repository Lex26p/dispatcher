#pragma once

#include "dispatcher/users_access/access.hpp"
#include "dispatcher/users_access/credential.hpp"
#include "dispatcher/users_access/security_audit.hpp"
#include "dispatcher/users_access/user.hpp"
#include "dispatcher/users_access/users_access_repository.hpp"

#include <cstdint>
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
        const CredentialVerifier& verifier,
        const SecurityAuditRecord& audit) = 0;
    virtual AdministrationStoreStatus update_user_enabled(
        const User& user,
        const SecurityAuditRecord& audit) = 0;
    virtual AdministrationStoreStatus set_credential_verifier(
        const CredentialVerifier& verifier,
        const SecurityAuditRecord& audit) = 0;

    virtual AdministrationStoreStatus list_permission_sets(
        std::vector<PermissionSet>& permission_sets) const = 0;
    virtual AdministrationStoreStatus insert_permission_set(
        const PermissionSet& permission_set,
        const SecurityAuditRecord& audit) = 0;

    virtual AdministrationStoreStatus list_assignments(
        std::optional<std::string_view> user_id,
        std::vector<AccessAssignment>& assignments) const = 0;
    virtual AdministrationStoreStatus insert_assignment(
        const AccessAssignment& assignment,
        const SecurityAuditRecord& audit) = 0;
    virtual AdministrationStoreStatus erase_assignment(
        const AccessAssignment& assignment,
        const SecurityAuditRecord& audit) = 0;
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
using AdministrationClock = std::function<std::int64_t()>;

class UsersAccessAdministrationService final {
public:
    UsersAccessAdministrationService(
        UsersAccessRepository& users_repository,
        UsersAccessAdministrationStore& administration_store,
        const PasswordHasher& password_hasher,
        AdministrationIdGenerator id_generator = {},
        AdministrationClock clock = {});

    [[nodiscard]] UsersAccessAdministrationResult<std::vector<User>> list_users() const;
    [[nodiscard]] UsersAccessAdministrationResult<User> create_user(
        std::string_view actor_user_id,
        const CreateAdministrationUserInput& input);
    [[nodiscard]] UsersAccessAdministrationResult<User> set_user_enabled(
        std::string_view actor_user_id,
        std::string_view user_id,
        bool enabled);
    [[nodiscard]] UsersAccessAdministrationError set_user_password(
        std::string_view actor_user_id,
        std::string_view user_id,
        std::string_view password);

    [[nodiscard]] UsersAccessAdministrationResult<std::vector<PermissionSet>>
    list_permission_sets() const;
    [[nodiscard]] UsersAccessAdministrationResult<PermissionSet>
    create_permission_set(
        std::string_view actor_user_id,
        const CreatePermissionSetInput& input);

    [[nodiscard]] UsersAccessAdministrationResult<std::vector<AccessAssignment>>
    list_assignments(std::optional<std::string_view> user_id) const;
    [[nodiscard]] UsersAccessAdministrationResult<AccessAssignment> assign(
        std::string_view actor_user_id,
        const CreateAccessAssignmentInput& input);
    [[nodiscard]] UsersAccessAdministrationError remove_assignment(
        std::string_view actor_user_id,
        const AccessAssignment& assignment);

private:
    [[nodiscard]] static bool has_non_whitespace(std::string_view value) noexcept;
    [[nodiscard]] static bool valid_scope(const AccessScope& scope) noexcept;
    [[nodiscard]] static bool valid_password(std::string_view password) noexcept;
    [[nodiscard]] static bool canonical_capabilities(
        const std::vector<Capability>& capabilities,
        std::vector<Capability>& canonical);

    [[nodiscard]] SecurityAuditRecord audit_record(
        SecurityAuditEventType event,
        std::string_view actor_user_id,
        std::string_view subject_user_id) const;

    UsersAccessRepository& users_repository_;
    UsersAccessAdministrationStore& administration_store_;
    const PasswordHasher& password_hasher_;
    AdministrationIdGenerator id_generator_;
    AdministrationClock clock_;
};

}  // namespace dispatcher::users_access
