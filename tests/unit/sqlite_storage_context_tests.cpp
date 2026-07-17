#include <dispatcher/storage/sqlite/sqlite_alarm_event.hpp>
#include <dispatcher/storage/sqlite/sqlite_configuration_snapshot.hpp>
#include <dispatcher/storage/sqlite/sqlite_database.hpp>
#include <dispatcher/storage/sqlite/sqlite_history_sample.hpp>
#include <dispatcher/storage/sqlite/sqlite_storage_context.hpp>

#include <gtest/gtest.h>

#include <filesystem>

namespace
{
    void append_history_sample(
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

    void append_alarm_event(
        dispatcher::storage::sqlite::SqliteAlarmEventStorage& storage,
        const dispatcher::storage::sqlite::SqliteAlarmEvent& event
    )
    {
        const auto id =
            storage.append(
                event
            );

        EXPECT_GT(
            id,
            0
        );
    }

    void save_configuration_snapshot(
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

TEST(SqliteStorageContextTests, OpensInMemoryDatabase)
{
    auto context =
        dispatcher::storage::sqlite::SqliteStorageContext::open_in_memory();

    EXPECT_TRUE(
        context.is_open()
    );

    EXPECT_TRUE(
        context.database().is_open()
    );

    EXPECT_EQ(
        context.database().path(),
        ":memory:"
    );
}

TEST(SqliteStorageContextTests, ExposesAllMigrations)
{
    const auto migrations =
        dispatcher::storage::sqlite::SqliteStorageContext::migrations();

    ASSERT_EQ(
        migrations.size(),
        3U
    );

    EXPECT_EQ(
        migrations[0].version,
        100
    );

    EXPECT_EQ(
        migrations[1].version,
        200
    );

    EXPECT_EQ(
        migrations[2].version,
        300
    );
}

TEST(SqliteStorageContextTests, AppliesAllSchemas)
{
    auto context =
        dispatcher::storage::sqlite::SqliteStorageContext::open_in_memory();

    context.apply_schema();

    EXPECT_EQ(
        context.database().query_int64(
            "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'history_samples';"
        ),
        1
    );

    EXPECT_EQ(
        context.database().query_int64(
            "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'alarm_events';"
        ),
        1
    );

    EXPECT_EQ(
        context.database().query_int64(
            "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'configuration_snapshots';"
        ),
        1
    );

    EXPECT_EQ(
        context.database().query_int64(
            "SELECT COUNT(*) FROM schema_migrations;"
        ),
        3
    );

    EXPECT_EQ(
        context.database().query_int64(
            "SELECT COALESCE(MAX(version), 0) FROM schema_migrations;"
        ),
        300
    );
}

TEST(SqliteStorageContextTests, ApplyingAllSchemasTwiceIsIdempotent)
{
    auto context =
        dispatcher::storage::sqlite::SqliteStorageContext::open_in_memory();

    context.apply_schema();
    context.apply_schema();

    EXPECT_EQ(
        context.database().query_int64(
            "SELECT COUNT(*) FROM schema_migrations;"
        ),
        3
    );

    EXPECT_EQ(
        context.database().query_int64(
            "SELECT COUNT(*) FROM schema_migrations WHERE version IN (100, 200, 300);"
        ),
        3
    );
}

TEST(SqliteStorageContextTests, RepositoriesShareSameDatabase)
{
    auto context =
        dispatcher::storage::sqlite::SqliteStorageContext::open_in_memory();

    context.apply_schema();

    auto history =
        context.history_storage();

    auto alarm_events =
        context.alarm_event_storage();

    auto configuration_snapshots =
        context.configuration_snapshot_storage();

    append_history_sample(
        history,
        dispatcher::storage::sqlite::SqliteHistorySample::numeric(
            "pump.pressure",
            "2026-07-02T10:00:00.000Z",
            12.5
        )
    );

    append_alarm_event(
        alarm_events,
        dispatcher::storage::sqlite::SqliteAlarmEvent::raised(
            "alarm-001",
            "pump.pressure",
            "critical",
            "Pressure is above limit",
            "2026-07-02T10:01:00.000Z"
        )
    );

    save_configuration_snapshot(
        configuration_snapshots,
        dispatcher::storage::sqlite::SqliteConfigurationSnapshotRecord::create(
            "initial",
            "1",
            "2026-07-02T10:02:00.000Z",
            "{\"devices\":[{\"id\":\"pump\"}],\"tags\":[{\"id\":\"pump.pressure\"}]}"
        )
    );

    EXPECT_EQ(
        history.count_for_tag(
            "pump.pressure"
        ),
        1
    );

    EXPECT_EQ(
        alarm_events.count_for_alarm(
            "alarm-001"
        ),
        1
    );

    EXPECT_EQ(
        configuration_snapshots.count(),
        1
    );

    EXPECT_EQ(
        context.database().query_int64(
            "SELECT COUNT(*) FROM history_samples;"
        ),
        1
    );

    EXPECT_EQ(
        context.database().query_int64(
            "SELECT COUNT(*) FROM alarm_events;"
        ),
        1
    );

    EXPECT_EQ(
        context.database().query_int64(
            "SELECT COUNT(*) FROM configuration_snapshots;"
        ),
        1
    );
}

TEST(SqliteStorageContextTests, RepositoryFactoriesCanBeCalledRepeatedly)
{
    auto context =
        dispatcher::storage::sqlite::SqliteStorageContext::open_in_memory();

    context.apply_schema();

    {
        auto history =
            context.history_storage();

        append_history_sample(
            history,
            dispatcher::storage::sqlite::SqliteHistorySample::numeric(
                "pump.pressure",
                "2026-07-02T10:00:00.000Z",
                10.0
            )
        );
    }

    {
        auto history =
            context.history_storage();

        append_history_sample(
            history,
            dispatcher::storage::sqlite::SqliteHistorySample::numeric(
                "pump.pressure",
                "2026-07-02T10:01:00.000Z",
                11.0
            )
        );

        EXPECT_EQ(
            history.count_for_tag(
                "pump.pressure"
            ),
            2
        );
    }
}

TEST(SqliteStorageContextTests, PersistsAllRepositoriesInFileDatabase)
{
    const auto path =
        std::filesystem::temp_directory_path()
        / "dispatcher_sqlite_storage_context_tests.sqlite";

    const auto removed_before =
        std::filesystem::remove(
            path
        );

    static_cast<void>(
        removed_before
        );

    {
        auto context =
            dispatcher::storage::sqlite::SqliteStorageContext::open_file(
                path
            );

        context.apply_schema();

        auto history =
            context.history_storage();

        auto alarm_events =
            context.alarm_event_storage();

        auto configuration_snapshots =
            context.configuration_snapshot_storage();

        append_history_sample(
            history,
            dispatcher::storage::sqlite::SqliteHistorySample::numeric(
                "pump.pressure",
                "2026-07-02T10:00:00.000Z",
                12.5
            )
        );

        append_alarm_event(
            alarm_events,
            dispatcher::storage::sqlite::SqliteAlarmEvent::raised(
                "alarm-001",
                "pump.pressure",
                "critical",
                "Pressure is above limit",
                "2026-07-02T10:01:00.000Z"
            )
        );

        save_configuration_snapshot(
            configuration_snapshots,
            dispatcher::storage::sqlite::SqliteConfigurationSnapshotRecord::create(
                "initial",
                "1",
                "2026-07-02T10:02:00.000Z",
                "{\"devices\":[{\"id\":\"pump\"}],\"tags\":[{\"id\":\"pump.pressure\"}]}"
            )
        );
    }

    {
        auto context =
            dispatcher::storage::sqlite::SqliteStorageContext::open_file(
                path
            );

        context.apply_schema();

        auto history =
            context.history_storage();

        auto alarm_events =
            context.alarm_event_storage();

        auto configuration_snapshots =
            context.configuration_snapshot_storage();

        EXPECT_EQ(
            history.count_for_tag(
                "pump.pressure"
            ),
            1
        );

        EXPECT_EQ(
            alarm_events.count_for_alarm(
                "alarm-001"
            ),
            1
        );

        EXPECT_EQ(
            configuration_snapshots.count(),
            1
        );

        const auto latest_history =
            history.latest_for_tag(
                "pump.pressure"
            );

        ASSERT_TRUE(
            latest_history.has_value()
        );

        ASSERT_TRUE(
            latest_history->numeric_value.has_value()
        );

        EXPECT_DOUBLE_EQ(
            latest_history->numeric_value.value(),
            12.5
        );

        const auto latest_alarm =
            alarm_events.latest_for_alarm(
                "alarm-001"
            );

        ASSERT_TRUE(
            latest_alarm.has_value()
        );

        EXPECT_EQ(
            latest_alarm->event_type,
            "raised"
        );

        const auto latest_snapshot =
            configuration_snapshots.latest();

        ASSERT_TRUE(
            latest_snapshot.has_value()
        );

        EXPECT_EQ(
            latest_snapshot->snapshot_name,
            "initial"
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