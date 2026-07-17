#include <dispatcher/storage/sqlite/sqlite_database.hpp>

#include <dispatcher/storage/sqlite/sqlite_error.hpp>

#include <sqlite3.h>

#include <utility>

namespace dispatcher::storage::sqlite
{
    SqliteDatabase::SqliteDatabase(
        sqlite3* database,
        std::string path
    )
        : database_(
            database
        )
        , path_(
            std::move(path)
        )
    {
    }

    SqliteDatabase::~SqliteDatabase()
    {
        close();
    }

    SqliteDatabase::SqliteDatabase(
        SqliteDatabase&& other
    ) noexcept
        : database_(
            other.database_
        )
        , path_(
            std::move(other.path_)
        )
    {
        other.database_ = nullptr;
    }

    SqliteDatabase& SqliteDatabase::operator=(
        SqliteDatabase&& other
        ) noexcept
    {
        if (this != &other)
        {
            close();

            database_ = other.database_;
            path_ = std::move(
                other.path_
            );

            other.database_ = nullptr;
        }

        return *this;
    }

    SqliteDatabase SqliteDatabase::open_in_memory()
    {
        return open(
            ":memory:"
        );
    }

    SqliteDatabase SqliteDatabase::open_file(
        const std::filesystem::path& path
    )
    {
        return open(
            path.string()
        );
    }

    bool SqliteDatabase::is_open() const noexcept
    {
        return database_ != nullptr;
    }

    const std::string& SqliteDatabase::path() const noexcept
    {
        return path_;
    }

    void SqliteDatabase::execute(
        std::string_view sql
    )
    {
        ensure_open();

        const std::string sql_text{
            sql
        };

        char* raw_error_message = nullptr;

        const auto result =
            sqlite3_exec(
                database_,
                sql_text.c_str(),
                nullptr,
                nullptr,
                &raw_error_message
            );

        if (result != SQLITE_OK)
        {
            std::string error_message =
                raw_error_message != nullptr
                ? raw_error_message
                : last_error_message();

            if (raw_error_message != nullptr)
            {
                sqlite3_free(
                    raw_error_message
                );
            }

            throw SqliteError(
                "SQLite execute failed: " + error_message
            );
        }
    }

    std::int64_t SqliteDatabase::query_int64(
        std::string_view sql
    )
    {
        ensure_open();

        const std::string sql_text{
            sql
        };

        sqlite3_stmt* statement = nullptr;

        const auto prepare_result =
            sqlite3_prepare_v2(
                database_,
                sql_text.c_str(),
                -1,
                &statement,
                nullptr
            );

        if (prepare_result != SQLITE_OK)
        {
            throw SqliteError(
                "SQLite prepare failed: " + last_error_message()
            );
        }

        const auto finalize_statement =
            [&statement]()
            {
                if (statement != nullptr)
                {
                    sqlite3_finalize(
                        statement
                    );

                    statement = nullptr;
                }
            };

        const auto step_result =
            sqlite3_step(
                statement
            );

        if (step_result != SQLITE_ROW)
        {
            finalize_statement();

            throw SqliteError(
                "SQLite query returned no row."
            );
        }

        const auto value =
            sqlite3_column_int64(
                statement,
                0
            );

        finalize_statement();

        return value;
    }

    void SqliteDatabase::close() noexcept
    {
        if (database_ != nullptr)
        {
            sqlite3_close(
                database_
            );

            database_ = nullptr;
        }
    }

    SqliteDatabase SqliteDatabase::open(
        const std::string& path
    )
    {
        sqlite3* database = nullptr;

        const auto result =
            sqlite3_open_v2(
                path.c_str(),
                &database,
                SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                nullptr
            );

        if (result != SQLITE_OK)
        {
            std::string message =
                database != nullptr
                ? sqlite3_errmsg(
                    database
                )
                : "unknown SQLite open error";

            if (database != nullptr)
            {
                sqlite3_close(
                    database
                );
            }

            throw SqliteError(
                "SQLite open failed: " + message
            );
        }

        return SqliteDatabase(
            database,
            path
        );
    }

    std::string SqliteDatabase::last_error_message() const
    {
        if (database_ == nullptr)
        {
            return "database is not open";
        }

        return sqlite3_errmsg(
            database_
        );
    }

    void SqliteDatabase::ensure_open() const
    {
        if (database_ == nullptr)
        {
            throw SqliteError(
                "SQLite database is not open."
            );
        }
    }

    sqlite3* SqliteDatabase::native_handle() const noexcept
    {
        return database_;
    }
}
