#include <dispatcher/storage/sqlite/sqlite_history_storage.hpp>

#include <dispatcher/storage/sqlite/sqlite_error.hpp>
#include <dispatcher/storage/sqlite/sqlite_migration_runner.hpp>

#include <sqlite3.h>

#include <algorithm>
#include <sstream>
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

        void bind_double(
            sqlite3_stmt* statement,
            int index,
            double value
        )
        {
            const auto result =
                sqlite3_bind_double(
                    statement,
                    index,
                    value
                );

            if (result != SQLITE_OK)
            {
                throw SqliteError(
                    "SQLite bind double failed."
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

        std::optional<double> column_optional_double(
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

            return sqlite3_column_double(
                statement,
                index
            );
        }

        std::optional<bool> column_optional_bool(
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

            return sqlite3_column_int(
                statement,
                index
            )
                != 0;
        }

        SqliteHistorySample sample_from_statement(
            sqlite3_stmt* statement
        )
        {
            SqliteHistorySample sample;

            sample.id =
                sqlite3_column_int64(
                    statement,
                    0
                );

            sample.tag_id =
                column_text(
                    statement,
                    1
                );

            sample.timestamp_utc =
                column_text(
                    statement,
                    2
                );

            sample.value_type =
                SqliteHistoryStorage::value_type_from_text(
                    sqlite3_column_text(
                        statement,
                        3
                    )
                );

            sample.numeric_value =
                column_optional_double(
                    statement,
                    4
                );

            sample.text_value =
                column_optional_text(
                    statement,
                    5
                );

            sample.bool_value =
                column_optional_bool(
                    statement,
                    6
                );

            sample.quality =
                column_text(
                    statement,
                    7
                );

            sample.source =
                column_text(
                    statement,
                    8
                );

            return sample;
        }
    }

    SqliteHistoryStorage::SqliteHistoryStorage(
        SqliteDatabase& database
    )
        : database_(
            &database
        )
    {
    }

    std::vector<SqliteMigration> SqliteHistoryStorage::migrations()
    {
        return {
            SqliteMigration{
                100,
                "create_history_samples",
                "CREATE TABLE IF NOT EXISTS history_samples ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "tag_id TEXT NOT NULL,"
                "timestamp_utc TEXT NOT NULL,"
                "value_type TEXT NOT NULL,"
                "numeric_value REAL NULL,"
                "text_value TEXT NULL,"
                "bool_value INTEGER NULL,"
                "quality TEXT NOT NULL,"
                "source TEXT NOT NULL"
                ");"
                "CREATE INDEX IF NOT EXISTS ix_history_samples_tag_time "
                "ON history_samples (tag_id, timestamp_utc DESC, id DESC);"
            }
        };
    }

    void SqliteHistoryStorage::apply_schema()
    {
        SqliteMigrationRunner runner{
            *database_
        };

        runner.apply_all(
            migrations()
        );
    }

    std::int64_t SqliteHistoryStorage::append(
        const SqliteHistorySample& sample
    )
    {
        validate_sample(
            sample
        );

        Statement statement{
            database_->native_handle(),
            "INSERT INTO history_samples ("
            "tag_id,"
            "timestamp_utc,"
            "value_type,"
            "numeric_value,"
            "text_value,"
            "bool_value,"
            "quality,"
            "source"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?);"
        };

        bind_text(
            statement.get(),
            1,
            sample.tag_id
        );

        bind_text(
            statement.get(),
            2,
            sample.timestamp_utc
        );

        bind_text(
            statement.get(),
            3,
            value_type_to_text(
                sample.value_type
            )
        );

        if (sample.numeric_value.has_value())
        {
            bind_double(
                statement.get(),
                4,
                *sample.numeric_value
            );
        }
        else
        {
            bind_null(
                statement.get(),
                4
            );
        }

        if (sample.text_value.has_value())
        {
            bind_text(
                statement.get(),
                5,
                *sample.text_value
            );
        }
        else
        {
            bind_null(
                statement.get(),
                5
            );
        }

        if (sample.bool_value.has_value())
        {
            bind_int64(
                statement.get(),
                6,
                *sample.bool_value
                ? 1
                : 0
            );
        }
        else
        {
            bind_null(
                statement.get(),
                6
            );
        }

        bind_text(
            statement.get(),
            7,
            sample.quality
        );

        bind_text(
            statement.get(),
            8,
            sample.source
        );

        const auto result =
            sqlite3_step(
                statement.get()
            );

        if (result != SQLITE_DONE)
        {
            throw SqliteError(
                std::string("SQLite history insert failed: ")
                + sqlite3_errmsg(
                    database_->native_handle()
                )
            );
        }

        return sqlite3_last_insert_rowid(
            database_->native_handle()
        );
    }

    std::int64_t SqliteHistoryStorage::count()
    {
        return database_->query_int64(
            "SELECT COUNT(*) FROM history_samples;"
        );
    }

    std::int64_t SqliteHistoryStorage::count_for_tag(
        std::string_view tag_id
    )
    {
        Statement statement{
            database_->native_handle(),
            "SELECT COUNT(*) FROM history_samples WHERE tag_id = ?;"
        };

        bind_text(
            statement.get(),
            1,
            tag_id
        );

        const auto result =
            sqlite3_step(
                statement.get()
            );

        if (result != SQLITE_ROW)
        {
            throw SqliteError(
                "SQLite history count query returned no row."
            );
        }

        return sqlite3_column_int64(
            statement.get(),
            0
        );
    }

    std::optional<SqliteHistorySample> SqliteHistoryStorage::latest_for_tag(
        std::string_view tag_id
    )
    {
        const auto samples =
            read_recent_for_tag(
                tag_id,
                1
            );

        if (samples.empty())
        {
            return std::nullopt;
        }

        return samples.front();
    }

    std::vector<SqliteHistorySample> SqliteHistoryStorage::read_recent_for_tag(
        std::string_view tag_id,
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
            "tag_id,"
            "timestamp_utc,"
            "value_type,"
            "numeric_value,"
            "text_value,"
            "bool_value,"
            "quality,"
            "source "
            "FROM history_samples "
            "WHERE tag_id = ? "
            "ORDER BY timestamp_utc DESC, id DESC "
            "LIMIT ?;"
        };

        bind_text(
            statement.get(),
            1,
            tag_id
        );

        bind_int64(
            statement.get(),
            2,
            limit
        );

        std::vector<SqliteHistorySample> samples;

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
                    std::string("SQLite history read failed: ")
                    + sqlite3_errmsg(
                        database_->native_handle()
                    )
                );
            }

            samples.push_back(
                sample_from_statement(
                    statement.get()
                )
            );
        }

        return samples;
    }

    void SqliteHistoryStorage::validate_sample(
        const SqliteHistorySample& sample
    )
    {
        if (sample.tag_id.empty())
        {
            throw SqliteError(
                "SQLite history sample tag_id must not be empty."
            );
        }

        if (sample.timestamp_utc.empty())
        {
            throw SqliteError(
                "SQLite history sample timestamp_utc must not be empty."
            );
        }

        if (sample.quality.empty())
        {
            throw SqliteError(
                "SQLite history sample quality must not be empty."
            );
        }

        if (sample.source.empty())
        {
            throw SqliteError(
                "SQLite history sample source must not be empty."
            );
        }

        switch (sample.value_type)
        {
        case SqliteHistoryValueType::numeric:
            if (!sample.numeric_value.has_value())
            {
                throw SqliteError(
                    "SQLite numeric history sample requires numeric_value."
                );
            }
            break;

        case SqliteHistoryValueType::text:
            if (!sample.text_value.has_value())
            {
                throw SqliteError(
                    "SQLite text history sample requires text_value."
                );
            }
            break;

        case SqliteHistoryValueType::boolean:
            if (!sample.bool_value.has_value())
            {
                throw SqliteError(
                    "SQLite boolean history sample requires bool_value."
                );
            }
            break;
        }
    }

    const char* SqliteHistoryStorage::value_type_to_text(
        SqliteHistoryValueType value_type
    )
    {
        switch (value_type)
        {
        case SqliteHistoryValueType::numeric:
            return "numeric";

        case SqliteHistoryValueType::text:
            return "text";

        case SqliteHistoryValueType::boolean:
            return "boolean";
        }

        return "numeric";
    }

    SqliteHistoryValueType SqliteHistoryStorage::value_type_from_text(
        const unsigned char* value
    )
    {
        if (value == nullptr)
        {
            throw SqliteError(
                "SQLite history value_type is null."
            );
        }

        const std::string text{
            reinterpret_cast<const char*>(
                value
            )
        };

        if (text == "numeric")
        {
            return SqliteHistoryValueType::numeric;
        }

        if (text == "text")
        {
            return SqliteHistoryValueType::text;
        }

        if (text == "boolean")
        {
            return SqliteHistoryValueType::boolean;
        }

        throw SqliteError(
            "Unknown SQLite history value_type: " + text
        );
    }
}