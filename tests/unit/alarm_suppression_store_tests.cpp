#include <dispatcher/alarm/alarm_suppression_command.hpp>
#include <dispatcher/alarm/alarm_suppression_mode.hpp>
#include <dispatcher/alarm/alarm_suppression_reason.hpp>
#include <dispatcher/alarm/alarm_suppression_status.hpp>
#include <dispatcher/alarm/alarm_suppression_store.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/alarm/alarm_suppression_runtime_snapshot.hpp>

#include <gtest/gtest.h>

#include <chrono>

namespace
{
    dispatcher::alarm::AlarmSuppressionCommand make_store_command(
        dispatcher::domain::AlarmId alarm_id =
        dispatcher::domain::AlarmId{ "alarm-1" },
        std::optional<dispatcher::alarm::AlarmSuppressionCommand::TimePoint>
        expires_at = std::nullopt,
        dispatcher::alarm::AlarmSuppressionMode mode =
        dispatcher::alarm::AlarmSuppressionMode::Shelved,
        dispatcher::alarm::AlarmSuppressionReason reason =
        dispatcher::alarm::AlarmSuppressionReason::Maintenance
    )
    {
        return dispatcher::alarm::AlarmSuppressionCommand(
            alarm_id,
            "operator-1",
            mode,
            reason,
            "planned maintenance",
            expires_at
        );
    }
}

TEST(AlarmSuppressionStoreTests, NewStoreIsEmpty)
{
    const dispatcher::alarm::AlarmSuppressionStore store;

    EXPECT_TRUE(store.empty());
    EXPECT_EQ(store.size(), 0);
    EXPECT_TRUE(store.records().empty());
}

TEST(AlarmSuppressionStoreTests, ApplyStoresActiveRecord)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto now = Store::Clock::now();

    const auto result = store.apply(
        make_store_command(
            dispatcher::domain::AlarmId{ "alarm-1" },
            now + std::chrono::minutes(30)
        ),
        now
    );

    ASSERT_TRUE(result.success());
    ASSERT_TRUE(result.applied());
    ASSERT_TRUE(result.has_record());

    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::AlarmSuppressionStatus::Applied
    );

    EXPECT_FALSE(store.empty());
    EXPECT_EQ(store.size(), 1);

    EXPECT_TRUE(
        store.is_active(
            dispatcher::domain::AlarmId{ "alarm-1" },
            now
        )
    );

    const auto* record = store.find_by_alarm_id(
        dispatcher::domain::AlarmId{ "alarm-1" },
        now
    );

    ASSERT_NE(record, nullptr);

    EXPECT_EQ(record->alarm_id(), dispatcher::domain::AlarmId{ "alarm-1" });
    EXPECT_EQ(record->operator_id(), "operator-1");

    EXPECT_EQ(
        record->mode(),
        dispatcher::alarm::AlarmSuppressionMode::Shelved
    );

    EXPECT_EQ(
        record->reason(),
        dispatcher::alarm::AlarmSuppressionReason::Maintenance
    );

    EXPECT_EQ(record->comment(), "planned maintenance");
}

TEST(AlarmSuppressionStoreTests, ApplyRejectsInvalidCommand)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto now = Store::Clock::now();

    const dispatcher::alarm::AlarmSuppressionCommand command(
        dispatcher::domain::AlarmId{ "" },
        "",
        dispatcher::alarm::AlarmSuppressionMode::Shelved,
        dispatcher::alarm::AlarmSuppressionReason::Unknown
    );

    const auto result = store.apply(command, now);

    EXPECT_FALSE(result.success());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::AlarmSuppressionStatus::InvalidCommand
    );

    EXPECT_FALSE(result.has_record());
    EXPECT_TRUE(store.empty());
}

TEST(AlarmSuppressionStoreTests, ApplyRejectsExpiredCommand)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto now = Store::Clock::now();

    const auto result = store.apply(
        make_store_command(
            dispatcher::domain::AlarmId{ "alarm-1" },
            now - std::chrono::seconds(1)
        ),
        now
    );

    EXPECT_FALSE(result.success());

    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::AlarmSuppressionStatus::Expired
    );

    EXPECT_TRUE(store.empty());
}

TEST(AlarmSuppressionStoreTests, ApplyRejectsAlreadySuppressedAlarm)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto now = Store::Clock::now();

    const auto first = store.apply(
        make_store_command(
            dispatcher::domain::AlarmId{ "alarm-1" },
            now + std::chrono::minutes(30)
        ),
        now
    );

    ASSERT_TRUE(first.success());

    const auto second = store.apply(
        make_store_command(
            dispatcher::domain::AlarmId{ "alarm-1" },
            now + std::chrono::minutes(30)
        ),
        now
    );

    EXPECT_FALSE(second.success());

    EXPECT_EQ(
        second.status(),
        dispatcher::alarm::AlarmSuppressionStatus::AlreadySuppressed
    );

    EXPECT_EQ(store.size(), 1);
}

TEST(AlarmSuppressionStoreTests, ApplyReplacesExpiredExistingRecord)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto now = Store::Clock::now();

    const auto first = store.apply(
        make_store_command(
            dispatcher::domain::AlarmId{ "alarm-1" },
            now + std::chrono::minutes(5)
        ),
        now
    );

    ASSERT_TRUE(first.success());

    const auto later = now + std::chrono::minutes(10);

    EXPECT_FALSE(
        store.is_active(
            dispatcher::domain::AlarmId{ "alarm-1" },
            later
        )
    );

    const auto second = store.apply(
        make_store_command(
            dispatcher::domain::AlarmId{ "alarm-1" },
            later + std::chrono::minutes(30)
        ),
        later
    );

    EXPECT_TRUE(second.success());
    EXPECT_EQ(store.size(), 1);

    EXPECT_TRUE(
        store.is_active(
            dispatcher::domain::AlarmId{ "alarm-1" },
            later
        )
    );
}

TEST(AlarmSuppressionStoreTests, ClearRemovesActiveRecord)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto now = Store::Clock::now();

    const auto apply_result = store.apply(
        make_store_command(
            dispatcher::domain::AlarmId{ "alarm-1" },
            now + std::chrono::minutes(30)
        ),
        now
    );

    ASSERT_TRUE(apply_result.success());

    const auto clear_result = store.clear(
        dispatcher::domain::AlarmId{ "alarm-1" },
        now
    );

    EXPECT_TRUE(clear_result.success());
    EXPECT_TRUE(clear_result.cleared());
    ASSERT_TRUE(clear_result.has_record());

    EXPECT_EQ(
        clear_result.status(),
        dispatcher::alarm::AlarmSuppressionStatus::Cleared
    );

    EXPECT_EQ(
        clear_result.record().alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-1" }
    );

    EXPECT_TRUE(store.empty());

    EXPECT_FALSE(
        store.is_active(
            dispatcher::domain::AlarmId{ "alarm-1" },
            now
        )
    );
}

TEST(AlarmSuppressionStoreTests, ClearRejectsMissingAlarmId)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto result = store.clear(
        dispatcher::domain::AlarmId{ "" },
        Store::Clock::now()
    );

    EXPECT_FALSE(result.success());

    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::AlarmSuppressionStatus::InvalidCommand
    );
}

TEST(AlarmSuppressionStoreTests, ClearRejectsNotSuppressedAlarm)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto result = store.clear(
        dispatcher::domain::AlarmId{ "alarm-1" },
        Store::Clock::now()
    );

    EXPECT_FALSE(result.success());

    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::AlarmSuppressionStatus::NotSuppressed
    );
}

TEST(AlarmSuppressionStoreTests, ClearRemovesExpiredRecordAndReturnsExpired)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto now = Store::Clock::now();

    const auto apply_result = store.apply(
        make_store_command(
            dispatcher::domain::AlarmId{ "alarm-1" },
            now + std::chrono::minutes(5)
        ),
        now
    );

    ASSERT_TRUE(apply_result.success());

    const auto result = store.clear(
        dispatcher::domain::AlarmId{ "alarm-1" },
        now + std::chrono::minutes(10)
    );

    EXPECT_FALSE(result.success());

    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::AlarmSuppressionStatus::Expired
    );

    EXPECT_TRUE(store.empty());
}

TEST(AlarmSuppressionStoreTests, ActiveRecordsReturnsOnlyNonExpiredRecords)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto now = Store::Clock::now();

    ASSERT_TRUE(
        store.apply(
            make_store_command(
                dispatcher::domain::AlarmId{ "alarm-1" },
                now + std::chrono::minutes(5)
            ),
            now
        ).success()
    );

    ASSERT_TRUE(
        store.apply(
            make_store_command(
                dispatcher::domain::AlarmId{ "alarm-2" },
                now + std::chrono::minutes(30)
            ),
            now
        ).success()
    );

    const auto active = store.active_records(
        now + std::chrono::minutes(10)
    );

    ASSERT_EQ(active.size(), 1);

    EXPECT_EQ(
        active.front().alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-2" }
    );
}

TEST(AlarmSuppressionStoreTests, RemoveExpiredDeletesOnlyExpiredRecords)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto now = Store::Clock::now();

    ASSERT_TRUE(
        store.apply(
            make_store_command(
                dispatcher::domain::AlarmId{ "alarm-1" },
                now + std::chrono::minutes(5)
            ),
            now
        ).success()
    );

    ASSERT_TRUE(
        store.apply(
            make_store_command(
                dispatcher::domain::AlarmId{ "alarm-2" },
                now + std::chrono::minutes(30)
            ),
            now
        ).success()
    );

    ASSERT_EQ(store.size(), 2);

    const auto removed = store.remove_expired(
        now + std::chrono::minutes(10)
    );

    EXPECT_EQ(removed, 1);
    EXPECT_EQ(store.size(), 1);

    EXPECT_FALSE(
        store.is_active(
            dispatcher::domain::AlarmId{ "alarm-1" },
            now + std::chrono::minutes(10)
        )
    );

    EXPECT_TRUE(
        store.is_active(
            dispatcher::domain::AlarmId{ "alarm-2" },
            now + std::chrono::minutes(10)
        )
    );
}

TEST(AlarmSuppressionStoreTests, ClearAllRemovesAllRecords)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto now = Store::Clock::now();

    ASSERT_TRUE(
        store.apply(
            make_store_command(
                dispatcher::domain::AlarmId{ "alarm-1" },
                now + std::chrono::minutes(30)
            ),
            now
        ).success()
    );

    ASSERT_TRUE(
        store.apply(
            make_store_command(
                dispatcher::domain::AlarmId{ "alarm-2" },
                now + std::chrono::minutes(30)
            ),
            now
        ).success()
    );

    ASSERT_EQ(store.size(), 2);

    store.clear_all();

    EXPECT_TRUE(store.empty());
    EXPECT_EQ(store.size(), 0);
    EXPECT_TRUE(store.records().empty());
}

TEST(AlarmSuppressionStoreTests, RuntimeSnapshotCountsActiveRecordsByMode)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto now = Store::Clock::now();

    ASSERT_TRUE(
        store.apply(
            make_store_command(
                dispatcher::domain::AlarmId{ "alarm-shelved" },
                now + std::chrono::minutes(30),
                dispatcher::alarm::AlarmSuppressionMode::Shelved,
                dispatcher::alarm::AlarmSuppressionReason::Maintenance
            ),
            now
        ).success()
    );

    ASSERT_TRUE(
        store.apply(
            make_store_command(
                dispatcher::domain::AlarmId{ "alarm-suppressed" },
                now + std::chrono::minutes(30),
                dispatcher::alarm::AlarmSuppressionMode::Suppressed,
                dispatcher::alarm::AlarmSuppressionReason::Testing
            ),
            now
        ).success()
    );

    ASSERT_TRUE(
        store.apply(
            make_store_command(
                dispatcher::domain::AlarmId{ "alarm-inhibited" },
                now + std::chrono::minutes(30),
                dispatcher::alarm::AlarmSuppressionMode::Inhibited,
                dispatcher::alarm::AlarmSuppressionReason::ExternalInterlock
            ),
            now
        ).success()
    );

    const auto snapshot = store.runtime_snapshot(now);

    EXPECT_EQ(snapshot.store_size, 3);
    EXPECT_EQ(snapshot.active_count, 3);
    EXPECT_EQ(snapshot.expired_count, 0);

    EXPECT_EQ(snapshot.shelved_count, 1);
    EXPECT_EQ(snapshot.suppressed_count, 1);
    EXPECT_EQ(snapshot.inhibited_count, 1);

    EXPECT_EQ(snapshot.operator_controlled_count, 1);
    EXPECT_EQ(snapshot.system_controlled_count, 2);

    EXPECT_EQ(snapshot.applied_count, 3);
    EXPECT_EQ(snapshot.cleared_count, 0);
    EXPECT_EQ(snapshot.rejected_count, 0);
    EXPECT_EQ(snapshot.total_command_count(), 3);

    EXPECT_FALSE(snapshot.empty());
    EXPECT_TRUE(snapshot.has_records());
    EXPECT_TRUE(snapshot.has_active_records());
    EXPECT_FALSE(snapshot.has_expired_records());
    EXPECT_TRUE(snapshot.has_operator_controlled_records());
    EXPECT_TRUE(snapshot.has_system_controlled_records());
}

TEST(AlarmSuppressionStoreTests, RuntimeSnapshotCountsExpiredRecords)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto now = Store::Clock::now();

    ASSERT_TRUE(
        store.apply(
            make_store_command(
                dispatcher::domain::AlarmId{ "alarm-expiring" },
                now + std::chrono::minutes(5)
            ),
            now
        ).success()
    );

    const auto snapshot = store.runtime_snapshot(
        now + std::chrono::minutes(10)
    );

    EXPECT_EQ(snapshot.store_size, 1);
    EXPECT_EQ(snapshot.active_count, 0);
    EXPECT_EQ(snapshot.expired_count, 1);

    EXPECT_TRUE(snapshot.has_records());
    EXPECT_FALSE(snapshot.has_active_records());
    EXPECT_TRUE(snapshot.has_expired_records());
}

TEST(AlarmSuppressionStoreTests, RuntimeSnapshotTracksRejectedAndClearedCounts)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto now = Store::Clock::now();

    const auto invalid = store.apply(
        dispatcher::alarm::AlarmSuppressionCommand(
            dispatcher::domain::AlarmId{ "" },
            "",
            dispatcher::alarm::AlarmSuppressionMode::Shelved,
            dispatcher::alarm::AlarmSuppressionReason::Unknown
        ),
        now
    );

    ASSERT_TRUE(invalid.failed());

    const auto applied = store.apply(
        make_store_command(
            dispatcher::domain::AlarmId{ "alarm-1" },
            now + std::chrono::minutes(30)
        ),
        now
    );

    ASSERT_TRUE(applied.success());

    const auto duplicate = store.apply(
        make_store_command(
            dispatcher::domain::AlarmId{ "alarm-1" },
            now + std::chrono::minutes(30)
        ),
        now
    );

    ASSERT_TRUE(duplicate.failed());

    const auto cleared = store.clear(
        dispatcher::domain::AlarmId{ "alarm-1" },
        now
    );

    ASSERT_TRUE(cleared.success());

    const auto snapshot = store.runtime_snapshot(now);

    EXPECT_EQ(snapshot.store_size, 0);
    EXPECT_EQ(snapshot.active_count, 0);

    EXPECT_EQ(snapshot.applied_count, 1);
    EXPECT_EQ(snapshot.cleared_count, 1);
    EXPECT_EQ(snapshot.rejected_count, 2);
    EXPECT_EQ(snapshot.total_command_count(), 4);
}

TEST(AlarmSuppressionStoreTests, RuntimeSnapshotTracksExpiredRemovedCount)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto now = Store::Clock::now();

    ASSERT_TRUE(
        store.apply(
            make_store_command(
                dispatcher::domain::AlarmId{ "alarm-1" },
                now + std::chrono::minutes(5)
            ),
            now
        ).success()
    );

    const auto removed = store.remove_expired(
        now + std::chrono::minutes(10)
    );

    ASSERT_EQ(removed, 1);

    const auto snapshot = store.runtime_snapshot(
        now + std::chrono::minutes(10)
    );

    EXPECT_EQ(snapshot.store_size, 0);
    EXPECT_EQ(snapshot.expired_removed_count, 1);
}

TEST(AlarmSuppressionStoreTests, ResetStatisticsClearsCountersOnly)
{
    using Store = dispatcher::alarm::AlarmSuppressionStore;

    Store store;

    const auto now = Store::Clock::now();

    ASSERT_TRUE(
        store.apply(
            make_store_command(
                dispatcher::domain::AlarmId{ "alarm-1" },
                now + std::chrono::minutes(30)
            ),
            now
        ).success()
    );

    ASSERT_EQ(store.runtime_snapshot(now).applied_count, 1);
    ASSERT_EQ(store.runtime_snapshot(now).store_size, 1);

    store.reset_statistics();

    const auto snapshot = store.runtime_snapshot(now);

    EXPECT_EQ(snapshot.store_size, 1);
    EXPECT_EQ(snapshot.active_count, 1);

    EXPECT_EQ(snapshot.applied_count, 0);
    EXPECT_EQ(snapshot.cleared_count, 0);
    EXPECT_EQ(snapshot.rejected_count, 0);
    EXPECT_EQ(snapshot.expired_removed_count, 0);
    EXPECT_EQ(snapshot.total_command_count(), 0);
}