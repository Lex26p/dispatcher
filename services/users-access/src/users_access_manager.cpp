#include "dispatcher/users_access/users_access_manager.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <random>
#include <utility>

namespace dispatcher::users_access {
namespace {

constexpr std::size_t max_login_bytes = 256;
constexpr std::size_t max_display_name_bytes = 256;
constexpr std::size_t max_permission_set_name_bytes = 256;
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

[[nodiscard]] bool assignment_applies(
    const AccessScope& assignment_scope,
    const AccessScope& requested_scope) noexcept {
    if (assignment_scope.kind == AccessScopeKind::global) {
        return assignment_scope.project_id.empty();
    }

    return requested_scope.kind == AccessScopeKind::project &&
           !assignment_scope.project_id.empty() &&
           assignment_scope.project_id == requested_scope.project_id;
}

}  // namespace

UsersAccessManager::UsersAccessManager(
    UsersAccessRepository& repository,
    UsersAccessIdGenerator id_generator)
    : repository_(repository),
      id_generator_(id_generator ? std::move(id_generator) : UsersAccessIdGenerator{default_id}) {}

UsersAccessManagerResult<User> UsersAccessManager::create_user(
    const CreateUserInput& input) {
    if (!has_non_whitespace(input.login)) {
        return UsersAccessManagerResult<User>::failure(
            UsersAccessManagerError::invalid_login);
    }
    if (input.login.size() > max_login_bytes) {
        return UsersAccessManagerResult<User>::failure(
            UsersAccessManagerError::login_too_long);
    }
    if (input.display_name.size() > max_display_name_bytes) {
        return UsersAccessManagerResult<User>::failure(
            UsersAccessManagerError::display_name_too_long);
    }

    User existing;
    const auto existing_status = repository_.find_user_by_login(input.login, existing);
    if (existing_status == UsersAccessRepositoryStatus::ok) {
        return UsersAccessManagerResult<User>::failure(
            UsersAccessManagerError::login_conflict);
    }
    if (existing_status != UsersAccessRepositoryStatus::not_found) {
        return UsersAccessManagerResult<User>::failure(
            UsersAccessManagerError::storage_error);
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

        const auto status = repository_.insert_user(user);
        if (status == UsersAccessRepositoryStatus::ok) {
            return UsersAccessManagerResult<User>::success(std::move(user));
        }
        if (status == UsersAccessRepositoryStatus::error) {
            return UsersAccessManagerResult<User>::failure(
                UsersAccessManagerError::storage_error);
        }
        if (status == UsersAccessRepositoryStatus::not_found) {
            return UsersAccessManagerResult<User>::failure(
                UsersAccessManagerError::storage_error);
        }

        const auto login_status = repository_.find_user_by_login(input.login, existing);
        if (login_status == UsersAccessRepositoryStatus::ok) {
            return UsersAccessManagerResult<User>::failure(
                UsersAccessManagerError::login_conflict);
        }
        if (login_status != UsersAccessRepositoryStatus::not_found) {
            return UsersAccessManagerResult<User>::failure(
                UsersAccessManagerError::storage_error);
        }
    }

    return UsersAccessManagerResult<User>::failure(
        UsersAccessManagerError::id_generation_failed);
}

UsersAccessManagerResult<User> UsersAccessManager::set_user_enabled(
    const std::string_view user_id,
    const bool enabled) {
    User user;
    const auto find_status = repository_.find_user_by_id(user_id, user);
    if (find_status == UsersAccessRepositoryStatus::not_found) {
        return UsersAccessManagerResult<User>::failure(
            UsersAccessManagerError::user_not_found);
    }
    if (find_status != UsersAccessRepositoryStatus::ok) {
        return UsersAccessManagerResult<User>::failure(
            UsersAccessManagerError::storage_error);
    }

    user.enabled = enabled;
    const auto update_status = repository_.update_user(user);
    if (update_status == UsersAccessRepositoryStatus::not_found) {
        return UsersAccessManagerResult<User>::failure(
            UsersAccessManagerError::user_not_found);
    }
    if (update_status != UsersAccessRepositoryStatus::ok) {
        return UsersAccessManagerResult<User>::failure(
            UsersAccessManagerError::storage_error);
    }

    return UsersAccessManagerResult<User>::success(std::move(user));
}

UsersAccessManagerResult<PermissionSet> UsersAccessManager::create_permission_set(
    const CreatePermissionSetInput& input) {
    if (!has_non_whitespace(input.name)) {
        return UsersAccessManagerResult<PermissionSet>::failure(
            UsersAccessManagerError::invalid_permission_set_name);
    }
    if (input.name.size() > max_permission_set_name_bytes) {
        return UsersAccessManagerResult<PermissionSet>::failure(
            UsersAccessManagerError::permission_set_name_too_long);
    }

    for (int attempt = 0; attempt < id_generation_attempts; ++attempt) {
        PermissionSet permission_set{
            .id = id_generator_(),
            .name = input.name,
            .capabilities = canonical_capabilities(input.capabilities),
        };

        if (permission_set.id.empty()) {
            continue;
        }

        const auto status = repository_.insert_permission_set(permission_set);
        if (status == UsersAccessRepositoryStatus::ok) {
            return UsersAccessManagerResult<PermissionSet>::success(
                std::move(permission_set));
        }
        if (status != UsersAccessRepositoryStatus::conflict) {
            return UsersAccessManagerResult<PermissionSet>::failure(
                UsersAccessManagerError::storage_error);
        }
    }

    return UsersAccessManagerResult<PermissionSet>::failure(
        UsersAccessManagerError::id_generation_failed);
}

UsersAccessManagerResult<AccessAssignment> UsersAccessManager::assign(
    const CreateAccessAssignmentInput& input) {
    if (!valid_scope(input.scope)) {
        return UsersAccessManagerResult<AccessAssignment>::failure(
            UsersAccessManagerError::invalid_scope);
    }

    User user;
    const auto user_status = repository_.find_user_by_id(input.user_id, user);
    if (user_status == UsersAccessRepositoryStatus::not_found) {
        return UsersAccessManagerResult<AccessAssignment>::failure(
            UsersAccessManagerError::user_not_found);
    }
    if (user_status != UsersAccessRepositoryStatus::ok) {
        return UsersAccessManagerResult<AccessAssignment>::failure(
            UsersAccessManagerError::storage_error);
    }

    PermissionSet permission_set;
    const auto permission_status = repository_.find_permission_set_by_id(
        input.permission_set_id,
        permission_set);
    if (permission_status == UsersAccessRepositoryStatus::not_found) {
        return UsersAccessManagerResult<AccessAssignment>::failure(
            UsersAccessManagerError::permission_set_not_found);
    }
    if (permission_status != UsersAccessRepositoryStatus::ok) {
        return UsersAccessManagerResult<AccessAssignment>::failure(
            UsersAccessManagerError::storage_error);
    }

    AccessAssignment assignment{
        .user_id = input.user_id,
        .permission_set_id = input.permission_set_id,
        .scope = input.scope,
    };

    const auto insert_status = repository_.insert_assignment(assignment);
    if (insert_status == UsersAccessRepositoryStatus::ok) {
        return UsersAccessManagerResult<AccessAssignment>::success(
            std::move(assignment));
    }
    if (insert_status == UsersAccessRepositoryStatus::conflict) {
        return UsersAccessManagerResult<AccessAssignment>::failure(
            UsersAccessManagerError::assignment_conflict);
    }
    return UsersAccessManagerResult<AccessAssignment>::failure(
        UsersAccessManagerError::storage_error);
}

AccessEvaluation UsersAccessManager::evaluate(
    const std::string_view user_id,
    const AccessScope& scope,
    const Capability required_capability) const {
    if (!valid_scope(scope)) {
        return AccessEvaluation{
            .allowed = false,
            .effective_capabilities = {},
            .error = AccessEvaluationError::invalid_scope,
        };
    }

    User user;
    const auto user_status = repository_.find_user_by_id(user_id, user);
    if (user_status == UsersAccessRepositoryStatus::not_found) {
        return AccessEvaluation{
            .allowed = false,
            .effective_capabilities = {},
            .error = AccessEvaluationError::user_not_found,
        };
    }
    if (user_status != UsersAccessRepositoryStatus::ok) {
        return AccessEvaluation{
            .allowed = false,
            .effective_capabilities = {},
            .error = AccessEvaluationError::storage_error,
        };
    }
    if (!user.enabled) {
        return AccessEvaluation{};
    }

    std::vector<AccessAssignment> assignments;
    if (repository_.list_assignments_for_user(user_id, assignments) !=
        UsersAccessRepositoryStatus::ok) {
        return AccessEvaluation{
            .allowed = false,
            .effective_capabilities = {},
            .error = AccessEvaluationError::storage_error,
        };
    }

    std::array<bool, all_capabilities.size()> effective{};

    for (const auto& assignment : assignments) {
        if (!valid_scope(assignment.scope)) {
            return AccessEvaluation{
                .allowed = false,
                .effective_capabilities = {},
                .error = AccessEvaluationError::storage_error,
            };
        }
        if (!assignment_applies(assignment.scope, scope)) {
            continue;
        }

        PermissionSet permission_set;
        if (repository_.find_permission_set_by_id(
                assignment.permission_set_id,
                permission_set) != UsersAccessRepositoryStatus::ok) {
            return AccessEvaluation{
                .allowed = false,
                .effective_capabilities = {},
                .error = AccessEvaluationError::storage_error,
            };
        }

        for (const auto capability : permission_set.capabilities) {
            const auto it = std::find(
                all_capabilities.begin(),
                all_capabilities.end(),
                capability);
            if (it == all_capabilities.end()) {
                return AccessEvaluation{
                    .allowed = false,
                    .effective_capabilities = {},
                    .error = AccessEvaluationError::storage_error,
                };
            }
            effective[static_cast<std::size_t>(it - all_capabilities.begin())] = true;
        }
    }

    std::vector<Capability> capabilities;
    for (std::size_t index = 0; index < all_capabilities.size(); ++index) {
        if (effective[index]) {
            capabilities.push_back(all_capabilities[index]);
        }
    }

    return AccessEvaluation{
        .allowed = std::find(
                       capabilities.begin(),
                       capabilities.end(),
                       required_capability) != capabilities.end(),
        .effective_capabilities = std::move(capabilities),
        .error = AccessEvaluationError::none,
    };
}

bool UsersAccessManager::valid_scope(const AccessScope& scope) noexcept {
    if (scope.kind == AccessScopeKind::global) {
        return scope.project_id.empty();
    }
    return has_non_whitespace(scope.project_id);
}

bool UsersAccessManager::has_non_whitespace(const std::string_view value) noexcept {
    return std::any_of(value.begin(), value.end(), [](const unsigned char character) {
        return std::isspace(character) == 0;
    });
}

std::vector<Capability> UsersAccessManager::canonical_capabilities(
    const std::vector<Capability>& capabilities) {
    std::vector<Capability> result;
    for (const auto capability : all_capabilities) {
        if (std::find(capabilities.begin(), capabilities.end(), capability) !=
            capabilities.end()) {
            result.push_back(capability);
        }
    }
    return result;
}

}  // namespace dispatcher::users_access
