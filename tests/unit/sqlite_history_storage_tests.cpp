#include <dispatcher/storage/sqlite/sqlite_database.hpp>
#include <dispatcher/storage/sqlite/sqlite_error.hpp>
#include <dispatcher/storage/sqlite/sqlite_history_sample.hpp>
#include <dispatcher/storage/sqlite/sqlite_history_storage.hpp>
#include <dispatcher/storage/sqlite/sqlite_migration_runner.hpp>

#include <gtest/gtest.h>

#include <filesystem>

namespace
{
    dispatcher::storage::sqlite::SqliteHistoryStorage make_storage(
        dispatcher::storage::sqlite::SqliteDatabase& database
    )
    {
        dispatcher::storage::sqlite::SqliteHistoryStorage storage{
            database
        };

        storage.apply_schema();

        return storage;
    }

    void append_sample(
        dispatcher::storage::sqlite::SqliteHistoryStorage& storage,
        const dispatcher::storage::sqlite::SqliteHistorySample& sample
    )
    {
        const auto id =
            storage.append(
                sample
            );

        EXPECT_GT(
            id,
            0
        );
    }
}

TEST(SqliteHistoryStorageTests, AppliesSchema)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    static_cast<void>(
        storage
        );

    EXPECT_EQ(
        database.query_int64(
            "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'history_samples';"
        ),
        1
    );

    EXPECT_EQ(
        database.query_int64(
            "SELECT COUNT(*) FROM schema_migrations WHERE version = 100;"
        ),
        1
    );
}

TEST(SqliteHistoryStorageTests, ApplyingSchemaTwiceIsIdempotent)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    storage.apply_schema();

    EXPECT_EQ(
        database.query_int64(
            "SELECT COUNT(*) FROM schema_migrations WHERE version = 100;"
        ),
        1
    );
}

TEST(SqliteHistoryStorageTests, AppendsNumericSample)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    const auto id =
        storage.append(
            dispatcher::storage::sqlite::SqliteHistorySample::numeric(
                "pump.pressure",
                "2026-07-02T10:00:00.000Z",
                12.5
            )
        );

    EXPECT_GT(
        id,
        0
    );

    EXPECT_EQ(
        storage.count(),
        1
    );

    EXPECT_EQ(
        storage.count_for_tag(
            "pump.pressure"
        ),
        1
    );
}

TEST(SqliteHistoryStorageTests, AppendsTextSample)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    append_sample(
        storage,
        dispatcher::storage::sqlite::SqliteHistorySample::text(
            "pump.mode",
            "2026-07-02T10:01:00.000Z",
            "AUTO"
        )
    );

    const auto latest =
        storage.latest_for_tag(
            "pump.mode"
        );

    ASSERT_TRUE(
        latest.has_value()
    );

    EXPECT_EQ(
        latest->value_type,
        dispatcher::storage::sqlite::SqliteHistoryValueType::text
    );

    ASSERT_TRUE(
        latest->text_value.has_value()
    );

    EXPECT_EQ(
        latest->text_value.value(),
        "AUTO"
    );
}

TEST(SqliteHistoryStorageTests, AppendsBooleanSample)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    append_sample(
        storage,
        dispatcher::storage::sqlite::SqliteHistorySample::boolean(
            "pump.running",
            "2026-07-02T10:02:00.000Z",
            true
        )
    );

    const auto latest =
        storage.latest_for_tag(
            "pump.running"
        );

    ASSERT_TRUE(
        latest.has_value()
    );

    EXPECT_EQ(
        latest->value_type,
        dispatcher::storage::sqlite::SqliteHistoryValueType::boolean
    );

    ASSERT_TRUE(
        latest->bool_value.has_value()
    );

    EXPECT_TRUE(
        latest->bool_value.value()
    );
}

TEST(SqliteHistoryStorageTests, LatestForTagReturnsNewestByTimestamp)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    append_sample(
        storage,
        dispatcher::storage::sqlite::SqliteHistorySample::numeric(
            "pump.pressure",
            "2026-07-02T10:00:00.000Z",
            10.0
        )
    );

    append_sample(
        storage,
        dispatcher::storage::sqlite::SqliteHistorySample::numeric(
            "pump.pressure",
            "2026-07-02T10:05:00.000Z",
            15.0
        )
    );

    const auto latest =
        storage.latest_for_tag(
            "pump.pressure"
        );

    ASSERT_TRUE(
        latest.has_value()
    );

    ASSERT_TRUE(
        latest->numeric_value.has_value()
    );

    EXPECT_DOUBLE_EQ(
        latest->numeric_value.value(),
        15.0
    );
}

TEST(SqliteHistoryStorageTests, ReadRecentForTagHonorsLimitAndOrder)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    append_sample(
        storage,
        dispatcher::storage::sqlite::SqliteHistorySample::numeric(
            "pump.pressure",
            "2026-07-02T10:00:00.000Z",
            10.0
        )
    );

    append_sample(
        storage,
        dispatcher::storage::sqlite::SqliteHistorySample::numeric(
            "pump.pressure",
            "2026-07-02T10:01:00.000Z",
            11.0
        )
    );

    append_sample(
        storage,
        dispatcher::storage::sqlite::SqliteHistorySample::numeric(
            "pump.pressure",
            "2026-07-02T10:02:00.000Z",
            12.0
        )
    );

    const auto samples =
        storage.read_recent_for_tag(
            "pump.pressure",
            2
        );

    ASSERT_EQ(
        samples.size(),
        2U
    );

    ASSERT_TRUE(
        samples[0].numeric_value.has_value()
    );

    ASSERT_TRUE(
        samples[1].numeric_value.has_value()
    );

    EXPECT_DOUBLE_EQ(
        samples[0].numeric_value.value(),
        12.0
    );

    EXPECT_DOUBLE_EQ(
        samples[1].numeric_value.value(),
        11.0
    );
}

TEST(SqliteHistoryStorageTests, ReadRecentForUnknownTagReturnsEmpty)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    const auto samples =
        storage.read_recent_for_tag(
            "unknown",
            10
        );

    EXPECT_TRUE(
        samples.empty()
    );

    const auto latest =
        storage.latest_for_tag(
            "unknown"
        );

    EXPECT_FALSE(
        latest.has_value()
    );
}

TEST(SqliteHistoryStorageTests, ReadRecentWithNonPositiveLimitReturnsEmpty)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    append_sample(
        storage,
        dispatcher::storage::sqlite::SqliteHistorySample::numeric(
            "pump.pressure",
            "2026-07-02T10:00:00.000Z",
            10.0
        )
    );

    const auto zero_limit_samples =
        storage.read_recent_for_tag(
            "pump.pressure",
            0
        );

    EXPECT_TRUE(
        zero_limit_samples.empty()
    );

    const auto negative_limit_samples =
        storage.read_recent_for_tag(
            "pump.pressure",
            -1
        );

    EXPECT_TRUE(
        negative_limit_samples.empty()
    );
}

TEST(SqliteHistoryStorageTests, RejectsEmptyTag)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    auto sample =
        dispatcher::storage::sqlite::SqliteHistorySample::numeric(
            "",
            "2026-07-02T10:00:00.000Z",
            10.0
        );

    EXPECT_THROW(
        {
            const auto id =
                storage.append(
                    sample
                );

            static_cast<void>(
                id
            );
        },
        dispatcher::storage::sqlite::SqliteError
    );
}

TEST(SqliteHistoryStorageTests, RejectsEmptyTimestamp)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    auto sample =
        dispatcher::storage::sqlite::SqliteHistorySample::numeric(
            "pump.pressure",
            "",
            10.0
        );

    EXPECT_THROW(
        {
            const auto id =
                storage.append(
                    sample
                );

            static_cast<void>(
                id
            );
        },
        dispatcher::storage::sqlite::SqliteError
    );
}

TEST(SqliteHistoryStorageTests, RejectsMissingNumericValue)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    dispatcher::storage::sqlite::SqliteHistorySample sample;

    sample.tag_id = "pump.pressure";
    sample.timestamp_utc = "2026-07-02T10:00:00.000Z";
    sample.value_type = dispatcher::storage::sqlite::SqliteHistoryValueType::numeric;

    EXPECT_THROW(
        {
            const auto id =
                storage.append(
                    sample
                );

            static_cast<void>(
                id
            );
        },
        dispatcher::storage::sqlite::SqliteError
    );
}

TEST(SqliteHistoryStorageTests, PersistsSamplesInFileDatabase)
{
    const auto path =
        std::filesystem::temp_directory_path()
        / "dispatcher_sqlite_history_storage_tests.sqlite";

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

        auto storage =
            make_storage(
                database
            );

        append_sample(
            storage,
            dispatcher::storage::sqlite::SqliteHistorySample::numeric(
                "pump.pressure",
                "2026-07-02T10:00:00.000Z",
                12.5
            )
        );
    }

    {
        auto database =
            dispatcher::storage::sqlite::SqliteDatabase::open_file(
                path
            );

        auto storage =
            make_storage(
                database
            );

        EXPECT_EQ(
            storage.count_for_tag(
                "pump.pressure"
            ),
            1
        );

        const auto latest =
            storage.latest_for_tag(
                "pump.pressure"
            );

        ASSERT_TRUE(
            latest.has_value()
        );

        ASSERT_TRUE(
            latest->numeric_value.has_value()
        );

        EXPECT_DOUBLE_EQ(
            latest->numeric_value.value(),
            12.5
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