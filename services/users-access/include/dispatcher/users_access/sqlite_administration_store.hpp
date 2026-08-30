#pragma once

#include "dispatcher/users_access/administration.hpp"

#include <string>
#include <string_view>

struct sqlite3;

namespace dispatcher::users_access {

class SqliteUsersAccessAdministrationStore final
    : public UsersAccessAdministrationStore {
public:
    explicit SqliteUsersAccessAdministrationStore(std::string_view database_path);
    ~SqliteUsersAccessAdministrationStore() override;

    SqliteUsersAccessAdministrationStore(
        const SqliteUsersAccessAdministrationStore&) = delete;
    SqliteUsersAccessAdministrationStore& operator=(
        const SqliteUsersAccessAdministrationStore&) = delete;

    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] const std::string& error_message() const noexcept;

    AdministrationStoreStatus list_users(std::vector<User>& users) const override;
    AdministrationStoreStatus insert_user_with_credential(
        const User& user,
        const CredentialVerifier& verifier) override;
    AdministrationStoreStatus list_permission_sets(
        std::vector<PermissionSet>& permission_sets) const override;
    AdministrationStoreStatus list_assignments(
        std::optional<std::string_view> user_id,
        std::vector<AccessAssignment>& assignments) const override;
    AdministrationStoreStatus erase_assignment(
        const AccessAssignment& assignment) override;

private:
    [[nodiscard]] bool execute(std::string_view sql);
    void set_error(std::string message) const;

    sqlite3* database_{nullptr};
    mutable std::string error_message_;
    bool ready_{false};
};

}  // namespace dispatcher::users_access
