#pragma once

#include <array>
#include <string_view>

namespace dispatcher::users_access::contract {

inline constexpr std::string_view service_address = "users-access.v1";

inline constexpr std::string_view login = "login";
inline constexpr std::string_view logout = "logout";
inline constexpr std::string_view current_session = "current-session";
inline constexpr std::string_view evaluate_access = "evaluate-access";
inline constexpr std::string_view list_users = "list-users";
inline constexpr std::string_view create_user = "create-user";
inline constexpr std::string_view set_user_enabled = "set-user-enabled";
inline constexpr std::string_view set_user_password = "set-user-password";
inline constexpr std::string_view list_permission_sets = "list-permission-sets";
inline constexpr std::string_view create_permission_set = "create-permission-set";
inline constexpr std::string_view list_access_assignments = "list-access-assignments";
inline constexpr std::string_view assign_access = "assign-access";
inline constexpr std::string_view remove_access_assignment = "remove-access-assignment";

inline constexpr std::array<std::string_view, 13> operations{
    login,
    logout,
    current_session,
    evaluate_access,
    list_users,
    create_user,
    set_user_enabled,
    set_user_password,
    list_permission_sets,
    create_permission_set,
    list_access_assignments,
    assign_access,
    remove_access_assignment,
};

}  // namespace dispatcher::users_access::contract
