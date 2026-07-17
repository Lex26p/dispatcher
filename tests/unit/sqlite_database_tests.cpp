#include <dispatcher/storage/sqlite/sqlite_database.hpp>
#include <dispatcher/storage/sqlite/sqlite_error.hpp>

#include <gtest/gtest.h>

#include <filesystem>

TEST(SqliteDatabaseTests, OpensInMemoryDatabase)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    EXPECT_TRUE(
        database.is_open()
    );

    EXPECT_EQ(
        database.path(),
        ":memory:"
    );
}

TEST(SqliteDatabaseTests, ExecutesSql)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    database.execute(
        "CREATE TABLE sample_items (id INTEGER PRIMARY KEY, name TEXT NOT NULL);"
    );

    database.execute(
        "INSERT INTO sample_items (name) VALUES ('alpha');"
    );

    const auto count =
        database.query_int64(
            "SELECT COUNT(*) FROM sample_items;"
        );

    EXPECT_EQ(
        count,
        1
    );
}

TEST(SqliteDatabaseTests, QueryInt64ReturnsScalarValue)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    const auto value =
        database.query_int64(
            "SELECT 42;"
        );

    EXPECT_EQ(
        value,
        42
    );
}

TEST(SqliteDatabaseTests, InvalidSqlThrows)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    EXPECT_THROW(
        database.execute(
            "CREATE TABLE"
        ),
        dispatcher::storage::sqlite::SqliteError
    );
}

TEST(SqliteDatabaseTests, QueryWithoutRowsThrows)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    EXPECT_THROW(
        {
            const auto value =
                database.query_int64(
                    "SELECT 1 WHERE 0;"
                );

            static_cast<void>(
                value
            );
        },
        dispatcher::storage::sqlite::SqliteError
    );
}

TEST(SqliteDatabaseTests, ClosedDatabaseRejectsExecute)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    database.close();

    EXPECT_FALSE(
        database.is_open()
    );

    EXPECT_THROW(
        database.execute(
            "SELECT 1;"
        ),
        dispatcher::storage::sqlite::SqliteError
    );
}

TEST(SqliteDatabaseTests, SupportsMoveConstruction)
{
    auto original =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto moved =
        std::move(
            original
        );

    EXPECT_FALSE(
        original.is_open()
    );

    EXPECT_TRUE(
        moved.is_open()
    );

    moved.execute(
        "CREATE TABLE moved_items (id INTEGER PRIMARY KEY);"
    );
}

TEST(SqliteDatabaseTests, SupportsMoveAssignment)
{
    auto first =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto second =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    second =
        std::move(
            first
        );

    EXPECT_FALSE(
        first.is_open()
    );

    EXPECT_TRUE(
        second.is_open()
    );

    second.execute(
        "CREATE TABLE assigned_items (id INTEGER PRIMARY KEY);"
    );
}

TEST(SqliteDatabaseTests, OpensFileDatabase)
{
    const auto path =
        std::filesystem::temp_directory_path()
        / "dispatcher_sqlite_database_tests.sqlite";

    const auto removed_before =
        std::filesystem::remove(
            path
        );

    static_cast<void>(
        removed_before
        );

    {
        auto database =
            dispatcher::storage::sqlite::SqliteDatabase::open_file(
                path
            );

        database.execute(
            "CREATE TABLE persistent_items (id INTEGER PRIMARY KEY, name TEXT NOT NULL);"
        );

        database.execute(
            "INSERT INTO persistent_items (name) VALUES ('alpha');"
        );
    }

    {
        auto database =
            dispatcher::storage::sqlite::SqliteDatabase::open_file(
                path
            );

        const auto count =
            database.query_int64(
                "SELECT COUNT(*) FROM persistent_items;"
            );

        EXPECT_EQ(
            count,
            1
        );
    }

    const auto removed_after =
        std::filesystem::remove(
            path
        );

    static_cast<void>(
        removed_after
        );
}