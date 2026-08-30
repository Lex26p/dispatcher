#include "dispatcher/users_access/openssl_scrypt_password_hasher.hpp"
#include "dispatcher/users_access/openssl_session_token_codec.hpp"
#include "dispatcher/users_access/sqlite_users_access_repository.hpp"
#include "dispatcher/users_access/users_access_manager.hpp"

#include <sqlite3.h>

#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace dispatcher::users_access;

int fail(const std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    return 1;
}

[[nodiscard]] bool read_first_line(
    const std::string_view path,
    std::string& value) {
    std::ifstream stream{std::string(path)};
    return static_cast<bool>(std::getline(stream, value)) && !value.empty();
}

class Statement final {
public:
    Statement(sqlite3* database, const char* sql) {
        if (sqlite3_prepare_v2(
                database,
                sql,
                -1,
                &statement_,
                nullptr) != SQLITE_OK) {
            statement_ = nullptr;
        }
    }

    ~Statement() {
        if (statement_ != nullptr) {
            sqlite3_finalize(statement_);
        }
    }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    [[nodiscard]] sqlite3_stmt* get() const noexcept {
        return statement_;
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return statement_ != nullptr;
    }

private:
    sqlite3_stmt* statement_{nullptr};
};

[[nodiscard]] bool bind_text(
    sqlite3_stmt* statement,
    const int index,
    const std::string_view value) {
    return sqlite3_bind_text(
               statement,
               index,
               value.data(),
               static_cast<int>(value.size()),
               SQLITE_TRANSIENT) == SQLITE_OK;
}

[[nodiscard]] bool open_database(
    const std::string_view path,
    sqlite3*& database) {
    const std::string value(path);
    if (sqlite3_open_v2(
            value.c_str(),
            &database,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX,
            nullptr) != SQLITE_OK) {
        if (database != nullptr) {
            sqlite3_close(database);
            database = nullptr;
        }
        return false;
    }

    sqlite3_busy_timeout(database, 5000);
    return true;
}

int create_project_user(
    const std::string_view database_path,
    const std::string_view login,
    const std::string_view display_name,
    const std::string_view password_path,
    const std::string_view project_id,
    const std::string_view profile) {
    std::string password;
    if (!read_first_line(password_path, password)) {
        return fail("read fixture password");
    }

    SqliteUsersAccessRepository repository{database_path};
    if (!repository.ready()) {
        return fail("open Users & Access repository");
    }

    UsersAccessManager manager{repository};

    auto user_result = manager.create_user(CreateUserInput{
        .login = std::string(login),
        .display_name = std::string(display_name),
        .enabled = true,
    });
    if (!user_result.ok()) {
        return fail("create fixture user");
    }

    std::vector<Capability> capabilities;
    if (profile == "editor") {
        capabilities = {
            Capability::view,
            Capability::edit,
        };
    } else if (profile == "admin") {
        capabilities = {
            Capability::admin,
        };
    } else {
        return fail("unknown fixture permission profile");
    }

    auto permissions = manager.create_permission_set(
        CreatePermissionSetInput{
            .name = std::string(login) + " project " + std::string(profile),
            .capabilities = std::move(capabilities),
        });
    if (!permissions.ok()) {
        return fail("create fixture permission set");
    }

    auto assignment = manager.assign(CreateAccessAssignmentInput{
        .user_id = user_result.value->id,
        .permission_set_id = permissions.value->id,
        .scope = AccessScope::project(std::string(project_id)),
    });
    if (!assignment.ok()) {
        return fail("assign fixture project access");
    }

    OpenSslScryptPasswordHasher password_hasher;
    CredentialVerifier verifier;
    if (password_hasher.hash(password, verifier) !=
        PasswordHashStatus::ok) {
        return fail("hash fixture password");
    }

    verifier.user_id = user_result.value->id;
    if (repository.set_credential_verifier(verifier) !=
        CredentialRepositoryStatus::ok) {
        return fail("store fixture credential");
    }

    return 0;
}

int revoke_project(
    const std::string_view database_path,
    const std::string_view login,
    const std::string_view project_id) {
    sqlite3* database = nullptr;
    if (!open_database(database_path, database)) {
        return fail("open database for revocation");
    }

    Statement statement{
        database,
        "DELETE FROM access_assignments "
        "WHERE user_id=(SELECT id FROM users WHERE login=?1) "
        "AND scope_kind=1 AND project_id=?2;"};

    const bool ready =
        statement &&
        bind_text(statement.get(), 1, login) &&
        bind_text(statement.get(), 2, project_id);

    const int result =
        ready ? sqlite3_step(statement.get()) : SQLITE_ERROR;
    const int changed =
        result == SQLITE_DONE ? sqlite3_changes(database) : 0;

    sqlite3_close(database);

    return result == SQLITE_DONE && changed > 0
        ? 0
        : fail("revoke fixture project access");
}

int set_user_enabled(
    const std::string_view database_path,
    const std::string_view login,
    const bool enabled) {
    SqliteUsersAccessRepository repository{database_path};
    if (!repository.ready()) {
        return fail("open Users & Access repository");
    }

    User user;
    if (repository.find_user_by_login(login, user) !=
        UsersAccessRepositoryStatus::ok) {
        return fail("find fixture user");
    }

    UsersAccessManager manager{repository};
    const auto result = manager.set_user_enabled(user.id, enabled);
    return result.ok()
        ? 0
        : fail("update fixture user enabled state");
}

int expire_session(
    const std::string_view database_path,
    const std::string_view token_path) {
    std::string token;
    if (!read_first_line(token_path, token)) {
        return fail("read fixture session token");
    }

    OpenSslSessionTokenCodec codec;
    std::vector<unsigned char> digest;
    if (codec.digest(token, digest) != SessionTokenStatus::ok) {
        return fail("digest fixture session token");
    }

    sqlite3* database = nullptr;
    if (!open_database(database_path, database)) {
        return fail("open database for session expiry");
    }

    Statement statement{
        database,
        "UPDATE sessions SET absolute_expires_at_unix_ms=issued_at_unix_ms "
        "WHERE token_digest=?1;"};

    const bool ready =
        statement &&
        sqlite3_bind_blob(
            statement.get(),
            1,
            digest.data(),
            static_cast<int>(digest.size()),
            SQLITE_TRANSIENT) == SQLITE_OK;

    const int result =
        ready ? sqlite3_step(statement.get()) : SQLITE_ERROR;
    const int changed =
        result == SQLITE_DONE ? sqlite3_changes(database) : 0;

    sqlite3_close(database);

    return result == SQLITE_DONE && changed == 1
        ? 0
        : fail("expire fixture session");
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr
            << "Usage: project-manager-authorization-fixture <command> ...\n";
        return 2;
    }

    const std::string_view command = argv[1];

    if (command == "create-user" && argc == 8) {
        return create_project_user(
            argv[2],
            argv[3],
            argv[4],
            argv[5],
            argv[6],
            argv[7]);
    }

    if (command == "revoke-project" && argc == 5) {
        return revoke_project(argv[2], argv[3], argv[4]);
    }

    if (command == "set-enabled" && argc == 5) {
        const std::string_view enabled = argv[4];
        if (enabled != "0" && enabled != "1") {
            return fail("enabled value must be 0 or 1");
        }
        return set_user_enabled(
            argv[2],
            argv[3],
            enabled == "1");
    }

    if (command == "expire-session" && argc == 4) {
        return expire_session(argv[2], argv[3]);
    }

    return fail("unknown fixture command or invalid arguments");
}
