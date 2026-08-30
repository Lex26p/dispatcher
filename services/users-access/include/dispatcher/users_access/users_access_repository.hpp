#pragma once

#include "dispatcher/users_access/access.hpp"
#include "dispatcher/users_access/user.hpp"

#include <string_view>
#include <vector>

namespace dispatcher::users_access {

enum class UsersAccessRepositoryStatus {
    ok,
    not_found,
    conflict,
    error,
};

class UsersAccessRepository {
public:
    virtual ~UsersAccessRepository() = default;

    virtual UsersAccessRepositoryStatus insert_user(const User& user) = 0;
    virtual UsersAccessRepositoryStatus update_user(const User& user) = 0;

    virtual UsersAccessRepositoryStatus find_user_by_id(
        std::string_view user_id,
        User& user) const = 0;

    virtual UsersAccessRepositoryStatus find_user_by_login(
        std::string_view login,
        User& user) const = 0;

    virtual UsersAccessRepositoryStatus insert_permission_set(
        const PermissionSet& permission_set) = 0;

    virtual UsersAccessRepositoryStatus find_permission_set_by_id(
        std::string_view permission_set_id,
        PermissionSet& permission_set) const = 0;

    virtual UsersAccessRepositoryStatus insert_assignment(
        const AccessAssignment& assignment) = 0;

    virtual UsersAccessRepositoryStatus list_assignments_for_user(
        std::string_view user_id,
        std::vector<AccessAssignment>& assignments) const = 0;
};

}  // namespace dispatcher::users_access
