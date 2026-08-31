#include "dispatcher/device_manager/sqlite_metadata_repository.hpp"

#include <sqlite3.h>

#include <string>
#include <utility>

namespace dispatcher::device_manager {
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

[[nodiscard]] bool bind_optional_text(
    sqlite3_stmt* statement,
    const int index,
    const std::optional<std::string>& value) {
    if (!value.has_value()) {
        return sqlite3_bind_null(statement, index) == SQLITE_OK;
    }
    return bind_text(statement, index, *value);
}

[[nodiscard]] std::string column_text(sqlite3_stmt* statement, const int index) {
    const auto* value = sqlite3_column_text(statement, index);
    const int bytes = sqlite3_column_bytes(statement, index);
    if (value == nullptr || bytes <= 0) {
        return {};
    }
    return std::string(
        reinterpret_cast<const char*>(value),
        static_cast<std::size_t>(bytes));
}

[[nodiscard]] std::optional<std::string> column_optional_text(
    sqlite3_stmt* statement,
    const int index) {
    if (sqlite3_column_type(statement, index) == SQLITE_NULL) {
        return std::nullopt;
    }
    return column_text(statement, index);
}

[[nodiscard]] int value_type_value(const MetricValueType type) noexcept {
    switch (type) {
    case MetricValueType::boolean:
        return 0;
    case MetricValueType::signed_integer:
        return 1;
    case MetricValueType::unsigned_integer:
        return 2;
    case MetricValueType::floating_point:
        return 3;
    case MetricValueType::string:
        return 4;
    case MetricValueType::bytes:
        return 5;
    }
    return -1;
}

[[nodiscard]] bool value_type_from_value(
    const int value,
    MetricValueType& type) noexcept {
    switch (value) {
    case 0:
        type = MetricValueType::boolean;
        return true;
    case 1:
        type = MetricValueType::signed_integer;
        return true;
    case 2:
        type = MetricValueType::unsigned_integer;
        return true;
    case 3:
        type = MetricValueType::floating_point;
        return true;
    case 4:
        type = MetricValueType::string;
        return true;
    case 5:
        type = MetricValueType::bytes;
        return true;
    default:
        return false;
    }
}

[[nodiscard]] int metric_kind_value(const MetricKind kind) noexcept {
    return kind == MetricKind::working ? 0 : 1;
}

[[nodiscard]] bool metric_kind_from_value(const int value, MetricKind& kind) noexcept {
    if (value == 0) {
        kind = MetricKind::working;
        return true;
    }
    if (value == 1) {
        kind = MetricKind::state;
        return true;
    }
    return false;
}

}  // namespace

SqliteMetadataRepository::SqliteMetadataRepository(const std::string_view database_path) {
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

SqliteMetadataRepository::~SqliteMetadataRepository() {
    if (database_ != nullptr) {
        sqlite3_close(database_);
    }
}

bool SqliteMetadataRepository::ready() const noexcept {
    return ready_;
}

const std::string& SqliteMetadataRepository::error_message() const noexcept {
    return error_message_;
}

void SqliteMetadataRepository::set_error(std::string message) const {
    error_message_ = std::move(message);
}

bool SqliteMetadataRepository::execute(const std::string_view sql) const {
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
        set_error(message);
        sqlite3_free(message);
    } else {
        set_error(sqlite3_errmsg(database_));
    }
    return false;
}

bool SqliteMetadataRepository::initialize_schema() {
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
    if (version != 0) {
        set_error("unsupported database schema version");
        return false;
    }

    if (!execute("BEGIN IMMEDIATE;")) {
        return false;
    }

    const bool success =
        execute(
            "CREATE TABLE devices ("
            "id TEXT PRIMARY KEY NOT NULL,"
            "name TEXT NOT NULL,"
            "description TEXT NOT NULL,"
            "location TEXT NOT NULL"
            ");") &&
        execute(
            "CREATE TABLE metrics ("
            "id TEXT PRIMARY KEY NOT NULL,"
            "device_id TEXT NULL REFERENCES devices(id) ON DELETE RESTRICT,"
            "name TEXT NOT NULL,"
            "description TEXT NOT NULL,"
            "value_type INTEGER NOT NULL CHECK(value_type BETWEEN 0 AND 5),"
            "unit TEXT NOT NULL,"
            "writable INTEGER NOT NULL CHECK(writable IN (0,1)),"
            "kind INTEGER NOT NULL CHECK(kind IN (0,1)),"
            "state_metric_id TEXT NULL,"
            "CHECK((kind=0 AND state_metric_id IS NOT NULL) OR "
            "      (kind=1 AND state_metric_id IS NULL AND writable=0)),"
            "FOREIGN KEY(state_metric_id) REFERENCES metrics(id) "
            "ON DELETE RESTRICT DEFERRABLE INITIALLY DEFERRED"
            ");") &&
        execute(
            "CREATE TABLE device_projects ("
            "device_id TEXT NOT NULL REFERENCES devices(id) ON DELETE CASCADE,"
            "project_id TEXT NOT NULL CHECK(length(project_id) > 0),"
            "PRIMARY KEY(device_id, project_id)"
            ");") &&
        execute(
            "CREATE TABLE standalone_metric_projects ("
            "metric_id TEXT NOT NULL REFERENCES metrics(id) ON DELETE CASCADE,"
            "project_id TEXT NOT NULL CHECK(length(project_id) > 0),"
            "PRIMARY KEY(metric_id, project_id)"
            ");") &&
        execute("PRAGMA user_version = 1;");

    if (!success) {
        (void)execute("ROLLBACK;");
        return false;
    }
    if (!execute("COMMIT;")) {
        (void)execute("ROLLBACK;");
        return false;
    }
    return true;
}

MetadataRepositoryStatus SqliteMetadataRepository::replace_catalog(
    const DeviceCatalog& catalog) {
    if (!ready_) {
        return MetadataRepositoryStatus::storage_error;
    }
    if (!validate_catalog(catalog).ok()) {
        return MetadataRepositoryStatus::validation_error;
    }

    if (!execute("BEGIN IMMEDIATE;")) {
        return MetadataRepositoryStatus::storage_error;
    }

    const auto rollback = [this]() {
        (void)execute("ROLLBACK;");
        return MetadataRepositoryStatus::storage_error;
    };

    if (!execute("DELETE FROM standalone_metric_projects;") ||
        !execute("DELETE FROM device_projects;") ||
        !execute("DELETE FROM metrics;") ||
        !execute("DELETE FROM devices;")) {
        return rollback();
    }

    Statement device_statement(
        database_,
        "INSERT INTO devices(id, name, description, location) "
        "VALUES(?1, ?2, ?3, ?4);");
    if (!device_statement) {
        set_error(sqlite3_errmsg(database_));
        return rollback();
    }

    for (const auto& device : catalog.devices) {
        sqlite3_reset(device_statement.get());
        sqlite3_clear_bindings(device_statement.get());
        if (!bind_text(device_statement.get(), 1, device.id) ||
            !bind_text(device_statement.get(), 2, device.name) ||
            !bind_text(device_statement.get(), 3, device.description) ||
            !bind_text(device_statement.get(), 4, device.location) ||
            sqlite3_step(device_statement.get()) != SQLITE_DONE) {
            set_error(sqlite3_errmsg(database_));
            return rollback();
        }
    }

    Statement metric_statement(
        database_,
        "INSERT INTO metrics("
        "id, device_id, name, description, value_type, unit, writable, kind, state_metric_id"
        ") VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9);");
    if (!metric_statement) {
        set_error(sqlite3_errmsg(database_));
        return rollback();
    }

    const auto insert_metric = [&](const Metric& metric) {
        sqlite3_reset(metric_statement.get());
        sqlite3_clear_bindings(metric_statement.get());
        if (!bind_text(metric_statement.get(), 1, metric.id) ||
            !bind_optional_text(metric_statement.get(), 2, metric.device_id) ||
            !bind_text(metric_statement.get(), 3, metric.name) ||
            !bind_text(metric_statement.get(), 4, metric.description) ||
            sqlite3_bind_int(
                metric_statement.get(),
                5,
                value_type_value(metric.value_type)) != SQLITE_OK ||
            !bind_text(metric_statement.get(), 6, metric.unit) ||
            sqlite3_bind_int(metric_statement.get(), 7, metric.writable ? 1 : 0) != SQLITE_OK ||
            sqlite3_bind_int(
                metric_statement.get(),
                8,
                metric_kind_value(metric.kind)) != SQLITE_OK ||
            !bind_optional_text(metric_statement.get(), 9, metric.state_metric_id) ||
            sqlite3_step(metric_statement.get()) != SQLITE_DONE) {
            set_error(sqlite3_errmsg(database_));
            return false;
        }
        return true;
    };

    for (const auto& metric : catalog.metrics) {
        if (metric.kind == MetricKind::state && !insert_metric(metric)) {
            return rollback();
        }
    }
    for (const auto& metric : catalog.metrics) {
        if (metric.kind == MetricKind::working && !insert_metric(metric)) {
            return rollback();
        }
    }

    Statement device_project_statement(
        database_,
        "INSERT INTO device_projects(device_id, project_id) VALUES(?1, ?2);");
    if (!device_project_statement) {
        set_error(sqlite3_errmsg(database_));
        return rollback();
    }
    for (const auto& association : catalog.device_projects) {
        sqlite3_reset(device_project_statement.get());
        sqlite3_clear_bindings(device_project_statement.get());
        if (!bind_text(device_project_statement.get(), 1, association.resource_id) ||
            !bind_text(device_project_statement.get(), 2, association.project_id) ||
            sqlite3_step(device_project_statement.get()) != SQLITE_DONE) {
            set_error(sqlite3_errmsg(database_));
            return rollback();
        }
    }

    Statement metric_project_statement(
        database_,
        "INSERT INTO standalone_metric_projects(metric_id, project_id) VALUES(?1, ?2);");
    if (!metric_project_statement) {
        set_error(sqlite3_errmsg(database_));
        return rollback();
    }
    for (const auto& association : catalog.standalone_metric_projects) {
        sqlite3_reset(metric_project_statement.get());
        sqlite3_clear_bindings(metric_project_statement.get());
        if (!bind_text(metric_project_statement.get(), 1, association.resource_id) ||
            !bind_text(metric_project_statement.get(), 2, association.project_id) ||
            sqlite3_step(metric_project_statement.get()) != SQLITE_DONE) {
            set_error(sqlite3_errmsg(database_));
            return rollback();
        }
    }

    if (!execute("COMMIT;")) {
        (void)execute("ROLLBACK;");
        return MetadataRepositoryStatus::storage_error;
    }
    return MetadataRepositoryStatus::ok;
}

MetadataRepositoryStatus SqliteMetadataRepository::load_catalog(
    DeviceCatalog& catalog) const {
    if (!ready_) {
        return MetadataRepositoryStatus::storage_error;
    }

    DeviceCatalog loaded;

    Statement devices_statement(
        database_,
        "SELECT id, name, description, location FROM devices ORDER BY id;");
    if (!devices_statement) {
        set_error(sqlite3_errmsg(database_));
        return MetadataRepositoryStatus::storage_error;
    }
    while (true) {
        const int result = sqlite3_step(devices_statement.get());
        if (result == SQLITE_DONE) {
            break;
        }
        if (result != SQLITE_ROW) {
            set_error(sqlite3_errmsg(database_));
            return MetadataRepositoryStatus::storage_error;
        }
        loaded.devices.push_back(Device{
            .id = column_text(devices_statement.get(), 0),
            .name = column_text(devices_statement.get(), 1),
            .description = column_text(devices_statement.get(), 2),
            .location = column_text(devices_statement.get(), 3),
        });
    }

    Statement metrics_statement(
        database_,
        "SELECT id, device_id, name, description, value_type, unit, writable, kind, "
        "state_metric_id FROM metrics ORDER BY id;");
    if (!metrics_statement) {
        set_error(sqlite3_errmsg(database_));
        return MetadataRepositoryStatus::storage_error;
    }
    while (true) {
        const int result = sqlite3_step(metrics_statement.get());
        if (result == SQLITE_DONE) {
            break;
        }
        if (result != SQLITE_ROW) {
            set_error(sqlite3_errmsg(database_));
            return MetadataRepositoryStatus::storage_error;
        }

        MetricValueType value_type{};
        MetricKind kind{};
        if (!value_type_from_value(sqlite3_column_int(metrics_statement.get(), 4), value_type) ||
            !metric_kind_from_value(sqlite3_column_int(metrics_statement.get(), 7), kind)) {
            set_error("database contains unsupported metric metadata values");
            return MetadataRepositoryStatus::storage_error;
        }

        loaded.metrics.push_back(Metric{
            .id = column_text(metrics_statement.get(), 0),
            .device_id = column_optional_text(metrics_statement.get(), 1),
            .name = column_text(metrics_statement.get(), 2),
            .description = column_text(metrics_statement.get(), 3),
            .value_type = value_type,
            .unit = column_text(metrics_statement.get(), 5),
            .writable = sqlite3_column_int(metrics_statement.get(), 6) != 0,
            .kind = kind,
            .state_metric_id = column_optional_text(metrics_statement.get(), 8),
        });
    }

    Statement device_projects_statement(
        database_,
        "SELECT device_id, project_id FROM device_projects "
        "ORDER BY device_id, project_id;");
    if (!device_projects_statement) {
        set_error(sqlite3_errmsg(database_));
        return MetadataRepositoryStatus::storage_error;
    }
    while (true) {
        const int result = sqlite3_step(device_projects_statement.get());
        if (result == SQLITE_DONE) {
            break;
        }
        if (result != SQLITE_ROW) {
            set_error(sqlite3_errmsg(database_));
            return MetadataRepositoryStatus::storage_error;
        }
        loaded.device_projects.push_back(ProjectAssociation{
            .resource_id = column_text(device_projects_statement.get(), 0),
            .project_id = column_text(device_projects_statement.get(), 1),
        });
    }

    Statement metric_projects_statement(
        database_,
        "SELECT metric_id, project_id FROM standalone_metric_projects "
        "ORDER BY metric_id, project_id;");
    if (!metric_projects_statement) {
        set_error(sqlite3_errmsg(database_));
        return MetadataRepositoryStatus::storage_error;
    }
    while (true) {
        const int result = sqlite3_step(metric_projects_statement.get());
        if (result == SQLITE_DONE) {
            break;
        }
        if (result != SQLITE_ROW) {
            set_error(sqlite3_errmsg(database_));
            return MetadataRepositoryStatus::storage_error;
        }
        loaded.standalone_metric_projects.push_back(ProjectAssociation{
            .resource_id = column_text(metric_projects_statement.get(), 0),
            .project_id = column_text(metric_projects_statement.get(), 1),
        });
    }

    if (!validate_catalog(loaded).ok()) {
        set_error("database contains invalid Device Manager metadata relationships");
        return MetadataRepositoryStatus::storage_error;
    }

    catalog = std::move(loaded);
    return MetadataRepositoryStatus::ok;
}

}  // namespace dispatcher::device_manager
