#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dispatcher::users_access {

enum class Capability {
    view,
    control,
    edit,
    admin,
};

inline constexpr std::array<Capability, 4> all_capabilities{
    Capability::view,
    Capability::control,
    Capability::edit,
    Capability::admin,
};

[[nodiscard]] constexpr std::string_view capability_name(
    const Capability capability) noexcept {
    switch (capability) {
    case Capability::view:
        return "view";
    case Capability::control:
        return "control";
    case Capability::edit:
        return "edit";
    case Capability::admin:
        return "admin";
    }
    return "unknown";
}

enum class AccessScopeKind {
    global,
    project,
};

struct AccessScope final {
    AccessScopeKind kind{AccessScopeKind::global};
    std::string project_id;

    [[nodiscard]] static AccessScope global() {
        return AccessScope{};
    }

    [[nodiscard]] static AccessScope project(std::string id) {
        return AccessScope{AccessScopeKind::project, std::move(id)};
    }
};

struct PermissionSet final {
    std::string id;
    std::string name;
    std::vector<Capability> capabilities;
};

struct CreatePermissionSetInput final {
    std::string name;
    std::vector<Capability> capabilities;
};

struct AccessAssignment final {
    std::string user_id;
    std::string permission_set_id;
    AccessScope scope;
};

struct CreateAccessAssignmentInput final {
    std::string user_id;
    std::string permission_set_id;
    AccessScope scope;
};

enum class AccessEvaluationError {
    none,
    invalid_scope,
    user_not_found,
    storage_error,
};

struct AccessEvaluation final {
    bool allowed{false};
    std::vector<Capability> effective_capabilities;
    AccessEvaluationError error{AccessEvaluationError::none};

    [[nodiscard]] bool ok() const noexcept {
        return error == AccessEvaluationError::none;
    }

    [[nodiscard]] bool has(const Capability capability) const noexcept {
        return std::find(
                   effective_capabilities.begin(),
                   effective_capabilities.end(),
                   capability) != effective_capabilities.end();
    }
};

}  // namespace dispatcher::users_access
