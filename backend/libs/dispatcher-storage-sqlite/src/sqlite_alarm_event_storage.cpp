#include <dispatcher/storage/sqlite/sqlite_alarm_event_storage.hpp>

#include <dispatcher/storage/sqlite/sqlite_error.hpp>
#include <dispatcher/storage/sqlite/sqlite_migration_runner.hpp>

#include <sqlite3.h>

#include <string>
#include <utility>

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

        void bind_null(
            sqlite3_stmt* statement,
            int index
        )
        {
            const auto result =
                sqlite3_bind_null(
                    statement,
                    index
                );

            if (result != SQLITE_OK)
            {
                throw SqliteError(
                    "SQLite bind null failed."
                );
            }
        }

        void bind_optional_text(
            sqlite3_stmt* statement,
            int index,
            const std::optional<std::string>& value
        )
        {
            if (value.has_value())
            {
                bind_text(
                    statement,
                    index,
                    *value
                );
            }
            else
            {
                bind_null(
                    statement,
                    index
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

        std::optional<std::string> column_optional_text(
            sqlite3_stmt* statement,
            int index
        )
        {
            if (sqlite3_column_type(
                statement,
                index
            )
                == SQLITE_NULL)
            {
                return std::nullopt;
            }

            return column_text(
                statement,
                index
            );
        }

        SqliteAlarmEvent event_from_statement(
            sqlite3_stmt* statement
        )
        {
            SqliteAlarmEvent event;

            event.id =
                sqlite3_column_int64(
                    statement,
                    0
                );

            event.alarm_id =
                column_text(
                    statement,
                    1
                );

            event.tag_id =
                column_text(
                    statement,
                    2
                );

            event.event_type =
                column_text(
                    statement,
                    3
                );

            event.severity =
                column_text(
                    statement,
                    4
                );

            event.state =
                column_text(
                    statement,
                    5
                );

            event.message =
                column_text(
                    statement,
                    6
                );

            event.timestamp_utc =
                column_text(
                    statement,
                    7
                );

            event.source =
                column_text(
                    statement,
                    8
                );

            event.operator_id =
                column_optional_text(
                    statement,
                    9
                );

            event.comment =
                column_optional_text(
                    statement,
                    10
                );

            return event;
        }
    }

    SqliteAlarmEventStorage::SqliteAlarmEventStorage(
        SqliteDatabase& database
    )
        : database_(
            &database
        )
    {
    }

    std::vector<SqliteMigration> SqliteAlarmEventStorage::migrations()
    {
        return {
            SqliteMigration{
                200,
                "create_alarm_events",
                "CREATE TABLE IF NOT EXISTS alarm_events ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "alarm_id TEXT NOT NULL,"
                "tag_id TEXT NOT NULL,"
                "event_type TEXT NOT NULL,"
                "severity TEXT NOT NULL,"
                "state TEXT NOT NULL,"
                "message TEXT NOT NULL,"
                "timestamp_utc TEXT NOT NULL,"
                "source TEXT NOT NULL,"
                "operator_id TEXT NULL,"
                "comment TEXT NULL"
                ");"
                "CREATE INDEX IF NOT EXISTS ix_alarm_events_alarm_time "
                "ON alarm_events (alarm_id, timestamp_utc DESC, id DESC);"
                "CREATE INDEX IF NOT EXISTS ix_alarm_events_time "
                "ON alarm_events (timestamp_utc DESC, id DESC);"
                "CREATE INDEX IF NOT EXISTS ix_alarm_events_tag_time "
                "ON alarm_events (tag_id, timestamp_utc DESC, id DESC);"
            }
        };
    }

    void SqliteAlarmEventStorage::apply_schema()
    {
        SqliteMigrationRunner runner{
            *database_
        };

        runner.apply_all(
            migrations()
        );
    }

    std::int64_t SqliteAlarmEventStorage::append(
        const SqliteAlarmEvent& event
    )
    {
        validate_event(
            event
        );

        Statement statement{
            database_->native_handle(),
            "INSERT INTO alarm_events ("
            "alarm_id,"
            "tag_id,"
            "event_type,"
            "severity,"
            "state,"
            "message,"
            "timestamp_utc,"
            "source,"
            "operator_id,"
            "comment"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"
        };

        bind_text(
            statement.get(),
            1,
            event.alarm_id
        );

        bind_text(
            statement.get(),
            2,
            event.tag_id
        );

        bind_text(
            statement.get(),
            3,
            event.event_type
        );

        bind_text(
            statement.get(),
            4,
            event.severity
        );

        bind_text(
            statement.get(),
            5,
            event.state
        );

        bind_text(
            statement.get(),
            6,
            event.message
        );

        bind_text(
            statement.get(),
            7,
            event.timestamp_utc
        );

        bind_text(
            statement.get(),
            8,
            event.source
        );

        bind_optional_text(
            statement.get(),
            9,
            event.operator_id
        );

        bind_optional_text(
            statement.get(),
            10,
            event.comment
        );

        const auto result =
            sqlite3_step(
                statement.get()
            );

        if (result != SQLITE_DONE)
        {
            throw SqliteError(
                std::string("SQLite alarm event insert failed: ")
                + sqlite3_errmsg(
                    database_->native_handle()
                )
            );
        }

        return sqlite3_last_insert_rowid(
            database_->native_handle()
        );
    }

    std::int64_t SqliteAlarmEventStorage::count()
    {
        return database_->query_int64(
            "SELECT COUNT(*) FROM alarm_events;"
        );
    }

    std::int64_t SqliteAlarmEventStorage::count_for_alarm(
        std::string_view alarm_id
    )
    {
        Statement statement{
            database_->native_handle(),
            "SELECT COUNT(*) FROM alarm_events WHERE alarm_id = ?;"
        };

        bind_text(
            statement.get(),
            1,
            alarm_id
        );

        const auto result =
            sqlite3_step(
                statement.get()
            );

        if (result != SQLITE_ROW)
        {
            throw SqliteError(
                "SQLite alarm event count query returned no row."
            );
        }

        return sqlite3_column_int64(
            statement.get(),
            0
        );
    }

    std::optional<SqliteAlarmEvent> SqliteAlarmEventStorage::latest_for_alarm(
        std::string_view alarm_id
    )
    {
        const auto events =
            read_recent_for_alarm(
                alarm_id,
                1
            );

        if (events.empty())
        {
            return std::nullopt;
        }

        return events.front();
    }

    std::vector<SqliteAlarmEvent> SqliteAlarmEventStorage::read_recent_for_alarm(
        std::string_view alarm_id,
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
            "alarm_id,"
            "tag_id,"
            "event_type,"
            "severity,"
            "state,"
            "message,"
            "timestamp_utc,"
            "source,"
            "operator_id,"
            "comment "
            "FROM alarm_events "
            "WHERE alarm_id = ? "
            "ORDER BY timestamp_utc DESC, id DESC "
            "LIMIT ?;"
        };

        bind_text(
            statement.get(),
            1,
            alarm_id
        );

        bind_int64(
            statement.get(),
            2,
            limit
        );

        std::vector<SqliteAlarmEvent> events;

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
                    std::string("SQLite alarm event read failed: ")
                    + sqlite3_errmsg(
                        database_->native_handle()
                    )
                );
            }

            events.push_back(
                event_from_statement(
                    statement.get()
                )
            );
        }

        return events;
    }

    std::vector<SqliteAlarmEvent> SqliteAlarmEventStorage::read_recent(
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
            "alarm_id,"
            "tag_id,"
            "event_type,"
            "severity,"
            "state,"
            "message,"
            "timestamp_utc,"
            "source,"
            "operator_id,"
            "comment "
            "FROM alarm_events "
            "ORDER BY timestamp_utc DESC, id DESC "
            "LIMIT ?;"
        };

        bind_int64(
            statement.get(),
            1,
            limit
        );

        std::vector<SqliteAlarmEvent> events;

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
                    std::string("SQLite alarm event read recent failed: ")
                    + sqlite3_errmsg(
                        database_->native_handle()
                    )
                );
            }

            events.push_back(
                event_from_statement(
                    statement.get()
                )
            );
        }

        return events;
    }

    void SqliteAlarmEventStorage::validate_event(
        const SqliteAlarmEvent& event
    )
    {
        if (event.alarm_id.empty())
        {
            throw SqliteError(
                "SQLite alarm event alarm_id must not be empty."
            );
        }

        if (event.tag_id.empty())
        {
            throw SqliteError(
                "SQLite alarm event tag_id must not be empty."
            );
        }

        if (event.event_type.empty())
        {
            throw SqliteError(
                "SQLite alarm event event_type must not be empty."
            );
        }

        if (event.severity.empty())
        {
            throw SqliteError(
                "SQLite alarm event severity must not be empty."
            );
        }

        if (event.state.empty())
        {
            throw SqliteError(
                "SQLite alarm event state must not be empty."
            );
        }

        if (event.message.empty())
        {
            throw SqliteError(
                "SQLite alarm event message must not be empty."
            );
        }

        if (event.timestamp_utc.empty())
        {
            throw SqliteError(
                "SQLite alarm event timestamp_utc must not be empty."
            );
        }

        if (event.source.empty())
        {
            throw SqliteError(
                "SQLite alarm event source must not be empty."
            );
        }
    }
}