#pragma once

#include <dispatcher/storage/sqlite/sqlite_configuration_snapshot.hpp>
#include <dispatcher/storage/sqlite/sqlite_database.hpp>
#include <dispatcher/storage/sqlite/sqlite_migration.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace dispatcher::storage::sqlite
{
    class SqliteConfigurationSnapshotStorage final
    {
    public:
        explicit SqliteConfigurationSnapshotStorage(
            SqliteDatabase& database
        );

        [[nodiscard]] static std::vector<SqliteMigration> migrations();

        void apply_schema();

        [[nodiscard]] std::int64_t save(
            const SqliteConfigurationSnapshotRecord& record
        );

        [[nodiscard]] std::int64_t count();

        [[nodiscard]] std::optional<SqliteConfigurationSnapshotRecord> find_by_id(
            std::int64_t id
        );

        [[nodiscard]] std::optional<SqliteConfigurationSnapshotRecord> latest();

        [[nodiscard]] std::vector<SqliteConfigurationSnapshotRecord> read_latest(
            std::int64_t limit
        );

    private:
        SqliteDatabase* database_;

        static void validate_record(
            const SqliteConfigurationSnapshotRecord& record
        );
    };
}