#include "dispatcher/users_access/administration.hpp"
#include "dispatcher/users_access/openssl_scrypt_password_hasher.hpp"
#include "dispatcher/users_access/sqlite_administration_store.hpp"
#include "dispatcher/users_access/sqlite_users_access_repository.hpp"

#include <sqlite3.h>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace dispatcher::users_access;

int failures = 0;

void check(const bool condition, const char* message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

std::filesystem::path temp_database_path() {
    const auto directory = std::filesystem::temp_directory_path();
    return directory / "dispatcher-users-access-administration-test.db";
}

}  // namespace

int main() {
    const auto path = temp_database_path();
    std::error_code ignored;
    std::filesystem::remove(path, ignored);

    {
        SqliteUsersAccessRepository repository{path.string()};
        check(repository.ready(), "base SQLite repository should initialize schema");

        SqliteUsersAccessAdministrationStore administration_store{path.string()};
        check(administration_store.ready(), "administration store should open schema v2");

        OpenSslScryptPasswordHasher password_hasher;
        int next_id = 1;
        auto id_generator = [&next_id] {
            return "admin-test-id-" + std::to_string(next_id++);
        };
        constexpr std::string_view actor_user_id = "admin-actor";
        std::int64_t audit_time = 1000;
        UsersAccessAdministrationService administration{
            repository,
            administration_store,
            password_hasher,
            id_generator,
            [&audit_time] { return audit_time++; }};

        auto short_password = administration.create_user(actor_user_id, CreateAdministrationUserInput{
            .login = "short-password",
            .display_name = "Short Password",
            .enabled = true,
            .password = "too-short",
        });
        check(
            short_password.error == UsersAccessAdministrationError::password_too_short,
            "ordinary admin password should use the 15-byte minimum baseline");

        auto created = administration.create_user(actor_user_id, CreateAdministrationUserInput{
            .login = "engineer",
            .display_name = "Engineer",
            .enabled = true,
            .password = "Administration password 1",
        });
        check(created.ok(), "admin create-user should create user and credential atomically");
        if (!created.ok()) {
            std::filesystem::remove(path, ignored);
            return 1;
        }

        CredentialVerifier verifier;
        check(
            repository.find_credential_verifier(created.value->id, verifier) ==
                CredentialRepositoryStatus::ok,
            "created user should have a credential verifier");
        bool password_matches = false;
        check(
            password_hasher.verify(
                "Administration password 1",
                verifier,
                password_matches) == PasswordHashStatus::ok && password_matches,
            "created credential should verify the initial password");

        auto duplicate = administration.create_user(actor_user_id, CreateAdministrationUserInput{
            .login = "engineer",
            .display_name = "Duplicate",
            .enabled = true,
            .password = "Administration password 2",
        });
        check(
            duplicate.error == UsersAccessAdministrationError::login_conflict,
            "duplicate login should return conflict");

        auto users = administration.list_users();
        check(users.ok() && users.value->size() == 1, "list-users should return created user");

        auto disabled = administration.set_user_enabled(actor_user_id, created.value->id, false);
        check(
            disabled.ok() && !disabled.value->enabled,
            "set-user-enabled should persist disabled state");
        auto disabled_again = administration.set_user_enabled(
            actor_user_id,
            created.value->id,
            false);
        check(
            disabled_again.ok() && !disabled_again.value->enabled,
            "repeating the same enabled state should be a successful no-op");

        check(
            administration.set_user_password(
                actor_user_id,
                created.value->id,
                "Administration replacement password") ==
                UsersAccessAdministrationError::none,
            "set-user-password should replace credential verifier");
        check(
            repository.find_credential_verifier(created.value->id, verifier) ==
                CredentialRepositoryStatus::ok,
            "replacement credential should remain readable");
        password_matches = false;
        check(
            password_hasher.verify(
                "Administration replacement password",
                verifier,
                password_matches) == PasswordHashStatus::ok && password_matches,
            "replacement password should verify");

        auto permission_set = administration.create_permission_set(
            actor_user_id,
            CreatePermissionSetInput{
                .name = "Project editors",
                .capabilities = {Capability::view, Capability::edit},
            });
        check(permission_set.ok(), "create-permission-set should succeed");

        auto permission_sets = administration.list_permission_sets();
        check(
            permission_sets.ok() && permission_sets.value->size() == 1,
            "list-permission-sets should return created set");

        if (permission_set.ok()) {
            auto assignment = administration.assign(actor_user_id, CreateAccessAssignmentInput{
                .user_id = created.value->id,
                .permission_set_id = permission_set.value->id,
                .scope = AccessScope::project("project-42"),
            });
            check(assignment.ok(), "assign-access should create project assignment");

            auto filtered = administration.list_assignments(created.value->id);
            check(
                filtered.ok() && filtered.value->size() == 1 &&
                    filtered.value->front().scope.project_id == "project-42",
                "list-access-assignments should support user filter");

            auto all = administration.list_assignments(std::nullopt);
            check(all.ok() && all.value->size() == 1, "assignment list without filter should work");

            if (assignment.ok()) {
                check(
                    administration.remove_assignment(actor_user_id, *assignment.value) ==
                        UsersAccessAdministrationError::none,
                    "remove-access-assignment should delete exact composite assignment");
                check(
                    administration.remove_assignment(actor_user_id, *assignment.value) ==
                        UsersAccessAdministrationError::assignment_not_found,
                    "removing missing assignment should report deterministic conflict");
            }
        }

        std::vector<SecurityAuditRecord> audit_records;
        check(
            repository.list_security_audit(audit_records) ==
                SecurityAuditRepositoryStatus::ok,
            "administration audit records should remain readable through the repository");
        check(
            audit_records.size() == 6,
            "only successful administration mutations should create audit records");

        const std::vector<SecurityAuditEventType> expected_events{
            SecurityAuditEventType::user_created,
            SecurityAuditEventType::user_disabled,
            SecurityAuditEventType::user_password_reset,
            SecurityAuditEventType::permission_set_created,
            SecurityAuditEventType::access_assignment_added,
            SecurityAuditEventType::access_assignment_removed,
        };
        if (audit_records.size() == expected_events.size()) {
            for (std::size_t index = 0; index < expected_events.size(); ++index) {
                check(
                    audit_records[index].event == expected_events[index],
                    "administration audit event order should match successful mutations");
                check(
                    audit_records[index].actor_user_id == actor_user_id,
                    "administration audit should preserve authenticated actor user id");
                check(
                    audit_records[index].occurred_at_unix_ms ==
                        1000 + static_cast<std::int64_t>(index),
                    "administration audit should use the injected clock");
            }
            check(
                audit_records[0].subject_user_id == created.value->id &&
                    audit_records[1].subject_user_id == created.value->id &&
                    audit_records[2].subject_user_id == created.value->id &&
                    audit_records[3].subject_user_id.empty() &&
                    audit_records[4].subject_user_id == created.value->id &&
                    audit_records[5].subject_user_id == created.value->id,
                "administration audit should use user subject semantics consistently");
        }

        sqlite3* audit_failure_database = nullptr;
        const int audit_failure_open_status = sqlite3_open_v2(
            path.string().c_str(),
            &audit_failure_database,
            SQLITE_OPEN_READWRITE,
            nullptr);
        check(
            audit_failure_open_status == SQLITE_OK,
            "atomicity fixture should open the administration database");
        if (audit_failure_open_status == SQLITE_OK &&
            audit_failure_database != nullptr) {
            const char* create_trigger =
                "CREATE TRIGGER reject_admin_user_created_audit "
                "BEFORE INSERT ON security_audit "
                "WHEN NEW.event_type='user_created' "
                "BEGIN SELECT RAISE(ABORT, 'audit write blocked'); END;";
            check(
                sqlite3_exec(
                    audit_failure_database,
                    create_trigger,
                    nullptr,
                    nullptr,
                    nullptr) == SQLITE_OK,
                "atomicity fixture should install an audit failure trigger");

            auto rolled_back = administration.create_user(
                actor_user_id,
                CreateAdministrationUserInput{
                    .login = "rolled-back-user",
                    .display_name = "Rolled Back",
                    .enabled = true,
                    .password = "Administration rollback password",
                });
            check(
                rolled_back.error == UsersAccessAdministrationError::storage_error,
                "audit write failure should fail the administration mutation");

            User rolled_back_user;
            check(
                repository.find_user_by_login("rolled-back-user", rolled_back_user) ==
                    UsersAccessRepositoryStatus::not_found,
                "audit write failure should roll back the user and credential mutation");

            std::vector<SecurityAuditRecord> after_rollback;
            check(
                repository.list_security_audit(after_rollback) ==
                        SecurityAuditRepositoryStatus::ok &&
                    after_rollback.size() == expected_events.size(),
                "failed atomic administration mutation should not leave an audit row");

            check(
                sqlite3_exec(
                    audit_failure_database,
                    "DROP TRIGGER reject_admin_user_created_audit;",
                    nullptr,
                    nullptr,
                    nullptr) == SQLITE_OK,
                "atomicity fixture should remove the audit failure trigger");
            sqlite3_close(audit_failure_database);
        }
    }

    std::filesystem::remove(path, ignored);

    if (failures != 0) {
        std::cerr << failures << " administration test assertion(s) failed\n";
        return 1;
    }

    std::cout << "Users & Access administration tests passed\n";
    return 0;
}
