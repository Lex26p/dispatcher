#pragma once

#include "dispatcher/users_access/bootstrap.hpp"
#include "dispatcher/users_access/credential.hpp"
#include "dispatcher/users_access/security_audit.hpp"
#include "dispatcher/users_access/users_access_repository.hpp"

#include <string>
#include <string_view>

struct sqlite3;

namespace dispatcher::users_access {

class SqliteUsersAccessRepository final
    : public UsersAccessRepository,
      public CredentialRepository,
      public SecurityAuditRepository,
      public BootstrapStore {
public:
    explicit SqliteUsersAccessRepository(std::string_view database_path);
    ~SqliteUsersAccessRepository() override;

    SqliteUsersAccessRepository(const SqliteUsersAccessRepository&) = delete;
    SqliteUsersAccessRepository& operator=(const SqliteUsersAccessRepository&) = delete;

    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] const std::string& error_message() const noexcept;

    UsersAccessRepositoryStatus insert_user(const User& user) override;
    UsersAccessRepositoryStatus update_user(const User& user) override;
    UsersAccessRepositoryStatus find_user_by_id(
        std::string_view user_id,
        User& user) const override;
    UsersAccessRepositoryStatus find_user_by_login(
        std::string_view login,
        User& user) const override;
    UsersAccessRepositoryStatus insert_permission_set(
        const PermissionSet& permission_set) override;
    UsersAccessRepositoryStatus find_permission_set_by_id(
        std::string_view permission_set_id,
        PermissionSet& permission_set) const override;
    UsersAccessRepositoryStatus insert_assignment(
        const AccessAssignment& assignment) override;
    UsersAccessRepositoryStatus list_assignments_for_user(
        std::string_view user_id,
        std::vector<AccessAssignment>& assignments) const override;

    CredentialRepositoryStatus set_credential_verifier(
        const CredentialVerifier& verifier) override;
    CredentialRepositoryStatus find_credential_verifier(
        std::string_view user_id,
        CredentialVerifier& verifier) const override;

    SecurityAuditRepositoryStatus append_security_audit(
        const SecurityAuditRecord& record) override;
    SecurityAuditRepositoryStatus list_security_audit(
        std::vector<SecurityAuditRecord>& records) const override;

    BootstrapStoreStatus bootstrap_first_admin(
        const BootstrapAdminRecord& record) override;

private:
    [[nodiscard]] bool initialize_schema();
    [[nodiscard]] bool execute(std::string_view sql);
    void set_error(std::string message) const;

    sqlite3* database_{nullptr};
    mutable std::string error_message_;
    bool ready_{false};
};

}  // namespace dispatcher::users_access
