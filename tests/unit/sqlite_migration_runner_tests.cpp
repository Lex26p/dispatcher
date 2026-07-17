#include <dispatcher/storage/sqlite/sqlite_database.hpp>
#include <dispatcher/storage/sqlite/sqlite_error.hpp>
#include <dispatcher/storage/sqlite/sqlite_migration.hpp>
#include <dispatcher/storage/sqlite/sqlite_migration_runner.hpp>

#include <gtest/gtest.h>

#include <vector>

namespace
{
    dispatcher::storage::sqlite::SqliteMigration make_create_items_migration()
    {
        return dispatcher::storage::sqlite::SqliteMigration{
            1,
            "create_items",
            "CREATE TABLE items ("
            "id INTEGER PRIMARY KEY,"
            "name TEXT NOT NULL"
            ");"
        };
    }

    dispatcher::storage::sqlite::SqliteMigration make_insert_item_migration()
    {
        return dispatcher::storage::sqlite::SqliteMigration{
            2,
            "insert_item",
            "INSERT INTO items (name) VALUES ('alpha');"
        };
    }
}

TEST(SqliteMigrationRunnerTests, EnsuresMigrationTable)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    dispatcher::storage::sqlite::SqliteMigrationRunner runner{
        database
    };

    runner.ensure_migration_table();

    const auto count =
        database.query_int64(
            "SELECT COUNT(*) FROM schema_migrations;"
        );

    EXPECT_EQ(
        count,
        0
    );
}

TEST(SqliteMigrationRunnerTests, AppliesSingleMigration)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    dispatcher::storage::sqlite::SqliteMigrationRunner runner{
        database
    };

    runner.apply(
        make_create_items_migration()
    );

    EXPECT_TRUE(
        runner.has_migration(
            1
        )
    );

    EXPECT_EQ(
        runner.applied_migration_count(),
        1
    );

    EXPECT_EQ(
        runner.latest_applied_version(),
        1
    );

    database.execute(
        "INSERT INTO items (name) VALUES ('beta');"
    );

    EXPECT_EQ(
        database.query_int64(
            "SELECT COUNT(*) FROM items;"
        ),
        1
    );
}

TEST(SqliteMigrationRunnerTests, ApplyingSameMigrationTwiceIsIdempotent)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    dispatcher::storage::sqlite::SqliteMigrationRunner runner{
        database
    };

    const auto migration =
        make_create_items_migration();

    runner.apply(
        migration
    );

    runner.apply(
        migration
    );

    EXPECT_EQ(
        runner.applied_migration_count(),
        1
    );

    EXPECT_EQ(
        runner.latest_applied_version(),
        1
    );
}

TEST(SqliteMigrationRunnerTests, AppliesAllMigrationsInVersionOrder)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    dispatcher::storage::sqlite::SqliteMigrationRunner runner{
        database
    };

    const std::vector<dispatcher::storage::sqlite::SqliteMigration> migrations{
        make_insert_item_migration(),
        make_create_items_migration()
    };

    runner.apply_all(
        migrations
    );

    EXPECT_EQ(
        runner.applied_migration_count(),
        2
    );

    EXPECT_EQ(
        runner.latest_applied_version(),
        2
    );

    EXPECT_EQ(
        database.query_int64(
            "SELECT COUNT(*) FROM items;"
        ),
        1
    );
}

TEST(SqliteMigrationRunnerTests, RejectsInvalidVersion)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    dispatcher::storage::sqlite::SqliteMigrationRunner runner{
        database
    };

    const dispatcher::storage::sqlite::SqliteMigration migration{
        0,
        "invalid_version",
        "SELECT 1;"
    };

    EXPECT_THROW(
        runner.apply(
            migration
        ),
        dispatcher::storage::sqlite::SqliteError
    );
}

TEST(SqliteMigrationRunnerTests, RejectsEmptyName)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    dispatcher::storage::sqlite::SqliteMigrationRunner runner{
        database
    };

    const dispatcher::storage::sqlite::SqliteMigration migration{
        1,
        "",
        "SELECT 1;"
    };

    EXPECT_THROW(
        runner.apply(
            migration
        ),
        dispatcher::storage::sqlite::SqliteError
    );
}

TEST(SqliteMigrationRunnerTests, RejectsEmptySql)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    dispatcher::storage::sqlite::SqliteMigrationRunner runner{
        database
    };

    const dispatcher::storage::sqlite::SqliteMigration migration{
        1,
        "empty_sql",
        ""
    };

    EXPECT_THROW(
        runner.apply(
            migration
        ),
        dispatcher::storage::sqlite::SqliteError
    );
}

TEST(SqliteMigrationRunnerTests, RollsBackFailedMigration)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    dispatcher::storage::sqlite::SqliteMigrationRunner runner{
        database
    };

    const dispatcher::storage::sqlite::SqliteMigration migration{
        1,
        "broken_migration",
        "CREATE TABLE broken_items (id INTEGER PRIMARY KEY);"
        "INSERT INTO missing_table (name) VALUES ('boom');"
    };

    EXPECT_THROW(
        runner.apply(
            migration
        ),
        dispatcher::storage::sqlite::SqliteError
    );

    EXPECT_FALSE(
        runner.has_migration(
            1
        )
    );

    EXPECT_EQ(
        database.query_int64(
            "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'broken_items';"
        ),
        0
    );
}

TEST(SqliteMigrationRunnerTests, EscapesMigrationName)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    dispatcher::storage::sqlite::SqliteMigrationRunner runner{
        database
    };

    const dispatcher::storage::sqlite::SqliteMigration migration{
        1,
        "create 'quoted' item",
        "CREATE TABLE quoted_items (id INTEGER PRIMARY KEY);"
    };

    runner.apply(
        migration
    );

    EXPECT_EQ(
        runner.applied_migration_count(),
        1
    );

    EXPECT_EQ(
        database.query_int64(
            "SELECT COUNT(*) FROM schema_migrations WHERE name = 'create ''quoted'' item';"
        ),
        1
    );
}