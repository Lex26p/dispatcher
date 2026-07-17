#include <dispatcher/storage/sqlite/sqlite_alarm_event.hpp>
#include <dispatcher/storage/sqlite/sqlite_alarm_event_storage.hpp>
#include <dispatcher/storage/sqlite/sqlite_database.hpp>
#include <dispatcher/storage/sqlite/sqlite_error.hpp>

#include <gtest/gtest.h>

#include <filesystem>

namespace
{
    dispatcher::storage::sqlite::SqliteAlarmEventStorage make_storage(
        dispatcher::storage::sqlite::SqliteDatabase& database
    )
    {
        dispatcher::storage::sqlite::SqliteAlarmEventStorage storage{
            database
        };

        storage.apply_schema();

        return storage;
    }

    void append_event(
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
}

TEST(SqliteAlarmEventStorageTests, AppliesSchema)
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
            "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'alarm_events';"
        ),
        1
    );

    EXPECT_EQ(
        database.query_int64(
            "SELECT COUNT(*) FROM schema_migrations WHERE version = 200;"
        ),
        1
    );
}

TEST(SqliteAlarmEventStorageTests, ApplyingSchemaTwiceIsIdempotent)
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
            "SELECT COUNT(*) FROM schema_migrations WHERE version = 200;"
        ),
        1
    );
}

TEST(SqliteAlarmEventStorageTests, AppendsRaisedEvent)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    const auto id =
        storage.append(
            dispatcher::storage::sqlite::SqliteAlarmEvent::raised(
                "alarm-001",
                "pump.pressure",
                "critical",
                "Pressure is above limit",
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

    EXPECT_EQ(
        storage.count_for_alarm(
            "alarm-001"
        ),
        1
    );
}

TEST(SqliteAlarmEventStorageTests, AppendsAcknowledgedEventWithOperatorAndComment)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    append_event(
        storage,
        dispatcher::storage::sqlite::SqliteAlarmEvent::acknowledged(
            "alarm-001",
            "pump.pressure",
            "critical",
            "Operator acknowledged alarm",
            "2026-07-02T10:01:00.000Z",
            "operator-1",
            "Investigating pump pressure"
        )
    );

    const auto latest =
        storage.latest_for_alarm(
            "alarm-001"
        );

    ASSERT_TRUE(
        latest.has_value()
    );

    EXPECT_EQ(
        latest->event_type,
        "acknowledged"
    );

    ASSERT_TRUE(
        latest->operator_id.has_value()
    );

    EXPECT_EQ(
        latest->operator_id.value(),
        "operator-1"
    );

    ASSERT_TRUE(
        latest->comment.has_value()
    );

    EXPECT_EQ(
        latest->comment.value(),
        "Investigating pump pressure"
    );
}

TEST(SqliteAlarmEventStorageTests, LatestForAlarmReturnsNewestByTimestamp)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    append_event(
        storage,
        dispatcher::storage::sqlite::SqliteAlarmEvent::raised(
            "alarm-001",
            "pump.pressure",
            "critical",
            "Raised",
            "2026-07-02T10:00:00.000Z"
        )
    );

    append_event(
        storage,
        dispatcher::storage::sqlite::SqliteAlarmEvent::cleared(
            "alarm-001",
            "pump.pressure",
            "critical",
            "Cleared",
            "2026-07-02T10:05:00.000Z"
        )
    );

    const auto latest =
        storage.latest_for_alarm(
            "alarm-001"
        );

    ASSERT_TRUE(
        latest.has_value()
    );

    EXPECT_EQ(
        latest->event_type,
        "cleared"
    );

    EXPECT_EQ(
        latest->state,
        "cleared"
    );
}

TEST(SqliteAlarmEventStorageTests, ReadRecentForAlarmHonorsLimitAndOrder)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    append_event(
        storage,
        dispatcher::storage::sqlite::SqliteAlarmEvent::raised(
            "alarm-001",
            "pump.pressure",
            "critical",
            "Raised",
            "2026-07-02T10:00:00.000Z"
        )
    );

    append_event(
        storage,
        dispatcher::storage::sqlite::SqliteAlarmEvent::acknowledged(
            "alarm-001",
            "pump.pressure",
            "critical",
            "Acknowledged",
            "2026-07-02T10:01:00.000Z",
            "operator-1"
        )
    );

    append_event(
        storage,
        dispatcher::storage::sqlite::SqliteAlarmEvent::cleared(
            "alarm-001",
            "pump.pressure",
            "critical",
            "Cleared",
            "2026-07-02T10:02:00.000Z"
        )
    );

    const auto events =
        storage.read_recent_for_alarm(
            "alarm-001",
            2
        );

    ASSERT_EQ(
        events.size(),
        2U
    );

    EXPECT_EQ(
        events[0].event_type,
        "cleared"
    );

    EXPECT_EQ(
        events[1].event_type,
        "acknowledged"
    );
}

TEST(SqliteAlarmEventStorageTests, ReadRecentReturnsGlobalNewestEvents)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    append_event(
        storage,
        dispatcher::storage::sqlite::SqliteAlarmEvent::raised(
            "alarm-001",
            "pump.pressure",
            "critical",
            "Alarm 1",
            "2026-07-02T10:00:00.000Z"
        )
    );

    append_event(
        storage,
        dispatcher::storage::sqlite::SqliteAlarmEvent::raised(
            "alarm-002",
            "tank.level",
            "high",
            "Alarm 2",
            "2026-07-02T10:03:00.000Z"
        )
    );

    append_event(
        storage,
        dispatcher::storage::sqlite::SqliteAlarmEvent::raised(
            "alarm-003",
            "motor.temperature",
            "medium",
            "Alarm 3",
            "2026-07-02T10:02:00.000Z"
        )
    );

    const auto events =
        storage.read_recent(
            2
        );

    ASSERT_EQ(
        events.size(),
        2U
    );

    EXPECT_EQ(
        events[0].alarm_id,
        "alarm-002"
    );

    EXPECT_EQ(
        events[1].alarm_id,
        "alarm-003"
    );
}

TEST(SqliteAlarmEventStorageTests, UnknownAlarmReturnsEmpty)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    const auto events =
        storage.read_recent_for_alarm(
            "unknown",
            10
        );

    EXPECT_TRUE(
        events.empty()
    );

    const auto latest =
        storage.latest_for_alarm(
            "unknown"
        );

    EXPECT_FALSE(
        latest.has_value()
    );
}

TEST(SqliteAlarmEventStorageTests, NonPositiveLimitReturnsEmpty)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    append_event(
        storage,
        dispatcher::storage::sqlite::SqliteAlarmEvent::raised(
            "alarm-001",
            "pump.pressure",
            "critical",
            "Raised",
            "2026-07-02T10:00:00.000Z"
        )
    );

    const auto zero_alarm_events =
        storage.read_recent_for_alarm(
            "alarm-001",
            0
        );

    EXPECT_TRUE(
        zero_alarm_events.empty()
    );

    const auto negative_alarm_events =
        storage.read_recent_for_alarm(
            "alarm-001",
            -1
        );

    EXPECT_TRUE(
        negative_alarm_events.empty()
    );

    const auto zero_global_events =
        storage.read_recent(
            0
        );

    EXPECT_TRUE(
        zero_global_events.empty()
    );

    const auto negative_global_events =
        storage.read_recent(
            -1
        );

    EXPECT_TRUE(
        negative_global_events.empty()
    );
}

TEST(SqliteAlarmEventStorageTests, RejectsEmptyAlarmId)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    auto event =
        dispatcher::storage::sqlite::SqliteAlarmEvent::raised(
            "",
            "pump.pressure",
            "critical",
            "Raised",
            "2026-07-02T10:00:00.000Z"
        );

    EXPECT_THROW(
        {
            const auto id =
                storage.append(
                    event
                );

            static_cast<void>(
                id
            );
        },
        dispatcher::storage::sqlite::SqliteError
    );
}

TEST(SqliteAlarmEventStorageTests, RejectsEmptyTagId)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    auto event =
        dispatcher::storage::sqlite::SqliteAlarmEvent::raised(
            "alarm-001",
            "",
            "critical",
            "Raised",
            "2026-07-02T10:00:00.000Z"
        );

    EXPECT_THROW(
        {
            const auto id =
                storage.append(
                    event
                );

            static_cast<void>(
                id
            );
        },
        dispatcher::storage::sqlite::SqliteError
    );
}

TEST(SqliteAlarmEventStorageTests, RejectsEmptyTimestamp)
{
    auto database =
        dispatcher::storage::sqlite::SqliteDatabase::open_in_memory();

    auto storage =
        make_storage(
            database
        );

    auto event =
        dispatcher::storage::sqlite::SqliteAlarmEvent::raised(
            "alarm-001",
            "pump.pressure",
            "critical",
            "Raised",
            ""
        );

    EXPECT_THROW(
        {
            const auto id =
                storage.append(
                    event
                );

            static_cast<void>(
                id
            );
        },
        dispatcher::storage::sqlite::SqliteError
    );
}

TEST(SqliteAlarmEventStorageTests, PersistsEventsInFileDatabase)
{
    const auto path =
        std::filesystem::temp_directory_path()
        / "dispatcher_sqlite_alarm_event_storage_tests.sqlite";

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

        append_event(
            storage,
            dispatcher::storage::sqlite::SqliteAlarmEvent::raised(
                "alarm-001",
                "pump.pressure",
                "critical",
                "Pressure is above limit",
                "2026-07-02T10:00:00.000Z"
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
            storage.count_for_alarm(
                "alarm-001"
            ),
            1
        );

        const auto latest =
            storage.latest_for_alarm(
                "alarm-001"
            );

        ASSERT_TRUE(
            latest.has_value()
        );

        EXPECT_EQ(
            latest->event_type,
            "raised"
        );

        EXPECT_EQ(
            latest->message,
            "Pressure is above limit"
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