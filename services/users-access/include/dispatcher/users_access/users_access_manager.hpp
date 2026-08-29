#pragma once

#include "dispatcher/users_access/access.hpp"
#include "dispatcher/users_access/user.hpp"
#include "dispatcher/users_access/users_access_repository.hpp"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace dispatcher::users_access {

enum class UsersAccessManagerError {
    none,
    invalid_login,
    login_too_long,
    display_name_too_long,
    login_conflict,
    invalid_permission_set_name,
    permission_set_name_too_long,
    invalid_scope,
    user_not_found,
    permission_set_not_found,
    assignment_conflict,
    storage_error,
    id_generation_failed,
};

template <typename T>
struct UsersAccessManagerResult final {
    std::optional<T> value;
    UsersAccessManagerError error{UsersAccessManagerError::none};

    [[nodiscard]] bool ok() const noexcept {
        return value.has_value() && error == UsersAccessManagerError::none;
    }

    [[nodiscard]] static UsersAccessManagerResult success(T result) {
        return UsersAccessManagerResult{
            std::move(result),
            UsersAccessManagerError::none};
    }

    [[nodiscard]] static UsersAccessManagerResult failure(
        const UsersAccessManagerError error) {
        return UsersAccessManagerResult{std::nullopt, error};
    }
};

using UsersAccessIdGenerator = std::function<std::string()>;

class UsersAccessManager final {
public:
    explicit UsersAccessManager(
        UsersAccessRepository& repository,
        UsersAccessIdGenerator id_generator = {});

    [[nodiscard]] UsersAccessManagerResult<User> create_user(
        const CreateUserInput& input);

    [[nodiscard]] UsersAccessManagerResult<PermissionSet> create_permission_set(
        const CreatePermissionSetInput& input);

    [[nodiscard]] UsersAccessManagerResult<AccessAssignment> assign(
        const CreateAccessAssignmentInput& input);

    [[nodiscard]] AccessEvaluation evaluate(
        std::string_view user_id,
        const AccessScope& scope,
        Capability required_capability) const;

private:
    [[nodiscard]] static bool valid_scope(const AccessScope& scope) noexcept;
    [[nodiscard]] static bool has_non_whitespace(std::string_view value) noexcept;
    [[nodiscard]] static std::vector<Capability> canonical_capabilities(
        const std::vector<Capability>& capabilities);

    UsersAccessRepository& repository_;
    UsersAccessIdGenerator id_generator_;
};

}  // namespace dispatcher::users_access
