#include "dispatcher/users_access/sqlite_administration_store.hpp"

#include <sqlite3.h>

#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace dispatcher::users_access {
namespace {

class Statement final {
public:
    Statement(sqlite3* database, const char* sql) {
        if (sqlite3_prepare_v2(database, sql, -1, &statement_, nullptr) != SQLITE_OK) {
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

[[nodiscard]] bool bind_blob(
    sqlite3_stmt* statement,
    const int index,
    const std::vector<unsigned char>& value) {
    return sqlite3_bind_blob(
               statement,
               index,
               value.data(),
               static_cast<int>(value.size()),
               SQLITE_TRANSIENT) == SQLITE_OK;
}

[[nodiscard]] std::string column_text(sqlite3_stmt* statement, const int index) {
    const auto* text = sqlite3_column_text(statement, index);
    const int bytes = sqlite3_column_bytes(statement, index);
    if (text == nullptr || bytes <= 0) {
        return {};
    }
    return std::string(
        reinterpret_cast<const char*>(text),
        static_cast<std::size_t>(bytes));
}

[[nodiscard]] bool capabilities_from_mask(
    const std::uint32_t mask,
    std::vector<Capability>& capabilities) {
    if ((mask & ~0x0FU) != 0U) {
        return false;
    }

    capabilities.clear();
    for (std::size_t index = 0; index < all_capabilities.size(); ++index) {
        if ((mask & (1U << index)) != 0U) {
            capabilities.push_back(all_capabilities[index]);
        }
    }
    return true;
}

[[nodiscard]] int scope_kind_value(const AccessScopeKind kind) noexcept {
    return kind == AccessScopeKind::global ? 0 : 1;
}

[[nodiscard]] bool scope_from_row(
    sqlite3_stmt* statement,
    const int kind_index,
    const int project_index,
    AccessScope& scope) {
    const int kind = sqlite3_column_int(statement, kind_index);
    const std::string project_id = column_text(statement, project_index);
    if (kind == 0 && project_id.empty()) {
        scope = AccessScope::global();
        return true;
    }
    if (kind == 1 && !project_id.empty()) {
        scope = AccessScope::project(project_id);
        return true;
    }
    return false;
}

}  // namespace

SqliteUsersAccessAdministrationStore::SqliteUsersAccessAdministrationStore(
    const std::string_view database_path) {
    if (database_path.empty()) {
        error_message_ = "database path is empty";
        return;
    }

    const std::string path(database_path);
    const int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX;
    if (sqlite3_open_v2(path.c_str(), &database_, flags, nullptr) != SQLITE_OK) {
        error_message_ = database_ != nullptr
            ? sqlite3_errmsg(database_)
            : "failed to allocate SQLite connection";
        if (database_ != nullptr) {
            sqlite3_close(database_);
            database_ = nullptr;
        }
        return;
    }

    sqlite3_extended_result_codes(database_, 1);
    sqlite3_busy_timeout(database_, 5000);
    if (!execute("PRAGMA foreign_keys = ON;")) {
        return;
    }

    Statement version(database_, "PRAGMA user_version;");
    if (!version || sqlite3_step(version.get()) != SQLITE_ROW) {
        set_error(sqlite3_errmsg(database_));
        return;
    }
    if (sqlite3_column_int(version.get(), 0) != 2) {
        set_error("Users & Access administration requires SQLite schema v2");
        return;
    }

    ready_ = true;
}

SqliteUsersAccessAdministrationStore::~SqliteUsersAccessAdministrationStore() {
    if (database_ != nullptr) {
        sqlite3_close(database_);
    }
}

bool SqliteUsersAccessAdministrationStore::ready() const noexcept {
    return ready_;
}

const std::string& SqliteUsersAccessAdministrationStore::error_message() const noexcept {
    return error_message_;
}

bool SqliteUsersAccessAdministrationStore::execute(const std::string_view sql) {
    char* message = nullptr;
    const std::string statement(sql);
    const int result = sqlite3_exec(
        database_,
        statement.c_str(),
        nullptr,
        nullptr,
        &message);
    if (result == SQLITE_OK) {
        return true;
    }

    if (message != nullptr) {
        error_message_ = message;
        sqlite3_free(message);
    } else {
        error_message_ = sqlite3_errmsg(database_);
    }
    return false;
}

void SqliteUsersAccessAdministrationStore::set_error(std::string message) const {
    error_message_ = std::move(message);
}

AdministrationStoreStatus SqliteUsersAccessAdministrationStore::list_users(
    std::vector<User>& users) const {
    users.clear();
    if (!ready_) {
        return AdministrationStoreStatus::error;
    }

    Statement statement(
        database_,
        "SELECT id, login, display_name, enabled FROM users ORDER BY login, id;");
    if (!statement) {
        set_error(sqlite3_errmsg(database_));
        return AdministrationStoreStatus::error;
    }

    for (;;) {
        const int result = sqlite3_step(statement.get());
        if (result == SQLITE_DONE) {
            return AdministrationStoreStatus::ok;
        }
        if (result != SQLITE_ROW) {
            set_error(sqlite3_errmsg(database_));
            return AdministrationStoreStatus::error;
        }

        users.push_back(User{
            .id = column_text(statement.get(), 0),
            .login = column_text(statement.get(), 1),
            .display_name = column_text(statement.get(), 2),
            .enabled = sqlite3_column_int(statement.get(), 3) != 0,
        });
    }
}

AdministrationStoreStatus
SqliteUsersAccessAdministrationStore::insert_user_with_credential(
    const User& user,
    const CredentialVerifier& verifier) {
    if (!ready_ || verifier.user_id != user.id) {
        return AdministrationStoreStatus::error;
    }
    if (!execute("BEGIN IMMEDIATE;")) {
        return AdministrationStoreStatus::error;
    }

    bool success = false;
    AdministrationStoreStatus status = AdministrationStoreStatus::error;

    {
        Statement statement(
            database_,
            "INSERT INTO users(id, login, display_name, enabled) "
            "VALUES(?1, ?2, ?3, ?4);");
        if (!statement ||
            !bind_text(statement.get(), 1, user.id) ||
            !bind_text(statement.get(), 2, user.login) ||
            !bind_text(statement.get(), 3, user.display_name) ||
            sqlite3_bind_int(statement.get(), 4, user.enabled ? 1 : 0) != SQLITE_OK) {
            set_error(sqlite3_errmsg(database_));
        } else {
            const int result = sqlite3_step(statement.get());
            if (result == SQLITE_DONE) {
                success = true;
            } else if (
                result == SQLITE_CONSTRAINT_PRIMARYKEY ||
                result == SQLITE_CONSTRAINT_UNIQUE) {
                status = AdministrationStoreStatus::conflict;
            } else {
                set_error(sqlite3_errmsg(database_));
            }
        }
    }

    if (success) {
        Statement statement(
            database_,
            "INSERT INTO credential_verifiers("
            "user_id, algorithm, cost_n, block_size_r, parallelization_p, salt, digest"
            ") VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7);");
        if (!statement ||
            !bind_text(statement.get(), 1, verifier.user_id) ||
            !bind_text(statement.get(), 2, verifier.algorithm) ||
            sqlite3_bind_int64(
                statement.get(),
                3,
                static_cast<sqlite3_int64>(verifier.cost_n)) != SQLITE_OK ||
            sqlite3_bind_int64(statement.get(), 4, verifier.block_size_r) != SQLITE_OK ||
            sqlite3_bind_int64(statement.get(), 5, verifier.parallelization_p) != SQLITE_OK ||
            !bind_blob(statement.get(), 6, verifier.salt) ||
            !bind_blob(statement.get(), 7, verifier.digest)) {
            set_error(sqlite3_errmsg(database_));
            success = false;
        } else if (sqlite3_step(statement.get()) != SQLITE_DONE) {
            set_error(sqlite3_errmsg(database_));
            success = false;
        }
    }

    if (!success) {
        (void)execute("ROLLBACK;");
        return status;
    }
    if (!execute("COMMIT;")) {
        (void)execute("ROLLBACK;");
        return AdministrationStoreStatus::error;
    }
    return AdministrationStoreStatus::ok;
}

AdministrationStoreStatus
SqliteUsersAccessAdministrationStore::list_permission_sets(
    std::vector<PermissionSet>& permission_sets) const {
    permission_sets.clear();
    if (!ready_) {
        return AdministrationStoreStatus::error;
    }

    Statement statement(
        database_,
        "SELECT id, name, capabilities FROM permission_sets ORDER BY name, id;");
    if (!statement) {
        set_error(sqlite3_errmsg(database_));
        return AdministrationStoreStatus::error;
    }

    for (;;) {
        const int result = sqlite3_step(statement.get());
        if (result == SQLITE_DONE) {
            return AdministrationStoreStatus::ok;
        }
        if (result != SQLITE_ROW) {
            set_error(sqlite3_errmsg(database_));
            return AdministrationStoreStatus::error;
        }

        const auto raw_mask = sqlite3_column_int64(statement.get(), 2);
        std::vector<Capability> capabilities;
        if (raw_mask < 0 ||
            raw_mask > std::numeric_limits<std::uint32_t>::max() ||
            !capabilities_from_mask(
                static_cast<std::uint32_t>(raw_mask),
                capabilities)) {
            set_error("invalid capability mask in database");
            return AdministrationStoreStatus::error;
        }

        permission_sets.push_back(PermissionSet{
            .id = column_text(statement.get(), 0),
            .name = column_text(statement.get(), 1),
            .capabilities = std::move(capabilities),
        });
    }
}

AdministrationStoreStatus SqliteUsersAccessAdministrationStore::list_assignments(
    const std::optional<std::string_view> user_id,
    std::vector<AccessAssignment>& assignments) const {
    assignments.clear();
    if (!ready_) {
        return AdministrationStoreStatus::error;
    }

    Statement statement(
        database_,
        user_id.has_value()
            ? "SELECT user_id, permission_set_id, scope_kind, project_id "
              "FROM access_assignments WHERE user_id=?1 "
              "ORDER BY user_id, permission_set_id, scope_kind, project_id;"
            : "SELECT user_id, permission_set_id, scope_kind, project_id "
              "FROM access_assignments "
              "ORDER BY user_id, permission_set_id, scope_kind, project_id;");
    if (!statement ||
        (user_id.has_value() && !bind_text(statement.get(), 1, *user_id))) {
        set_error(sqlite3_errmsg(database_));
        return AdministrationStoreStatus::error;
    }

    for (;;) {
        const int result = sqlite3_step(statement.get());
        if (result == SQLITE_DONE) {
            return AdministrationStoreStatus::ok;
        }
        if (result != SQLITE_ROW) {
            set_error(sqlite3_errmsg(database_));
            return AdministrationStoreStatus::error;
        }

        AccessScope scope;
        if (!scope_from_row(statement.get(), 2, 3, scope)) {
            set_error("invalid access scope in database");
            return AdministrationStoreStatus::error;
        }

        assignments.push_back(AccessAssignment{
            .user_id = column_text(statement.get(), 0),
            .permission_set_id = column_text(statement.get(), 1),
            .scope = std::move(scope),
        });
    }
}

AdministrationStoreStatus SqliteUsersAccessAdministrationStore::erase_assignment(
    const AccessAssignment& assignment) {
    if (!ready_) {
        return AdministrationStoreStatus::error;
    }

    Statement statement(
        database_,
        "DELETE FROM access_assignments "
        "WHERE user_id=?1 AND permission_set_id=?2 "
        "AND scope_kind=?3 AND project_id=?4;");
    if (!statement ||
        !bind_text(statement.get(), 1, assignment.user_id) ||
        !bind_text(statement.get(), 2, assignment.permission_set_id) ||
        sqlite3_bind_int(
            statement.get(),
            3,
            scope_kind_value(assignment.scope.kind)) != SQLITE_OK ||
        !bind_text(statement.get(), 4, assignment.scope.project_id)) {
        set_error(sqlite3_errmsg(database_));
        return AdministrationStoreStatus::error;
    }

    if (sqlite3_step(statement.get()) != SQLITE_DONE) {
        set_error(sqlite3_errmsg(database_));
        return AdministrationStoreStatus::error;
    }

    return sqlite3_changes(database_) == 0
        ? AdministrationStoreStatus::not_found
        : AdministrationStoreStatus::ok;
}

}  // namespace dispatcher::users_access
