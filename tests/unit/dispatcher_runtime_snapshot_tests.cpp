#include <dispatcher/runtime/dispatcher_runtime_snapshot.hpp>

#include <gtest/gtest.h>

TEST(DispatcherRuntimeSnapshotTests, DefaultSnapshotIsEmpty)
{
    const dispatcher::runtime::DispatcherRuntimeSnapshot snapshot;

    EXPECT_FALSE(snapshot.has_current_state());
    EXPECT_FALSE(snapshot.has_history_samples());
    EXPECT_FALSE(snapshot.has_alarm_events());
    EXPECT_FALSE(snapshot.has_acknowledgement_audit());
    EXPECT_FALSE(snapshot.requires_operator_attention());
}

TEST(DispatcherRuntimeSnapshotTests, SnapshotHelpersReflectNestedRuntimeState)
{
    dispatcher::runtime::DispatcherRuntimeSnapshot snapshot;

    snapshot.telemetry.current_state_size = 3;
    snapshot.history.store_size = 2;
    snapshot.alarm.event_store_size = 1;
    snapshot.alarm.acknowledgement_store_size = 1;
    snapshot.alarm_operator.unacknowledged_alarm_count = 1;

    EXPECT_TRUE(snapshot.has_current_state());
    EXPECT_TRUE(snapshot.has_history_samples());
    EXPECT_TRUE(snapshot.has_alarm_events());
    EXPECT_TRUE(snapshot.has_acknowledgement_audit());
    EXPECT_TRUE(snapshot.requires_operator_attention());
}