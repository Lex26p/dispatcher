#include "dispatcher/project_manager/sqlite_project_repository.hpp"

#include <sqlite3.h>

#include <string>
#include <utility>

namespace dispatcher::project_manager {
namespace {

constexpr int kSchemaVersion = 1;

class Statement final {
public:
    Statement(sqlite3* database, const char* sql) {
        if (database != nullptr && sqlite3_prepare_v2(database, sql, -1, &statement_, nullptr) == SQLITE_OK) {
            ready_ = true;
        }
    }

    ~Statement() {
        if (statement_ != nullptr) {
            sqlite3_finalize(statement_);
        }
    }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    [[nodiscard]] bool ready() const noexcept {
        return ready_;
    }

    [[nodiscard]] sqlite3_stmt* get() const noexcept {
        return statement_;
    }

private:
    sqlite3_stmt* statement_{nullptr};
    bool ready_{false};
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

[[nodiscard]] std::string column_text(sqlite3_stmt* statement, const int index) {
    const auto* text = reinterpret_cast<const char*>(sqlite3_column_text(statement, index));
    const int size = sqlite3_column_bytes(statement, index);

    if (text == nullptr || size <= 0) {
        return {};
    }

    return std::string(text, static_cast<std::size_t>(size));
}

[[nodiscard]] bool execute(sqlite3* database, const char* sql, std::string& error_message) {
    char* sqlite_error = nullptr;
    const int status = sqlite3_exec(database, sql, nullptr, nullptr, &sqlite_error);

    if (status == SQLITE_OK) {
        return true;
    }

    if (sqlite_error != nullptr) {
        error_message.assign(sqlite_error);
        sqlite3_free(sqlite_error);
    } else if (database != nullptr) {
        error_message.assign(sqlite3_errmsg(database));
    } else {
        error_message.assign("SQLite operation failed");
    }

    return false;
}

}  // namespace

SqliteProjectRepository::SqliteProjectRepository(const std::string_view database_path) {
    if (database_path.empty()) {
        set_error("Project Manager database path must not be empty");
        return;
    }

    const std::string path{database_path};
    const int open_status = sqlite3_open_v2(
        path.c_str(),
        &database_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
        nullptr);

    if (open_status != SQLITE_OK) {
        set_error(database_ != nullptr ? sqlite3_errmsg(database_) : "Failed to open SQLite database");
        return;
    }

    sqlite3_extended_result_codes(database_, 1);
    sqlite3_busy_timeout(database_, 5000);

    ready_ = initialize_schema();
}

SqliteProjectRepository::~SqliteProjectRepository() {
    if (database_ != nullptr) {
        sqlite3_close(database_);
    }
}

bool SqliteProjectRepository::ready() const noexcept {
    return ready_;
}

std::string_view SqliteProjectRepository::error_message() const noexcept {
    return error_message_;
}

ProjectRepositoryStatus SqliteProjectRepository::insert(const Project& project) {
    if (!ready_) {
        return ProjectRepositoryStatus::error;
    }

    Statement statement{
        database_,
        "INSERT INTO projects(id, name, description) VALUES(?1, ?2, ?3);"};

    if (!statement.ready() ||
        !bind_text(statement.get(), 1, project.id) ||
        !bind_text(statement.get(), 2, project.name) ||
        !bind_text(statement.get(), 3, project.description)) {
        set_error(sqlite3_errmsg(database_));
        return ProjectRepositoryStatus::error;
    }

    const int status = sqlite3_step(statement.get());

    if (status == SQLITE_DONE) {
        return ProjectRepositoryStatus::ok;
    }

    if (status == SQLITE_CONSTRAINT ||
        status == SQLITE_CONSTRAINT_PRIMARYKEY ||
        status == SQLITE_CONSTRAINT_UNIQUE) {
        return ProjectRepositoryStatus::conflict;
    }

    set_error(sqlite3_errmsg(database_));
    return ProjectRepositoryStatus::error;
}

ProjectRepositoryStatus SqliteProjectRepository::update(const Project& project) {
    if (!ready_) {
        return ProjectRepositoryStatus::error;
    }

    Statement statement{
        database_,
        "UPDATE projects SET name = ?1, description = ?2 WHERE id = ?3;"};

    if (!statement.ready() ||
        !bind_text(statement.get(), 1, project.name) ||
        !bind_text(statement.get(), 2, project.description) ||
        !bind_text(statement.get(), 3, project.id)) {
        set_error(sqlite3_errmsg(database_));
        return ProjectRepositoryStatus::error;
    }

    const int status = sqlite3_step(statement.get());

    if (status != SQLITE_DONE) {
        set_error(sqlite3_errmsg(database_));
        return ProjectRepositoryStatus::error;
    }

    return sqlite3_changes(database_) == 0
        ? ProjectRepositoryStatus::not_found
        : ProjectRepositoryStatus::ok;
}

ProjectRepositoryStatus SqliteProjectRepository::find_by_id(
    const std::string_view project_id,
    Project& project) const {
    if (!ready_) {
        return ProjectRepositoryStatus::error;
    }

    Statement statement{
        database_,
        "SELECT id, name, description FROM projects WHERE id = ?1;"};

    if (!statement.ready() || !bind_text(statement.get(), 1, project_id)) {
        set_error(sqlite3_errmsg(database_));
        return ProjectRepositoryStatus::error;
    }

    const int status = sqlite3_step(statement.get());

    if (status == SQLITE_DONE) {
        return ProjectRepositoryStatus::not_found;
    }

    if (status != SQLITE_ROW) {
        set_error(sqlite3_errmsg(database_));
        return ProjectRepositoryStatus::error;
    }

    project = Project{
        .id = column_text(statement.get(), 0),
        .name = column_text(statement.get(), 1),
        .description = column_text(statement.get(), 2),
    };

    return ProjectRepositoryStatus::ok;
}

ProjectRepositoryStatus SqliteProjectRepository::list(
    std::vector<Project>& projects) const {
    projects.clear();

    if (!ready_) {
        return ProjectRepositoryStatus::error;
    }

    Statement statement{
        database_,
        "SELECT id, name, description FROM projects ORDER BY rowid;"};

    if (!statement.ready()) {
        set_error(sqlite3_errmsg(database_));
        return ProjectRepositoryStatus::error;
    }

    for (;;) {
        const int status = sqlite3_step(statement.get());

        if (status == SQLITE_DONE) {
            return ProjectRepositoryStatus::ok;
        }

        if (status != SQLITE_ROW) {
            projects.clear();
            set_error(sqlite3_errmsg(database_));
            return ProjectRepositoryStatus::error;
        }

        projects.push_back(Project{
            .id = column_text(statement.get(), 0),
            .name = column_text(statement.get(), 1),
            .description = column_text(statement.get(), 2),
        });
    }
}

bool SqliteProjectRepository::initialize_schema() {
    int version = 0;

    {
        Statement version_statement{database_, "PRAGMA user_version;"};

        if (!version_statement.ready() || sqlite3_step(version_statement.get()) != SQLITE_ROW) {
            set_error(sqlite3_errmsg(database_));
            return false;
        }

        version = sqlite3_column_int(version_statement.get(), 0);
    }

    if (version > kSchemaVersion) {
        set_error("Project Manager database schema is newer than this executable supports");
        return false;
    }

    if (version == kSchemaVersion) {
        return true;
    }

    std::string error;

    if (!execute(database_, "BEGIN IMMEDIATE;", error)) {
        set_error(std::move(error));
        return false;
    }

    const bool schema_created = execute(
        database_,
        "CREATE TABLE IF NOT EXISTS projects("
        "id TEXT PRIMARY KEY NOT NULL,"
        "name TEXT NOT NULL,"
        "description TEXT NOT NULL"
        ");",
        error);

    const bool version_written = schema_created && execute(
        database_,
        "PRAGMA user_version = 1;",
        error);

    const bool committed = version_written && execute(database_, "COMMIT;", error);

    if (!committed) {
        std::string rollback_error;
        (void)execute(database_, "ROLLBACK;", rollback_error);
        set_error(std::move(error));
        return false;
    }

    return true;
}

void SqliteProjectRepository::set_error(std::string message) const {
    error_message_ = std::move(message);
}

}  // namespace dispatcher::project_manager
