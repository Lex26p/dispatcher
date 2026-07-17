#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

struct sqlite3;

namespace dispatcher::storage::sqlite
{
    class SqliteDatabase final
    {
    public:
        SqliteDatabase() = default;

        explicit SqliteDatabase(
            sqlite3* database,
            std::string path
        );

        ~SqliteDatabase();

        SqliteDatabase(
            const SqliteDatabase&
        ) = delete;

        SqliteDatabase& operator=(
            const SqliteDatabase&
            ) = delete;

        SqliteDatabase(
            SqliteDatabase&& other
        ) noexcept;

        SqliteDatabase& operator=(
            SqliteDatabase&& other
            ) noexcept;

        [[nodiscard]] static SqliteDatabase open_in_memory();

        [[nodiscard]] static SqliteDatabase open_file(
            const std::filesystem::path& path
        );

        [[nodiscard]] bool is_open() const noexcept;

        [[nodiscard]] const std::string& path() const noexcept;

        [[nodiscard]] sqlite3* native_handle() const noexcept;

        void execute(
            std::string_view sql
        );

        [[nodiscard]] std::int64_t query_int64(
            std::string_view sql
        );

        void close() noexcept;

    private:
        sqlite3* database_{ nullptr };
        std::string path_{};

        [[nodiscard]] static SqliteDatabase open(
            const std::string& path
        );

        [[nodiscard]] std::string last_error_message() const;

        void ensure_open() const;
    };
}