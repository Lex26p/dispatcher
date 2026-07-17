#pragma once

#include <dispatcher/storage/sqlite/sqlite_database.hpp>
#include <dispatcher/storage/sqlite/sqlite_migration.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace dispatcher::storage::sqlite
{
    class SqliteMigrationRunner final
    {
    public:
        explicit SqliteMigrationRunner(
            SqliteDatabase& database
        );

        void ensure_migration_table();

        [[nodiscard]] bool has_migration(
            std::int64_t version
        );

        [[nodiscard]] std::int64_t applied_migration_count();

        [[nodiscard]] std::int64_t latest_applied_version();

        void apply(
            const SqliteMigration& migration
        );

        void apply_all(
            const std::vector<SqliteMigration>& migrations
        );

    private:
        SqliteDatabase* database_;

        static void validate_migration(
            const SqliteMigration& migration
        );

        [[nodiscard]] static std::string escape_sql_string(
            std::string_view value
        );

        void record_migration(
            const SqliteMigration& migration
        );
    };
}