#include <dispatcher/storage/sqlite/sqlite_configuration_snapshot_storage.hpp>

#include <dispatcher/storage/sqlite/sqlite_error.hpp>
#include <dispatcher/storage/sqlite/sqlite_migration_runner.hpp>

#include <sqlite3.h>

#include <string>

namespace dispatcher::storage::sqlite
{
    namespace
    {
        class Statement final
        {
        public:
            Statement(
                sqlite3* database,
                const char* sql
            )
                : database_(
                    database
                )
            {
                const auto result =
                    sqlite3_prepare_v2(
                        database_,
                        sql,
                        -1,
                        &statement_,
                        nullptr
                    );

                if (result != SQLITE_OK)
                {
                    throw SqliteError(
                        std::string("SQLite prepare failed: ")
                        + sqlite3_errmsg(
                            database_
                        )
                    );
                }
            }

            ~Statement()
            {
                if (statement_ != nullptr)
                {
                    sqlite3_finalize(
                        statement_
                    );
                }
            }

            Statement(
                const Statement&
            ) = delete;

            Statement& operator=(
                const Statement&
                ) = delete;

            [[nodiscard]] sqlite3_stmt* get() const noexcept
            {
                return statement_;
            }

        private:
            sqlite3* database_{ nullptr };
            sqlite3_stmt* statement_{ nullptr };
        };

        void bind_text(
            sqlite3_stmt* statement,
            int index,
            std::string_view value
        )
        {
            const auto result =
                sqlite3_bind_text(
                    statement,
                    index,
                    value.data(),
                    static_cast<int>(
                        value.size()
                        ),
                    SQLITE_TRANSIENT
                );

            if (result != SQLITE_OK)
            {
                throw SqliteError(
                    "SQLite bind text failed."
                );
            }
        }

        void bind_int64(
            sqlite3_stmt* statement,
            int index,
            std::int64_t value
        )
        {
            const auto result =
                sqlite3_bind_int64(
                    statement,
                    index,
                    value
                );

            if (result != SQLITE_OK)
            {
                throw SqliteError(
                    "SQLite bind int64 failed."
                );
            }
        }

        std::string column_text(
            sqlite3_stmt* statement,
            int index
        )
        {
            const auto text =
                sqlite3_column_text(
                    statement,
                    index
                );

            if (text == nullptr)
            {
                return {};
            }

            return reinterpret_cast<const char*>(
                text
                );
        }

        SqliteConfigurationSnapshotRecord record_from_statement(
            sqlite3_stmt* statement
        )
        {
            SqliteConfigurationSnapshotRecord record;

            record.id =
                sqlite3_column_int64(
                    statement,
                    0
                );

            record.snapshot_name =
                column_text(
                    statement,
                    1
                );

            record.schema_version =
                column_text(
                    statement,
                    2
                );

            record.created_at_utc =
                column_text(
                    statement,
                    3
                );

            record.source =
                column_text(
                    statement,
                    4
                );

            record.payload_json =
                column_text(
                    statement,
                    5
                );

            return record;
        }
    }

    SqliteConfigurationSnapshotStorage::SqliteConfigurationSnapshotStorage(
        SqliteDatabase& database
    )
        : database_(
            &database
        )
    {
    }

    std::vector<SqliteMigration> SqliteConfigurationSnapshotStorage::migrations()
    {
        return {
            SqliteMigration{
                300,
                "create_configuration_snapshots",
                "CREATE TABLE IF NOT EXISTS configuration_snapshots ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "snapshot_name TEXT NOT NULL,"
                "schema_version TEXT NOT NULL,"
                "created_at_utc TEXT NOT NULL,"
                "source TEXT NOT NULL,"
                "payload_json TEXT NOT NULL"
                ");"
                "CREATE INDEX IF NOT EXISTS ix_configuration_snapshots_created "
                "ON configuration_snapshots (created_at_utc DESC, id DESC);"
            }
        };
    }

    void SqliteConfigurationSnapshotStorage::apply_schema()
    {
        SqliteMigrationRunner runner{
            *database_
        };

        runner.apply_all(
            migrations()
        );
    }

    std::int64_t SqliteConfigurationSnapshotStorage::save(
        const SqliteConfigurationSnapshotRecord& record
    )
    {
        validate_record(
            record
        );

        Statement statement{
            database_->native_handle(),
            "INSERT INTO configuration_snapshots ("
            "snapshot_name,"
            "schema_version,"
            "created_at_utc,"
            "source,"
            "payload_json"
            ") VALUES (?, ?, ?, ?, ?);"
        };

        bind_text(
            statement.get(),
            1,
            record.snapshot_name
        );

        bind_text(
            statement.get(),
            2,
            record.schema_version
        );

        bind_text(
            statement.get(),
            3,
            record.created_at_utc
        );

        bind_text(
            statement.get(),
            4,
            record.source
        );

        bind_text(
            statement.get(),
            5,
            record.payload_json
        );

        const auto result =
            sqlite3_step(
                statement.get()
            );

        if (result != SQLITE_DONE)
        {
            throw SqliteError(
                std::string("SQLite configuration snapshot insert failed: ")
                + sqlite3_errmsg(
                    database_->native_handle()
                )
            );
        }

        return sqlite3_last_insert_rowid(
            database_->native_handle()
        );
    }

    std::int64_t SqliteConfigurationSnapshotStorage::count()
    {
        return database_->query_int64(
            "SELECT COUNT(*) FROM configuration_snapshots;"
        );
    }

    std::optional<SqliteConfigurationSnapshotRecord>
        SqliteConfigurationSnapshotStorage::find_by_id(
            std::int64_t id
        )
    {
        Statement statement{
            database_->native_handle(),
            "SELECT "
            "id,"
            "snapshot_name,"
            "schema_version,"
            "created_at_utc,"
            "source,"
            "payload_json "
            "FROM configuration_snapshots "
            "WHERE id = ?;"
        };

        bind_int64(
            statement.get(),
            1,
            id
        );

        const auto result =
            sqlite3_step(
                statement.get()
            );

        if (result == SQLITE_DONE)
        {
            return std::nullopt;
        }

        if (result != SQLITE_ROW)
        {
            throw SqliteError(
                std::string("SQLite configuration snapshot find failed: ")
                + sqlite3_errmsg(
                    database_->native_handle()
                )
            );
        }

        return record_from_statement(
            statement.get()
        );
    }

    std::optional<SqliteConfigurationSnapshotRecord>
        SqliteConfigurationSnapshotStorage::latest()
    {
        const auto records =
            read_latest(
                1
            );

        if (records.empty())
        {
            return std::nullopt;
        }

        return records.front();
    }

    std::vector<SqliteConfigurationSnapshotRecord>
        SqliteConfigurationSnapshotStorage::read_latest(
            std::int64_t limit
        )
    {
        if (limit <= 0)
        {
            return {};
        }

        Statement statement{
            database_->native_handle(),
            "SELECT "
            "id,"
            "snapshot_name,"
            "schema_version,"
            "created_at_utc,"
            "source,"
            "payload_json "
            "FROM configuration_snapshots "
            "ORDER BY created_at_utc DESC, id DESC "
            "LIMIT ?;"
        };

        bind_int64(
            statement.get(),
            1,
            limit
        );

        std::vector<SqliteConfigurationSnapshotRecord> records;

        while (true)
        {
            const auto result =
                sqlite3_step(
                    statement.get()
                );

            if (result == SQLITE_DONE)
            {
                break;
            }

            if (result != SQLITE_ROW)
            {
                throw SqliteError(
                    std::string("SQLite configuration snapshot read failed: ")
                    + sqlite3_errmsg(
                        database_->native_handle()
                    )
                );
            }

            records.push_back(
                record_from_statement(
                    statement.get()
                )
            );
        }

        return records;
    }

    void SqliteConfigurationSnapshotStorage::validate_record(
        const SqliteConfigurationSnapshotRecord& record
    )
    {
        if (record.snapshot_name.empty())
        {
            throw SqliteError(
                "SQLite configuration snapshot name must not be empty."
            );
        }

        if (record.schema_version.empty())
        {
            throw SqliteError(
                "SQLite configuration snapshot schema_version must not be empty."
            );
        }

        if (record.created_at_utc.empty())
        {
            throw SqliteError(
                "SQLite configuration snapshot created_at_utc must not be empty."
            );
        }

        if (record.source.empty())
        {
            throw SqliteError(
                "SQLite configuration snapshot source must not be empty."
            );
        }

        if (record.payload_json.empty())
        {
            throw SqliteError(
                "SQLite configuration snapshot payload_json must not be empty."
            );
        }
    }
}