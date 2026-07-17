#pragma once

#include <dispatcher/storage/sqlite/sqlite_database.hpp>
#include <dispatcher/storage/sqlite/sqlite_history_sample.hpp>
#include <dispatcher/storage/sqlite/sqlite_migration.hpp>

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace dispatcher::storage::sqlite
{
    class SqliteHistoryStorage final
    {
    public:
        explicit SqliteHistoryStorage(
            SqliteDatabase& database
        );

        [[nodiscard]] static std::vector<SqliteMigration> migrations();

        [[nodiscard]] static const char* value_type_to_text(
            SqliteHistoryValueType value_type
        );

        [[nodiscard]] static SqliteHistoryValueType value_type_from_text(
            const unsigned char* value
        );

        void apply_schema();

        [[nodiscard]] std::int64_t append(
            const SqliteHistorySample& sample
        );

        [[nodiscard]] std::int64_t count();

        [[nodiscard]] std::int64_t count_for_tag(
            std::string_view tag_id
        );

        [[nodiscard]] std::optional<SqliteHistorySample> latest_for_tag(
            std::string_view tag_id
        );

        [[nodiscard]] std::vector<SqliteHistorySample> read_recent_for_tag(
            std::string_view tag_id,
            std::int64_t limit
        );

    private:
        SqliteDatabase* database_;

        static void validate_sample(
            const SqliteHistorySample& sample
        );
    };
}