#pragma once

#include "dispatcher/users_access/access.hpp"
#include "dispatcher/users_access/credential.hpp"
#include "dispatcher/users_access/security_audit.hpp"
#include "dispatcher/users_access/user.hpp"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace dispatcher::users_access {

struct BootstrapAdminRecord final {
    User user;
    PermissionSet permission_set;
    AccessAssignment assignment;
    CredentialVerifier credential;
    SecurityAuditRecord audit;
};

enum class BootstrapStoreStatus {
    ok,
    already_initialized,
    conflict,
    error,
};

class BootstrapStore {
public:
    virtual ~BootstrapStore() = default;

    virtual BootstrapStoreStatus bootstrap_first_admin(
        const BootstrapAdminRecord& record) = 0;
};

enum class BootstrapError {
    none,
    invalid_login,
    login_too_long,
    display_name_too_long,
    password_too_short,
    password_too_long,
    already_initialized,
    crypto_error,
    storage_error,
    id_generation_failed,
};

struct BootstrapResult final {
    std::optional<User> user;
    BootstrapError error{BootstrapError::none};

    [[nodiscard]] bool ok() const noexcept {
        return user.has_value() && error == BootstrapError::none;
    }

    [[nodiscard]] static BootstrapResult success(User value) {
        return BootstrapResult{std::move(value), BootstrapError::none};
    }

    [[nodiscard]] static BootstrapResult failure(const BootstrapError value) {
        return BootstrapResult{std::nullopt, value};
    }
};

using BootstrapIdGenerator = std::function<std::string()>;

class BootstrapService final {
public:
    BootstrapService(
        BootstrapStore& store,
        const PasswordHasher& password_hasher,
        BootstrapIdGenerator id_generator = {});

    [[nodiscard]] BootstrapResult bootstrap_first_admin(
        std::string_view login,
        std::string_view display_name,
        std::string_view password);

private:
    BootstrapStore& store_;
    const PasswordHasher& password_hasher_;
    BootstrapIdGenerator id_generator_;
};

}  // namespace dispatcher::users_access
