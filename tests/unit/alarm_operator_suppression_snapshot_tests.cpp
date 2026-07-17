#include <dispatcher/alarm/alarm_operator_snapshot.hpp>
#include <dispatcher/alarm/alarm_runtime.hpp>
#include <dispatcher/alarm/alarm_suppression_command.hpp>
#include <dispatcher/alarm/alarm_suppression_mode.hpp>
#include <dispatcher/alarm/alarm_suppression_reason.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <gtest/gtest.h>

namespace
{
    dispatcher::alarm::AlarmSuppressionCommand
        make_operator_snapshot_suppression_command(
            dispatcher::domain::AlarmId alarm_id,
            dispatcher::alarm::AlarmSuppressionMode mode,
            dispatcher::alarm::AlarmSuppressionReason reason
        )
    {
        return dispatcher::alarm::AlarmSuppressionCommand(
            alarm_id,
            "operator-1",
            mode,
            reason,
            "operator snapshot test"
        );
    }
}

TEST(
    AlarmOperatorSuppressionSnapshotTests,
    OperatorSnapshotIncludesShelvedSuppressedAndInhibitedCounts
)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(
        runtime.suppress(
            make_operator_snapshot_suppression_command(
                dispatcher::domain::AlarmId{ "alarm-shelved" },
                dispatcher::alarm::AlarmSuppressionMode::Shelved,
                dispatcher::alarm::AlarmSuppressionReason::Maintenance
            )
        ).success()
    );

    ASSERT_TRUE(
        runtime.suppress(
            make_operator_snapshot_suppression_command(
                dispatcher::domain::AlarmId{ "alarm-suppressed" },
                dispatcher::alarm::AlarmSuppressionMode::Suppressed,
                dispatcher::alarm::AlarmSuppressionReason::Testing
            )
        ).success()
    );

    ASSERT_TRUE(
        runtime.suppress(
            make_operator_snapshot_suppression_command(
                dispatcher::domain::AlarmId{ "alarm-inhibited" },
                dispatcher::alarm::AlarmSuppressionMode::Inhibited,
                dispatcher::alarm::AlarmSuppressionReason::ExternalInterlock
            )
        ).success()
    );

    const auto snapshot = runtime.operator_snapshot();

    EXPECT_EQ(snapshot.suppression_store_size, 3);

    EXPECT_EQ(snapshot.shelved_alarm_count, 1);
    EXPECT_EQ(snapshot.suppressed_alarm_count, 1);
    EXPECT_EQ(snapshot.inhibited_alarm_count, 1);

    EXPECT_EQ(snapshot.operator_controlled_suppression_count, 1);
    EXPECT_EQ(snapshot.system_controlled_suppression_count, 2);

    EXPECT_TRUE(snapshot.has_any_suppression());
    EXPECT_TRUE(snapshot.has_shelved_alarms());
    EXPECT_TRUE(snapshot.has_suppressed_alarms());
    EXPECT_TRUE(snapshot.has_inhibited_alarms());
}

TEST(
    AlarmOperatorSuppressionSnapshotTests,
    OperatorSnapshotUpdatesAfterSuppressionIsCleared
)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(
        runtime.suppress(
            make_operator_snapshot_suppression_command(
                dispatcher::domain::AlarmId{ "alarm-shelved" },
                dispatcher::alarm::AlarmSuppressionMode::Shelved,
                dispatcher::alarm::AlarmSuppressionReason::Maintenance
            )
        ).success()
    );

    ASSERT_TRUE(
        runtime.operator_snapshot().has_any_suppression()
    );

    ASSERT_TRUE(
        runtime.clear_suppression(
            dispatcher::domain::AlarmId{ "alarm-shelved" }
        ).success()
    );

    const auto snapshot = runtime.operator_snapshot();

    EXPECT_EQ(snapshot.suppression_store_size, 0);

    EXPECT_EQ(snapshot.shelved_alarm_count, 0);
    EXPECT_EQ(snapshot.suppressed_alarm_count, 0);
    EXPECT_EQ(snapshot.inhibited_alarm_count, 0);

    EXPECT_EQ(snapshot.operator_controlled_suppression_count, 0);
    EXPECT_EQ(snapshot.system_controlled_suppression_count, 0);

    EXPECT_FALSE(snapshot.has_any_suppression());
    EXPECT_FALSE(snapshot.has_shelved_alarms());
    EXPECT_FALSE(snapshot.has_suppressed_alarms());
    EXPECT_FALSE(snapshot.has_inhibited_alarms());
}