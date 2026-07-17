#pragma once

#include <dispatcher/storage/sqlite/sqlite_alarm_event_storage.hpp>
#include <dispatcher/storage/sqlite/sqlite_configuration_snapshot_storage.hpp>
#include <dispatcher/storage/sqlite/sqlite_database.hpp>
#include <dispatcher/storage/sqlite/sqlite_history_storage.hpp>
#include <dispatcher/storage/sqlite/sqlite_migration.hpp>

#include <filesystem>
#include <vector>

namespace dispatcher::storage::sqlite
{
    class SqliteStorageContext final
    {
    public:
        explicit SqliteStorageContext(
            SqliteDatabase database
        );

        SqliteStorageContext(
            const SqliteStorageContext&
        ) = delete;

        SqliteStorageContext& operator=(
            const SqliteStorageContext&
            ) = delete;

        SqliteStorageContext(
            SqliteStorageContext&& other
        ) noexcept = default;

        SqliteStorageContext& operator=(
            SqliteStorageContext&& other
            ) noexcept = default;

        [[nodiscard]] static SqliteStorageContext open_in_memory();

        [[nodiscard]] static SqliteStorageContext open_file(
            const std::filesystem::path& path
        );

        [[nodiscard]] static std::vector<SqliteMigration> migrations();

        void apply_schema();

        [[nodiscard]] bool is_open() const noexcept;

        [[nodiscard]] SqliteDatabase& database() noexcept;

        [[nodiscard]] const SqliteDatabase& database() const noexcept;

        [[nodiscard]] SqliteHistoryStorage history_storage();

        [[nodiscard]] SqliteAlarmEventStorage alarm_event_storage();

        [[nodiscard]] SqliteConfigurationSnapshotStorage configuration_snapshot_storage();

    private:
        SqliteDatabase database_;
    };
}