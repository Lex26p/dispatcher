#include <dispatcher/storage/sqlite/sqlite_migration_runner.hpp>

#include <dispatcher/storage/sqlite/sqlite_error.hpp>

#include <algorithm>
#include <sstream>

namespace dispatcher::storage::sqlite
{
    SqliteMigrationRunner::SqliteMigrationRunner(
        SqliteDatabase& database
    )
        : database_(
            &database
        )
    {
    }

    void SqliteMigrationRunner::ensure_migration_table()
    {
        database_->execute(
            "CREATE TABLE IF NOT EXISTS schema_migrations ("
            "version INTEGER PRIMARY KEY NOT NULL,"
            "name TEXT NOT NULL,"
            "applied_at_utc TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))"
            ");"
        );
    }

    bool SqliteMigrationRunner::has_migration(
        std::int64_t version
    )
    {
        ensure_migration_table();

        const auto sql =
            "SELECT COUNT(*) FROM schema_migrations WHERE version = "
            + std::to_string(
                version
            )
            + ";";

        return database_->query_int64(
            sql
        ) > 0;
    }

    std::int64_t SqliteMigrationRunner::applied_migration_count()
    {
        ensure_migration_table();

        return database_->query_int64(
            "SELECT COUNT(*) FROM schema_migrations;"
        );
    }

    std::int64_t SqliteMigrationRunner::latest_applied_version()
    {
        ensure_migration_table();

        return database_->query_int64(
            "SELECT COALESCE(MAX(version), 0) FROM schema_migrations;"
        );
    }

    void SqliteMigrationRunner::apply(
        const SqliteMigration& migration
    )
    {
        ensure_migration_table();
        validate_migration(
            migration
        );

        if (has_migration(
            migration.version
        ))
        {
            return;
        }

        try
        {
            database_->execute(
                "BEGIN;"
            );

            database_->execute(
                migration.sql
            );

            record_migration(
                migration
            );

            database_->execute(
                "COMMIT;"
            );
        }
        catch (...)
        {
            try
            {
                database_->execute(
                    "ROLLBACK;"
                );
            }
            catch (...)
            {
            }

            throw;
        }
    }

    void SqliteMigrationRunner::apply_all(
        const std::vector<SqliteMigration>& migrations
    )
    {
        auto sorted_migrations =
            migrations;

        std::sort(
            sorted_migrations.begin(),
            sorted_migrations.end(),
            [](const SqliteMigration& left, const SqliteMigration& right)
            {
                return left.version < right.version;
            }
        );

        for (const auto& migration : sorted_migrations)
        {
            apply(
                migration
            );
        }
    }

    void SqliteMigrationRunner::validate_migration(
        const SqliteMigration& migration
    )
    {
        if (migration.version <= 0)
        {
            throw SqliteError(
                "SQLite migration version must be positive."
            );
        }

        if (migration.name.empty())
        {
            throw SqliteError(
                "SQLite migration name must not be empty."
            );
        }

        if (migration.sql.empty())
        {
            throw SqliteError(
                "SQLite migration SQL must not be empty."
            );
        }
    }

    std::string SqliteMigrationRunner::escape_sql_string(
        std::string_view value
    )
    {
        std::string escaped;
        escaped.reserve(
            value.size()
        );

        for (const auto character : value)
        {
            escaped += character;

            if (character == '\'')
            {
                escaped += '\'';
            }
        }

        return escaped;
    }

    void SqliteMigrationRunner::record_migration(
        const SqliteMigration& migration
    )
    {
        std::ostringstream sql;

        sql
            << "INSERT INTO schema_migrations (version, name) VALUES ("
            << migration.version
            << ", '"
            << escape_sql_string(
                migration.name
            )
            << "');";

        database_->execute(
            sql.str()
        );
    }
}