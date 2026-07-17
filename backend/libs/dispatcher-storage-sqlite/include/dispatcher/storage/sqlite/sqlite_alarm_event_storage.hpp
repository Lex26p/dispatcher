#pragma once

#include <dispatcher/storage/sqlite/sqlite_alarm_event.hpp>
#include <dispatcher/storage/sqlite/sqlite_database.hpp>
#include <dispatcher/storage/sqlite/sqlite_migration.hpp>

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace dispatcher::storage::sqlite
{
    class SqliteAlarmEventStorage final
    {
    public:
        explicit SqliteAlarmEventStorage(
            SqliteDatabase& database
        );

        [[nodiscard]] static std::vector<SqliteMigration> migrations();

        void apply_schema();

        [[nodiscard]] std::int64_t append(
            const SqliteAlarmEvent& event
        );

        [[nodiscard]] std::int64_t count();

        [[nodiscard]] std::int64_t count_for_alarm(
            std::string_view alarm_id
        );

        [[nodiscard]] std::optional<SqliteAlarmEvent> latest_for_alarm(
            std::string_view alarm_id
        );

        [[nodiscard]] std::vector<SqliteAlarmEvent> read_recent_for_alarm(
            std::string_view alarm_id,
            std::int64_t limit
        );

        [[nodiscard]] std::vector<SqliteAlarmEvent> read_recent(
            std::int64_t limit
        );

    private:
        SqliteDatabase* database_;

        static void validate_event(
            const SqliteAlarmEvent& event
        );
    };
}