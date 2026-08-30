#include "dispatcher/users_access/sqlite_users_access_repository.hpp"

#include <sqlite3.h>

#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace dispatcher::users_access {
namespace {

constexpr int supported_schema_version = 1;

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
    return std::string(reinterpret_cast<const char*>(text), static_cast<std::size_t>(bytes));
}

[[nodiscard]] std::vector<unsigned char> column_blob(
    sqlite3_stmt* statement,
    const int index) {
    const auto* data = static_cast<const unsigned char*>(
        sqlite3_column_blob(statement, index));
    const int bytes = sqlite3_column_bytes(statement, index);
    if (data == nullptr || bytes <= 0) {
        return {};
    }
    return std::vector<unsigned char>(data, data + bytes);
}

[[nodiscard]] std::uint32_t capability_mask(
    const std::vector<Capability>& capabilities) noexcept {
    std::uint32_t mask = 0;
    for (const auto capability : capabilities) {
        switch (capability) {
        case Capability::view:
            mask |= 1U << 0U;
            break;
        case Capability::control:
            mask |= 1U << 1U;
            break;
        case Capability::edit:
            mask |= 1U << 2U;
            break;
        case Capability::admin:
            mask |= 1U << 3U;
            break;
        }
    }
    return mask;
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

[[nodiscard]] const char* audit_event_name(const SecurityAuditEventType event) noexcept {
    return security_audit_event_name(event);
}

[[nodiscard]] bool audit_event_from_name(
    const std::string_view value,
    SecurityAuditEventType& event) noexcept {
    if (value == "bootstrap_admin_created") {
        event = SecurityAuditEventType::bootstrap_admin_created;
        return true;
    }
    if (value == "user_enabled") {
        event = SecurityAuditEventType::user_enabled;
        return true;
    }
    if (value == "user_disabled") {
        event = SecurityAuditEventType::user_disabled;
        return true;
    }
    return false;
}

}  // namespace

SqliteUsersAccessRepository::SqliteUsersAccessRepository(
    const std::string_view database_path) {
    if (database_path.empty()) {
        error_message_ = "database path is empty";
        return;
    }

    const std::string path(database_path);
    const int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
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

    ready_ = initialize_schema();
}

SqliteUsersAccessRepository::~SqliteUsersAccessRepository() {
    if (database_ != nullptr) {
        sqlite3_close(database_);
    }
}

bool SqliteUsersAccessRepository::ready() const noexcept {
    return ready_;
}

const std::string& SqliteUsersAccessRepository::error_message() const noexcept {
    return error_message_;
}

bool SqliteUsersAccessRepository::execute(const std::string_view sql) {
    char* message = nullptr;
    const std::string statement(sql);
    const int result = sqlite3_exec(database_, statement.c_str(), nullptr, nullptr, &message);
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

void SqliteUsersAccessRepository::set_error(std::string message) const {
    error_message_ = std::move(message);
}

bool SqliteUsersAccessRepository::initialize_schema() {
    Statement version_statement(database_, "PRAGMA user_version;");
    if (!version_statement || sqlite3_step(version_statement.get()) != SQLITE_ROW) {
        set_error(sqlite3_errmsg(database_));
        return false;
    }

    const int version = sqlite3_column_int(version_statement.get(), 0);
    if (version > supported_schema_version) {
        set_error("database schema is newer than this executable supports");
        return false;
    }
    if (version == supported_schema_version) {
        return true;
    }

    if (!execute("BEGIN IMMEDIATE;")) {
        return false;
    }

    const bool created =
        execute(
            "CREATE TABLE users ("
            "id TEXT PRIMARY KEY NOT NULL,"
            "login TEXT NOT NULL UNIQUE,"
            "display_name TEXT NOT NULL,"
            "enabled INTEGER NOT NULL CHECK(enabled IN (0,1))"
            ");") &&
        execute(
            "CREATE TABLE permission_sets ("
            "id TEXT PRIMARY KEY NOT NULL,"
            "name TEXT NOT NULL,"
            "capabilities INTEGER NOT NULL"
            ");") &&
        execute(
            "CREATE TABLE access_assignments ("
            "user_id TEXT NOT NULL REFERENCES users(id) ON DELETE CASCADE,"
            "permission_set_id TEXT NOT NULL REFERENCES permission_sets(id) ON DELETE CASCADE,"
            "scope_kind INTEGER NOT NULL CHECK(scope_kind IN (0,1)),"
            "project_id TEXT NOT NULL,"
            "UNIQUE(user_id, permission_set_id, scope_kind, project_id)"
            ");") &&
        execute(
            "CREATE TABLE credential_verifiers ("
            "user_id TEXT PRIMARY KEY NOT NULL REFERENCES users(id) ON DELETE CASCADE,"
            "algorithm TEXT NOT NULL,"
            "cost_n INTEGER NOT NULL,"
            "block_size_r INTEGER NOT NULL,"
            "parallelization_p INTEGER NOT NULL,"
            "salt BLOB NOT NULL,"
            "digest BLOB NOT NULL"
            ");") &&
        execute(
            "CREATE TABLE security_audit ("
            "sequence INTEGER PRIMARY KEY AUTOINCREMENT,"
            "occurred_at_unix_ms INTEGER NOT NULL,"
            "event_type TEXT NOT NULL,"
            "actor_user_id TEXT NOT NULL,"
            "subject_user_id TEXT NOT NULL"
            ");") &&
        execute("PRAGMA user_version = 1;");

    if (!created) {
        (void)execute("ROLLBACK;");
        return false;
    }
    if (!execute("COMMIT;")) {
        (void)execute("ROLLBACK;");
        return false;
    }
    return true;
}

UsersAccessRepositoryStatus SqliteUsersAccessRepository::insert_user(
    const User& user) {
    if (!ready_) {
        return UsersAccessRepositoryStatus::error;
    }

    Statement statement(
        database_,
        "INSERT INTO users(id, login, display_name, enabled) VALUES(?1, ?2, ?3, ?4);");
    if (!statement ||
        !bind_text(statement.get(), 1, user.id) ||
        !bind_text(statement.get(), 2, user.login) ||
        !bind_text(statement.get(), 3, user.display_name) ||
        sqlite3_bind_int(statement.get(), 4, user.enabled ? 1 : 0) != SQLITE_OK) {
        set_error(sqlite3_errmsg(database_));
        return UsersAccessRepositoryStatus::error;
    }

    const int result = sqlite3_step(statement.get());
    if (result == SQLITE_DONE) {
        return UsersAccessRepositoryStatus::ok;
    }
    if (result == SQLITE_CONSTRAINT_PRIMARYKEY ||
        result == SQLITE_CONSTRAINT_UNIQUE) {
        return UsersAccessRepositoryStatus::conflict;
    }

    set_error(sqlite3_errmsg(database_));
    return UsersAccessRepositoryStatus::error;
}

UsersAccessRepositoryStatus SqliteUsersAccessRepository::update_user(
    const User& user) {
    if (!ready_) {
        return UsersAccessRepositoryStatus::error;
    }

    Statement statement(
        database_,
        "UPDATE users SET login=?2, display_name=?3, enabled=?4 WHERE id=?1;");
    if (!statement ||
        !bind_text(statement.get(), 1, user.id) ||
        !bind_text(statement.get(), 2, user.login) ||
        !bind_text(statement.get(), 3, user.display_name) ||
        sqlite3_bind_int(statement.get(), 4, user.enabled ? 1 : 0) != SQLITE_OK) {
        set_error(sqlite3_errmsg(database_));
        return UsersAccessRepositoryStatus::error;
    }

    const int result = sqlite3_step(statement.get());
    if (result == SQLITE_DONE) {
        return sqlite3_changes(database_) == 0
            ? UsersAccessRepositoryStatus::not_found
            : UsersAccessRepositoryStatus::ok;
    }
    if (result == SQLITE_CONSTRAINT_UNIQUE) {
        return UsersAccessRepositoryStatus::conflict;
    }

    set_error(sqlite3_errmsg(database_));
    return UsersAccessRepositoryStatus::error;
}

UsersAccessRepositoryStatus SqliteUsersAccessRepository::find_user_by_id(
    const std::string_view user_id,
    User& user) const {
    if (!ready_) {
        return UsersAccessRepositoryStatus::error;
    }

    Statement statement(
        database_,
        "SELECT id, login, display_name, enabled FROM users WHERE id=?1;");
    if (!statement || !bind_text(statement.get(), 1, user_id)) {
        set_error(sqlite3_errmsg(database_));
        return UsersAccessRepositoryStatus::error;
    }

    const int result = sqlite3_step(statement.get());
    if (result == SQLITE_DONE) {
        return UsersAccessRepositoryStatus::not_found;
    }
    if (result != SQLITE_ROW) {
        set_error(sqlite3_errmsg(database_));
        return UsersAccessRepositoryStatus::error;
    }

    user = User{
        .id = column_text(statement.get(), 0),
        .login = column_text(statement.get(), 1),
        .display_name = column_text(statement.get(), 2),
        .enabled = sqlite3_column_int(statement.get(), 3) != 0,
    };
    return UsersAccessRepositoryStatus::ok;
}

UsersAccessRepositoryStatus SqliteUsersAccessRepository::find_user_by_login(
    const std::string_view login,
    User& user) const {
    if (!ready_) {
        return UsersAccessRepositoryStatus::error;
    }

    Statement statement(
        database_,
        "SELECT id, login, display_name, enabled FROM users WHERE login=?1;");
    if (!statement || !bind_text(statement.get(), 1, login)) {
        set_error(sqlite3_errmsg(database_));
        return UsersAccessRepositoryStatus::error;
    }

    const int result = sqlite3_step(statement.get());
    if (result == SQLITE_DONE) {
        return UsersAccessRepositoryStatus::not_found;
    }
    if (result != SQLITE_ROW) {
        set_error(sqlite3_errmsg(database_));
        return UsersAccessRepositoryStatus::error;
    }

    user = User{
        .id = column_text(statement.get(), 0),
        .login = column_text(statement.get(), 1),
        .display_name = column_text(statement.get(), 2),
        .enabled = sqlite3_column_int(statement.get(), 3) != 0,
    };
    return UsersAccessRepositoryStatus::ok;
}

UsersAccessRepositoryStatus SqliteUsersAccessRepository::insert_permission_set(
    const PermissionSet& permission_set) {
    if (!ready_) {
        return UsersAccessRepositoryStatus::error;
    }

    Statement statement(
        database_,
        "INSERT INTO permission_sets(id, name, capabilities) VALUES(?1, ?2, ?3);");
    if (!statement ||
        !bind_text(statement.get(), 1, permission_set.id) ||
        !bind_text(statement.get(), 2, permission_set.name) ||
        sqlite3_bind_int64(
            statement.get(),
            3,
            static_cast<sqlite3_int64>(capability_mask(permission_set.capabilities))) !=
            SQLITE_OK) {
        set_error(sqlite3_errmsg(database_));
        return UsersAccessRepositoryStatus::error;
    }

    const int result = sqlite3_step(statement.get());
    if (result == SQLITE_DONE) {
        return UsersAccessRepositoryStatus::ok;
    }
    if (result == SQLITE_CONSTRAINT_PRIMARYKEY ||
        result == SQLITE_CONSTRAINT_UNIQUE) {
        return UsersAccessRepositoryStatus::conflict;
    }

    set_error(sqlite3_errmsg(database_));
    return UsersAccessRepositoryStatus::error;
}

UsersAccessRepositoryStatus SqliteUsersAccessRepository::find_permission_set_by_id(
    const std::string_view permission_set_id,
    PermissionSet& permission_set) const {
    if (!ready_) {
        return UsersAccessRepositoryStatus::error;
    }

    Statement statement(
        database_,
        "SELECT id, name, capabilities FROM permission_sets WHERE id=?1;");
    if (!statement || !bind_text(statement.get(), 1, permission_set_id)) {
        set_error(sqlite3_errmsg(database_));
        return UsersAccessRepositoryStatus::error;
    }

    const int result = sqlite3_step(statement.get());
    if (result == SQLITE_DONE) {
        return UsersAccessRepositoryStatus::not_found;
    }
    if (result != SQLITE_ROW) {
        set_error(sqlite3_errmsg(database_));
        return UsersAccessRepositoryStatus::error;
    }

    std::vector<Capability> capabilities;
    const auto raw_mask = sqlite3_column_int64(statement.get(), 2);
    if (raw_mask < 0 ||
        raw_mask > std::numeric_limits<std::uint32_t>::max() ||
        !capabilities_from_mask(static_cast<std::uint32_t>(raw_mask), capabilities)) {
        set_error("invalid capability mask in database");
        return UsersAccessRepositoryStatus::error;
    }

    permission_set = PermissionSet{
        .id = column_text(statement.get(), 0),
        .name = column_text(statement.get(), 1),
        .capabilities = std::move(capabilities),
    };
    return UsersAccessRepositoryStatus::ok;
}

UsersAccessRepositoryStatus SqliteUsersAccessRepository::insert_assignment(
    const AccessAssignment& assignment) {
    if (!ready_) {
        return UsersAccessRepositoryStatus::error;
    }

    Statement statement(
        database_,
        "INSERT INTO access_assignments("
        "user_id, permission_set_id, scope_kind, project_id"
        ") VALUES(?1, ?2, ?3, ?4);");
    if (!statement ||
        !bind_text(statement.get(), 1, assignment.user_id) ||
        !bind_text(statement.get(), 2, assignment.permission_set_id) ||
        sqlite3_bind_int(
            statement.get(),
            3,
            scope_kind_value(assignment.scope.kind)) != SQLITE_OK ||
        !bind_text(statement.get(), 4, assignment.scope.project_id)) {
        set_error(sqlite3_errmsg(database_));
        return UsersAccessRepositoryStatus::error;
    }

    const int result = sqlite3_step(statement.get());
    if (result == SQLITE_DONE) {
        return UsersAccessRepositoryStatus::ok;
    }
    if (result == SQLITE_CONSTRAINT_UNIQUE) {
        return UsersAccessRepositoryStatus::conflict;
    }

    set_error(sqlite3_errmsg(database_));
    return UsersAccessRepositoryStatus::error;
}

UsersAccessRepositoryStatus SqliteUsersAccessRepository::list_assignments_for_user(
    const std::string_view user_id,
    std::vector<AccessAssignment>& assignments) const {
    assignments.clear();
    if (!ready_) {
        return UsersAccessRepositoryStatus::error;
    }

    Statement statement(
        database_,
        "SELECT user_id, permission_set_id, scope_kind, project_id "
        "FROM access_assignments WHERE user_id=?1 ORDER BY rowid;");
    if (!statement || !bind_text(statement.get(), 1, user_id)) {
        set_error(sqlite3_errmsg(database_));
        return UsersAccessRepositoryStatus::error;
    }

    for (;;) {
        const int result = sqlite3_step(statement.get());
        if (result == SQLITE_DONE) {
            return UsersAccessRepositoryStatus::ok;
        }
        if (result != SQLITE_ROW) {
            set_error(sqlite3_errmsg(database_));
            return UsersAccessRepositoryStatus::error;
        }

        AccessScope scope;
        if (!scope_from_row(statement.get(), 2, 3, scope)) {
            set_error("invalid access scope in database");
            return UsersAccessRepositoryStatus::error;
        }

        assignments.push_back(AccessAssignment{
            .user_id = column_text(statement.get(), 0),
            .permission_set_id = column_text(statement.get(), 1),
            .scope = std::move(scope),
        });
    }
}

CredentialRepositoryStatus SqliteUsersAccessRepository::set_credential_verifier(
    const CredentialVerifier& verifier) {
    if (!ready_) {
        return CredentialRepositoryStatus::error;
    }

    Statement statement(
        database_,
        "INSERT INTO credential_verifiers("
        "user_id, algorithm, cost_n, block_size_r, parallelization_p, salt, digest"
        ") VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7) "
        "ON CONFLICT(user_id) DO UPDATE SET "
        "algorithm=excluded.algorithm, cost_n=excluded.cost_n, "
        "block_size_r=excluded.block_size_r, "
        "parallelization_p=excluded.parallelization_p, "
        "salt=excluded.salt, digest=excluded.digest;");
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
        return CredentialRepositoryStatus::error;
    }

    if (sqlite3_step(statement.get()) == SQLITE_DONE) {
        return CredentialRepositoryStatus::ok;
    }

    set_error(sqlite3_errmsg(database_));
    return CredentialRepositoryStatus::error;
}

CredentialRepositoryStatus SqliteUsersAccessRepository::find_credential_verifier(
    const std::string_view user_id,
    CredentialVerifier& verifier) const {
    if (!ready_) {
        return CredentialRepositoryStatus::error;
    }

    Statement statement(
        database_,
        "SELECT user_id, algorithm, cost_n, block_size_r, parallelization_p, salt, digest "
        "FROM credential_verifiers WHERE user_id=?1;");
    if (!statement || !bind_text(statement.get(), 1, user_id)) {
        set_error(sqlite3_errmsg(database_));
        return CredentialRepositoryStatus::error;
    }

    const int result = sqlite3_step(statement.get());
    if (result == SQLITE_DONE) {
        return CredentialRepositoryStatus::not_found;
    }
    if (result != SQLITE_ROW) {
        set_error(sqlite3_errmsg(database_));
        return CredentialRepositoryStatus::error;
    }

    const auto cost_n = sqlite3_column_int64(statement.get(), 2);
    const auto block_size_r = sqlite3_column_int64(statement.get(), 3);
    const auto parallelization_p = sqlite3_column_int64(statement.get(), 4);
    if (cost_n <= 0 ||
        block_size_r <= 0 ||
        parallelization_p <= 0 ||
        static_cast<std::uint64_t>(block_size_r) >
            std::numeric_limits<std::uint32_t>::max() ||
        static_cast<std::uint64_t>(parallelization_p) >
            std::numeric_limits<std::uint32_t>::max()) {
        set_error("invalid credential parameters in database");
        return CredentialRepositoryStatus::error;
    }

    verifier = CredentialVerifier{
        .user_id = column_text(statement.get(), 0),
        .algorithm = column_text(statement.get(), 1),
        .cost_n = static_cast<std::uint64_t>(cost_n),
        .block_size_r = static_cast<std::uint32_t>(block_size_r),
        .parallelization_p = static_cast<std::uint32_t>(parallelization_p),
        .salt = column_blob(statement.get(), 5),
        .digest = column_blob(statement.get(), 6),
    };
    return CredentialRepositoryStatus::ok;
}

SecurityAuditRepositoryStatus SqliteUsersAccessRepository::append_security_audit(
    const SecurityAuditRecord& record) {
    if (!ready_) {
        return SecurityAuditRepositoryStatus::error;
    }

    Statement statement(
        database_,
        "INSERT INTO security_audit("
        "occurred_at_unix_ms, event_type, actor_user_id, subject_user_id"
        ") VALUES(?1, ?2, ?3, ?4);");
    if (!statement ||
        sqlite3_bind_int64(statement.get(), 1, record.occurred_at_unix_ms) != SQLITE_OK ||
        !bind_text(statement.get(), 2, audit_event_name(record.event)) ||
        !bind_text(statement.get(), 3, record.actor_user_id) ||
        !bind_text(statement.get(), 4, record.subject_user_id)) {
        set_error(sqlite3_errmsg(database_));
        return SecurityAuditRepositoryStatus::error;
    }

    if (sqlite3_step(statement.get()) == SQLITE_DONE) {
        return SecurityAuditRepositoryStatus::ok;
    }

    set_error(sqlite3_errmsg(database_));
    return SecurityAuditRepositoryStatus::error;
}

SecurityAuditRepositoryStatus SqliteUsersAccessRepository::list_security_audit(
    std::vector<SecurityAuditRecord>& records) const {
    records.clear();
    if (!ready_) {
        return SecurityAuditRepositoryStatus::error;
    }

    Statement statement(
        database_,
        "SELECT sequence, occurred_at_unix_ms, event_type, actor_user_id, subject_user_id "
        "FROM security_audit ORDER BY sequence;");
    if (!statement) {
        set_error(sqlite3_errmsg(database_));
        return SecurityAuditRepositoryStatus::error;
    }

    for (;;) {
        const int result = sqlite3_step(statement.get());
        if (result == SQLITE_DONE) {
            return SecurityAuditRepositoryStatus::ok;
        }
        if (result != SQLITE_ROW) {
            set_error(sqlite3_errmsg(database_));
            return SecurityAuditRepositoryStatus::error;
        }

        SecurityAuditEventType event;
        if (!audit_event_from_name(column_text(statement.get(), 2), event)) {
            set_error("unknown security audit event in database");
            return SecurityAuditRepositoryStatus::error;
        }

        records.push_back(SecurityAuditRecord{
            .sequence = static_cast<std::uint64_t>(sqlite3_column_int64(statement.get(), 0)),
            .occurred_at_unix_ms = sqlite3_column_int64(statement.get(), 1),
            .event = event,
            .actor_user_id = column_text(statement.get(), 3),
            .subject_user_id = column_text(statement.get(), 4),
        });
    }
}

BootstrapStoreStatus SqliteUsersAccessRepository::bootstrap_first_admin(
    const BootstrapAdminRecord& record) {
    if (!ready_) {
        return BootstrapStoreStatus::error;
    }
    if (!execute("BEGIN IMMEDIATE;")) {
        return BootstrapStoreStatus::error;
    }

    bool has_users = false;
    {
        Statement count_statement(database_, "SELECT COUNT(*) FROM users;");
        if (!count_statement || sqlite3_step(count_statement.get()) != SQLITE_ROW) {
            set_error(sqlite3_errmsg(database_));
            (void)execute("ROLLBACK;");
            return BootstrapStoreStatus::error;
        }
        has_users = sqlite3_column_int64(count_statement.get(), 0) != 0;
    }
    if (has_users) {
        (void)execute("ROLLBACK;");
        return BootstrapStoreStatus::already_initialized;
    }

    const auto user_status = insert_user(record.user);
    const auto permission_status =
        user_status == UsersAccessRepositoryStatus::ok
        ? insert_permission_set(record.permission_set)
        : UsersAccessRepositoryStatus::error;
    const auto assignment_status =
        permission_status == UsersAccessRepositoryStatus::ok
        ? insert_assignment(record.assignment)
        : UsersAccessRepositoryStatus::error;
    const auto credential_status =
        assignment_status == UsersAccessRepositoryStatus::ok
        ? set_credential_verifier(record.credential)
        : CredentialRepositoryStatus::error;
    const auto audit_status =
        credential_status == CredentialRepositoryStatus::ok
        ? append_security_audit(record.audit)
        : SecurityAuditRepositoryStatus::error;

    const bool success =
        user_status == UsersAccessRepositoryStatus::ok &&
        permission_status == UsersAccessRepositoryStatus::ok &&
        assignment_status == UsersAccessRepositoryStatus::ok &&
        credential_status == CredentialRepositoryStatus::ok &&
        audit_status == SecurityAuditRepositoryStatus::ok;

    if (!success) {
        (void)execute("ROLLBACK;");
        if (user_status == UsersAccessRepositoryStatus::conflict ||
            permission_status == UsersAccessRepositoryStatus::conflict ||
            assignment_status == UsersAccessRepositoryStatus::conflict ||
            credential_status == CredentialRepositoryStatus::conflict) {
            return BootstrapStoreStatus::conflict;
        }
        return BootstrapStoreStatus::error;
    }

    if (!execute("COMMIT;")) {
        (void)execute("ROLLBACK;");
        return BootstrapStoreStatus::error;
    }
    return BootstrapStoreStatus::ok;
}

}  // namespace dispatcher::users_access
