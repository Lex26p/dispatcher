#include <dispatcher/storage/sqlite/sqlite_storage_context.hpp>

#include <dispatcher/storage/sqlite/sqlite_migration_runner.hpp>

#include <utility>

namespace dispatcher::storage::sqlite
{
    SqliteStorageContext::SqliteStorageContext(
        SqliteDatabase database
    )
        : database_(
            std::move(
                database
            )
        )
    {
    }

    SqliteStorageContext SqliteStorageContext::open_in_memory()
    {
        return SqliteStorageContext{
            SqliteDatabase::open_in_memory()
        };
    }

    SqliteStorageContext SqliteStorageContext::open_file(
        const std::filesystem::path& path
    )
    {
        return SqliteStorageContext{
            SqliteDatabase::open_file(
                path
            )
        };
    }

    std::vector<SqliteMigration> SqliteStorageContext::migrations()
    {
        std::vector<SqliteMigration> all_migrations;

        auto append_migrations =
            [&all_migrations](std::vector<SqliteMigration> migrations)
            {
                for (auto& migration : migrations)
                {
                    all_migrations.push_back(
                        std::move(
                            migration
                        )
                    );
                }
            };

        append_migrations(
            SqliteHistoryStorage::migrations()
        );

        append_migrations(
            SqliteAlarmEventStorage::migrations()
        );

        append_migrations(
            SqliteConfigurationSnapshotStorage::migrations()
        );

        return all_migrations;
    }

    void SqliteStorageContext::apply_schema()
    {
        SqliteMigrationRunner runner{
            database_
        };

        runner.apply_all(
            migrations()
        );
    }

    bool SqliteStorageContext::is_open() const noexcept
    {
        return database_.is_open();
    }

    SqliteDatabase& SqliteStorageContext::database() noexcept
    {
        return database_;
    }

    const SqliteDatabase& SqliteStorageContext::database() const noexcept
    {
        return database_;
    }

    SqliteHistoryStorage SqliteStorageContext::history_storage()
    {
        return SqliteHistoryStorage{
            database_
        };
    }

    SqliteAlarmEventStorage SqliteStorageContext::alarm_event_storage()
    {
        return SqliteAlarmEventStorage{
            database_
        };
    }

    SqliteConfigurationSnapshotStorage SqliteStorageContext::configuration_snapshot_storage()
    {
        return SqliteConfigurationSnapshotStorage{
            database_
        };
    }
}