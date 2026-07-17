#include <dispatcher/storage/sqlite/sqlite_configuration_snapshot.hpp>
#include <dispatcher/storage/sqlite/sqlite_configuration_snapshot_storage.hpp>
#include <dispatcher/storage/sqlite/sqlite_database.hpp>
#include <dispatcher/storage/sqlite/sqlite_error.hpp>

#include <gtest/gtest.h>

#include <filesystem>

namespace
{
    dispatcher::storage::sqlite::SqliteConfigurationSnapshotStorage make_storage(
        dispatcher::storage::sqlite::SqliteDatabase& database
    )
    {
        dispatcher::storage::sqlite::SqliteConfigurationSnapshotStorage storage{
            database
        };

        storage.apply_schema();

        return storage;
    }

    dispatcher::storage::sqlite::SqliteConfigurationSnapshotRecord make_record(
        std::string name,
        std::string timestamp,
        std::string payload = "{\"devices\":[],\"tags\":[]}"
    )
    {
        return dispatcher::storage::sqlite::SqliteConfigurationSnapshotRecord::create(
            std::move(
                name
            ),
            "1",
            std::move(
                timestamp
            ),
            std::move(
                payload
            )
        );
    }

    void save_record(
        dispatcher::storage::sqlite::SqliteConfigurationSnapshotStorage& storage,
        const dispatcher::storage::sqlite::SqliteConfigurationSnapshotRecord& record
    )
    {
        const auto id =
            storage.save(
                record
            );

        EXPECT_GT(
            id,
            0
        );
    }
}

TEST(SqliteConfigurationSnapshotStorageTests, AppliesSchema)
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
            "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'configuration_snapshots';"
        ),
        1
    );

    EXPECT_EQ(
        database.query_int64(
            "SELECT COUNT(*) FROM schema_migrations WHERE version = 300;"
        ),
        1
    );
}

TEST(SqliteConfigurationSnapshotStorageTests, ApplyingSchemaTwiceIsIdempotent)
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
            "SELECT COUNT(*) FROM schema_migrations WHERE version = 300;"
        ),
        1
    );
}

TEST(SqliteConfigurationSnapshotStorageTests, SavesSnapshot)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    const auto id =
        storage.save(
            make_record(
                "initial",
                "2026-07-02T10:00:00.000Z"
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
}

TEST(SqliteConfigurationSnapshotStorageTests, FindsSnapshotById)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    const auto id =
        storage.save(
            make_record(
                "initial",
                "2026-07-02T10:00:00.000Z"
            )
        );

    const auto record =
        storage.find_by_id(
            id
        );

    ASSERT_TRUE(
        record.has_value()
    );

    EXPECT_EQ(
        record->id,
        id
    );

    EXPECT_EQ(
        record->snapshot_name,
        "initial"
    );

    EXPECT_EQ(
        record->schema_version,
        "1"
    );

    EXPECT_EQ(
        record->payload_json,
        "{\"devices\":[],\"tags\":[]}"
    );
}

TEST(SqliteConfigurationSnapshotStorageTests, FindByUnknownIdReturnsEmpty)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    const auto record =
        storage.find_by_id(
            42
        );

    EXPECT_FALSE(
        record.has_value()
    );
}

TEST(SqliteConfigurationSnapshotStorageTests, LatestReturnsNewestByTimestamp)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    save_record(
        storage,
        make_record(
            "old",
            "2026-07-02T10:00:00.000Z"
        )
    );

    save_record(
        storage,
        make_record(
            "new",
            "2026-07-02T10:05:00.000Z"
        )
    );

    const auto latest =
        storage.latest();

    ASSERT_TRUE(
        latest.has_value()
    );

    EXPECT_EQ(
        latest->snapshot_name,
        "new"
    );
}

TEST(SqliteConfigurationSnapshotStorageTests, ReadLatestHonorsLimitAndOrder)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    save_record(
        storage,
        make_record(
            "snapshot-1",
            "2026-07-02T10:00:00.000Z"
        )
    );

    save_record(
        storage,
        make_record(
            "snapshot-2",
            "2026-07-02T10:01:00.000Z"
        )
    );

    save_record(
        storage,
        make_record(
            "snapshot-3",
            "2026-07-02T10:02:00.000Z"
        )
    );

    const auto records =
        storage.read_latest(
            2
        );

    ASSERT_EQ(
        records.size(),
        2U
    );

    EXPECT_EQ(
        records[0].snapshot_name,
        "snapshot-3"
    );

    EXPECT_EQ(
        records[1].snapshot_name,
        "snapshot-2"
    );
}

TEST(SqliteConfigurationSnapshotStorageTests, ReadLatestWithNonPositiveLimitReturnsEmpty)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    save_record(
        storage,
        make_record(
            "snapshot-1",
            "2026-07-02T10:00:00.000Z"
        )
    );

    const auto zero_records =
        storage.read_latest(
            0
        );

    EXPECT_TRUE(
        zero_records.empty()
    );

    const auto negative_records =
        storage.read_latest(
            -1
        );

    EXPECT_TRUE(
        negative_records.empty()
    );
}

TEST(SqliteConfigurationSnapshotStorageTests, LatestReturnsEmptyWhenNoRecordsExist)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    const auto latest =
        storage.latest();

    EXPECT_FALSE(
        latest.has_value()
    );
}

TEST(SqliteConfigurationSnapshotStorageTests, RejectsEmptyName)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    auto record =
        make_record(
            "",
            "2026-07-02T10:00:00.000Z"
        );

    EXPECT_THROW(
        {
            const auto id =
                storage.save(
                    record
                );

            static_cast<void>(
                id
            );
        },
        dispatcher::storage::sqlite::SqliteError
    );
}

TEST(SqliteConfigurationSnapshotStorageTests, RejectsEmptyTimestamp)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    auto record =
        make_record(
            "snapshot",
            ""
        );

    EXPECT_THROW(
        {
            const auto id =
                storage.save(
                    record
                );

            static_cast<void>(
                id
            );
        },
        dispatcher::storage::sqlite::SqliteError
    );
}

TEST(SqliteConfigurationSnapshotStorageTests, RejectsEmptyPayload)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    auto record =
        make_record(
            "snapshot",
            "2026-07-02T10:00:00.000Z",
            ""
        );

    EXPECT_THROW(
        {
            const auto id =
                storage.save(
                    record
                );

            static_cast<void>(
                id
            );
        },
        dispatcher::storage::sqlite::SqliteError
    );
}

TEST(SqliteConfigurationSnapshotStorageTests, PersistsSnapshotsInFileDatabase)
{
    const auto path =
        std::filesystem::temp_directory_path()
        / "dispatcher_sqlite_configuration_snapshot_storage_tests.sqlite";

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

        save_record(
            storage,
            make_record(
                "initial",
                "2026-07-02T10:00:00.000Z",
                "{\"devices\":[{\"id\":\"device-1\"}],\"tags\":[]}"
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
            storage.count(),
            1
        );

        const auto latest =
            storage.latest();

        ASSERT_TRUE(
            latest.has_value()
        );

        EXPECT_EQ(
            latest->snapshot_name,
            "initial"
        );

        EXPECT_EQ(
            latest->payload_json,
            "{\"devices\":[{\"id\":\"device-1\"}],\"tags\":[]}"
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