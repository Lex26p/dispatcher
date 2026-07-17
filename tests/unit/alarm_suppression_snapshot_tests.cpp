#include <dispatcher/alarm/alarm_suppression_runtime_snapshot.hpp>

#include <gtest/gtest.h>

TEST(AlarmSuppressionRuntimeSnapshotTests, DefaultSnapshotIsEmpty)
{
    const dispatcher::alarm::AlarmSuppressionRuntimeSnapshot snapshot;

    EXPECT_TRUE(snapshot.empty());
    EXPECT_FALSE(snapshot.has_records());
    EXPECT_FALSE(snapshot.has_active_records());
    EXPECT_FALSE(snapshot.has_expired_records());
    EXPECT_FALSE(snapshot.has_operator_controlled_records());
    EXPECT_FALSE(snapshot.has_system_controlled_records());
    EXPECT_EQ(snapshot.total_command_count(), 0);
}

TEST(AlarmSuppressionRuntimeSnapshotTests, PredicatesReflectFields)
{
    dispatcher::alarm::AlarmSuppressionRuntimeSnapshot snapshot;

    snapshot.store_size = 3;
    snapshot.active_count = 2;
    snapshot.expired_count = 1;
    snapshot.operator_controlled_count = 1;
    snapshot.system_controlled_count = 1;

    snapshot.applied_count = 4;
    snapshot.cleared_count = 2;
    snapshot.rejected_count = 3;

    EXPECT_FALSE(snapshot.empty());
    EXPECT_TRUE(snapshot.has_records());
    EXPECT_TRUE(snapshot.has_active_records());
    EXPECT_TRUE(snapshot.has_expired_records());
    EXPECT_TRUE(snapshot.has_operator_controlled_records());
    EXPECT_TRUE(snapshot.has_system_controlled_records());

    EXPECT_EQ(snapshot.total_command_count(), 9);
}