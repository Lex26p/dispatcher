#include "dispatcher/users_access/administration.hpp"
#include "dispatcher/users_access/openssl_scrypt_password_hasher.hpp"
#include "dispatcher/users_access/sqlite_administration_store.hpp"
#include "dispatcher/users_access/sqlite_users_access_repository.hpp"
#include "dispatcher/users_access/users_access_manager.hpp"

#include <filesystem>
#include <iostream>
#include <string>
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
        UsersAccessManager manager{repository, id_generator};
        UsersAccessAdministrationService administration{
            repository,
            repository,
            administration_store,
            password_hasher,
            manager,
            id_generator};

        auto short_password = administration.create_user(CreateAdministrationUserInput{
            .login = "short-password",
            .display_name = "Short Password",
            .enabled = true,
            .password = "too-short",
        });
        check(
            short_password.error == UsersAccessAdministrationError::password_too_short,
            "ordinary admin password should use the 15-byte minimum baseline");

        auto created = administration.create_user(CreateAdministrationUserInput{
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

        auto duplicate = administration.create_user(CreateAdministrationUserInput{
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

        auto disabled = administration.set_user_enabled(created.value->id, false);
        check(
            disabled.ok() && !disabled.value->enabled,
            "set-user-enabled should persist disabled state");

        check(
            administration.set_user_password(
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
            auto assignment = administration.assign(CreateAccessAssignmentInput{
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
                    administration.remove_assignment(*assignment.value) ==
                        UsersAccessAdministrationError::none,
                    "remove-access-assignment should delete exact composite assignment");
                check(
                    administration.remove_assignment(*assignment.value) ==
                        UsersAccessAdministrationError::assignment_not_found,
                    "removing missing assignment should report deterministic conflict");
            }
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
