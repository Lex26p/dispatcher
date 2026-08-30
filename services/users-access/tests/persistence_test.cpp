#include "dispatcher/users_access/bootstrap.hpp"
#include "dispatcher/users_access/openssl_scrypt_password_hasher.hpp"
#include "dispatcher/users_access/sqlite_users_access_repository.hpp"
#include "dispatcher/users_access/users_access_manager.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace ua = dispatcher::users_access;

namespace {

[[noreturn]] void fail(const std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
}

void expect(const bool condition, const std::string_view message) {
    if (!condition) {
        fail(message);
    }
}

class SequentialIdGenerator final {
public:
    std::string operator()() {
        return "persist-id-" + std::to_string(next_++);
    }

private:
    int next_{1};
};

[[nodiscard]] std::filesystem::path make_temp_dir() {
    const auto base = std::filesystem::temp_directory_path();
    for (int attempt = 0; attempt < 100; ++attempt) {
        const auto candidate =
            base / ("dispatcher-users-access-test-" + std::to_string(std::rand()));
        std::error_code error;
        if (std::filesystem::create_directory(candidate, error)) {
            return candidate;
        }
    }
    fail("failed to create temporary directory");
}

void test_scrypt_hash_and_verify() {
    ua::OpenSslScryptPasswordHasher hasher;
    ua::CredentialVerifier verifier;
    verifier.user_id = "user-1";

    expect(
        hasher.hash("correct horse battery staple", verifier) ==
            ua::PasswordHashStatus::ok,
        "scrypt hash should succeed");
    expect(verifier.algorithm == "scrypt", "verifier should record scrypt algorithm");
    expect(verifier.cost_n == (1ULL << 17U), "scrypt N should match documented baseline");
    expect(verifier.block_size_r == 8, "scrypt r should match documented baseline");
    expect(verifier.parallelization_p == 1, "scrypt p should match documented baseline");
    expect(!verifier.salt.empty(), "verifier should contain random salt");
    expect(!verifier.digest.empty(), "verifier should contain derived digest");

    bool matches = false;
    expect(
        hasher.verify("correct horse battery staple", verifier, matches) ==
            ua::PasswordHashStatus::ok &&
            matches,
        "correct password should verify");

    expect(
        hasher.verify("definitely-wrong-password", verifier, matches) ==
            ua::PasswordHashStatus::ok &&
            !matches,
        "wrong password should not verify");
}

void test_sqlite_reopen_and_fail_closed_state() {
    const auto temp_dir = make_temp_dir();
    const auto database_path = temp_dir / "users-access.db";

    std::string user_id;
    std::string permission_set_id;

    {
        ua::SqliteUsersAccessRepository repository{database_path.string()};
        expect(repository.ready(), "new SQLite repository should be ready");

        SequentialIdGenerator ids;
        ua::UsersAccessManager manager{repository, [&ids] { return ids(); }};

        const auto user = manager.create_user({
            .login = "operator",
            .display_name = "Operator",
            .enabled = true,
        });
        const auto permissions = manager.create_permission_set({
            .name = "Project editor",
            .capabilities = {ua::Capability::view, ua::Capability::edit},
        });
        expect(user.ok() && permissions.ok(), "durable fixtures should be created");

        user_id = user.value->id;
        permission_set_id = permissions.value->id;

        expect(
            manager.assign({
                .user_id = user_id,
                .permission_set_id = permission_set_id,
                .scope = ua::AccessScope::project("project-a"),
            }).ok(),
            "durable assignment should be created");

        ua::OpenSslScryptPasswordHasher hasher;
        ua::CredentialVerifier verifier;
        verifier.user_id = user_id;
        expect(
            hasher.hash("long-enough-test-password", verifier) ==
                ua::PasswordHashStatus::ok,
            "credential verifier should be generated");
        expect(
            repository.set_credential_verifier(verifier) ==
                ua::CredentialRepositoryStatus::ok,
            "credential verifier should be stored");

        expect(
            repository.append_security_audit({
                .sequence = 0,
                .occurred_at_unix_ms = 123456,
                .event = ua::SecurityAuditEventType::user_disabled,
                .actor_user_id = "admin-id",
                .subject_user_id = user_id,
            }) == ua::SecurityAuditRepositoryStatus::ok,
            "security audit record should be stored");

        const auto disabled = manager.set_user_enabled(user_id, false);
        expect(disabled.ok() && !disabled.value->enabled, "disabled state should persist");
    }

    {
        ua::SqliteUsersAccessRepository repository{database_path.string()};
        expect(repository.ready(), "SQLite repository should reopen");

        ua::User user;
        expect(
            repository.find_user_by_id(user_id, user) ==
                ua::UsersAccessRepositoryStatus::ok,
            "user should survive reopen");
        expect(!user.enabled, "disabled state should survive reopen");

        ua::PermissionSet permission_set;
        expect(
            repository.find_permission_set_by_id(permission_set_id, permission_set) ==
                ua::UsersAccessRepositoryStatus::ok,
            "permission set should survive reopen");

        std::vector<ua::AccessAssignment> assignments;
        expect(
            repository.list_assignments_for_user(user_id, assignments) ==
                ua::UsersAccessRepositoryStatus::ok &&
                assignments.size() == 1,
            "assignment should survive reopen");

        ua::CredentialVerifier verifier;
        expect(
            repository.find_credential_verifier(user_id, verifier) ==
                ua::CredentialRepositoryStatus::ok,
            "credential verifier should survive reopen");

        ua::OpenSslScryptPasswordHasher hasher;
        bool matches = false;
        expect(
            hasher.verify("long-enough-test-password", verifier, matches) ==
                ua::PasswordHashStatus::ok &&
                matches,
            "reopened credential should verify");

        std::vector<ua::SecurityAuditRecord> audit;
        expect(
            repository.list_security_audit(audit) ==
                ua::SecurityAuditRepositoryStatus::ok &&
                audit.size() == 1,
            "security audit should survive reopen");

        ua::UsersAccessManager manager{repository};
        const auto evaluation = manager.evaluate(
            user_id,
            ua::AccessScope::project("project-a"),
            ua::Capability::view);
        expect(evaluation.ok() && !evaluation.allowed, "disabled durable user must fail closed");
    }

    std::error_code error;
    std::filesystem::remove_all(temp_dir, error);
}

void test_atomic_bootstrap_and_no_plaintext_storage() {
    const auto temp_dir = make_temp_dir();
    const auto database_path = temp_dir / "bootstrap.db";
    const std::string secret = "bootstrap-secret-for-test-only";

    std::string admin_id;

    {
        ua::SqliteUsersAccessRepository repository{database_path.string()};
        expect(repository.ready(), "bootstrap repository should be ready");

        ua::OpenSslScryptPasswordHasher hasher;
        SequentialIdGenerator ids;
        ua::BootstrapService bootstrap{
            repository,
            hasher,
            [&ids] { return ids(); }};

        const auto created = bootstrap.bootstrap_first_admin(
            "admin",
            "Administrator",
            secret);
        expect(created.ok(), "first administrator bootstrap should succeed");
        admin_id = created.user->id;

        const auto second = bootstrap.bootstrap_first_admin(
            "other-admin",
            "Other",
            "another-long-test-password");
        expect(
            !second.ok() && second.error == ua::BootstrapError::already_initialized,
            "second bootstrap should be rejected");

        ua::UsersAccessManager manager{repository};
        for (const auto capability : ua::all_capabilities) {
            const auto evaluation = manager.evaluate(
                admin_id,
                ua::AccessScope::global(),
                capability);
            expect(
                evaluation.ok() && evaluation.allowed,
                "bootstrap administrator should receive every explicit global capability");
        }

        ua::CredentialVerifier verifier;
        expect(
            repository.find_credential_verifier(admin_id, verifier) ==
                ua::CredentialRepositoryStatus::ok,
            "bootstrap credential should be stored");

        bool matches = false;
        expect(
            hasher.verify(secret, verifier, matches) ==
                ua::PasswordHashStatus::ok &&
                matches,
            "bootstrap credential should verify");

        std::vector<ua::SecurityAuditRecord> audit;
        expect(
            repository.list_security_audit(audit) ==
                ua::SecurityAuditRepositoryStatus::ok &&
                audit.size() == 1 &&
                audit.front().event ==
                    ua::SecurityAuditEventType::bootstrap_admin_created &&
                audit.front().subject_user_id == admin_id,
            "bootstrap should create one durable audit record");
    }

    {
        std::ifstream input(database_path, std::ios::binary);
        const std::string bytes(
            (std::istreambuf_iterator<char>(input)),
            std::istreambuf_iterator<char>());
        expect(
            bytes.find(secret) == std::string::npos,
            "plaintext bootstrap password must not appear in SQLite file");
    }

    {
        ua::SqliteUsersAccessRepository repository{database_path.string()};
        expect(repository.ready(), "bootstrap database should reopen");
        ua::User user;
        expect(
            repository.find_user_by_id(admin_id, user) ==
                ua::UsersAccessRepositoryStatus::ok,
            "bootstrap administrator should survive reopen");
    }

    std::error_code error;
    std::filesystem::remove_all(temp_dir, error);
}

void test_unavailable_storage_fails() {
    ua::SqliteUsersAccessRepository empty_path{""};
    expect(!empty_path.ready(), "empty database path should fail");

    const auto temp_dir = make_temp_dir();
    ua::SqliteUsersAccessRepository directory_path{temp_dir.string()};
    expect(!directory_path.ready(), "directory path should not open as SQLite database");

    std::error_code error;
    std::filesystem::remove_all(temp_dir, error);
}

}  // namespace

int main() {
    test_scrypt_hash_and_verify();
    test_sqlite_reopen_and_fail_closed_state();
    test_atomic_bootstrap_and_no_plaintext_storage();
    test_unavailable_storage_fails();

    std::cout << "Users & Access persistence/credential tests passed\n";
    return 0;
}
