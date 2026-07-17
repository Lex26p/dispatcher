#include <dispatcher/alarm/alarm_runtime.hpp>
#include <dispatcher/alarm/alarm_suppression_command.hpp>
#include <dispatcher/alarm/alarm_suppression_mode.hpp>
#include <dispatcher/alarm/alarm_suppression_reason.hpp>
#include <dispatcher/alarm/alarm_suppression_status.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <optional>

namespace
{
    dispatcher::alarm::AlarmSuppressionCommand make_runtime_suppression_command(
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

TEST(AlarmRuntimeSuppressionTests, SuppressionStoreIsInitiallyEmpty)
{
    const dispatcher::alarm::AlarmRuntime runtime;

    EXPECT_TRUE(runtime.suppression_store().empty());

    const auto snapshot = runtime.runtime_suppression_snapshot();

    EXPECT_TRUE(snapshot.empty());
    EXPECT_EQ(snapshot.store_size, 0);
    EXPECT_EQ(snapshot.active_count, 0);
}

TEST(AlarmRuntimeSuppressionTests, SuppressAddsActiveSuppression)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto result = runtime.suppress(
        make_runtime_suppression_command(
            dispatcher::domain::AlarmId{ "alarm-1" }
        )
    );

    ASSERT_TRUE(result.success());
    ASSERT_TRUE(result.applied());

    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::AlarmSuppressionStatus::Applied
    );

    EXPECT_TRUE(
        runtime.is_suppressed(
            dispatcher::domain::AlarmId{ "alarm-1" }
        )
    );

    EXPECT_EQ(runtime.suppression_store().size(), 1);

    const auto snapshot = runtime.runtime_suppression_snapshot();

    EXPECT_EQ(snapshot.store_size, 1);
    EXPECT_EQ(snapshot.active_count, 1);
    EXPECT_EQ(snapshot.shelved_count, 1);
    EXPECT_EQ(snapshot.operator_controlled_count, 1);
    EXPECT_EQ(snapshot.applied_count, 1);
}

TEST(AlarmRuntimeSuppressionTests, ClearSuppressionRemovesActiveSuppression)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto apply_result = runtime.suppress(
        make_runtime_suppression_command(
            dispatcher::domain::AlarmId{ "alarm-1" }
        )
    );

    ASSERT_TRUE(apply_result.success());

    const auto clear_result = runtime.clear_suppression(
        dispatcher::domain::AlarmId{ "alarm-1" }
    );

    ASSERT_TRUE(clear_result.success());
    ASSERT_TRUE(clear_result.cleared());

    EXPECT_EQ(
        clear_result.status(),
        dispatcher::alarm::AlarmSuppressionStatus::Cleared
    );

    EXPECT_FALSE(
        runtime.is_suppressed(
            dispatcher::domain::AlarmId{ "alarm-1" }
        )
    );

    EXPECT_TRUE(runtime.suppression_store().empty());

    const auto snapshot = runtime.runtime_suppression_snapshot();

    EXPECT_EQ(snapshot.store_size, 0);
    EXPECT_EQ(snapshot.active_count, 0);
    EXPECT_EQ(snapshot.applied_count, 1);
    EXPECT_EQ(snapshot.cleared_count, 1);
}

TEST(AlarmRuntimeSuppressionTests, ClearUnknownSuppressionReturnsFailure)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto result = runtime.clear_suppression(
        dispatcher::domain::AlarmId{ "missing-alarm" }
    );

    EXPECT_FALSE(result.success());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::AlarmSuppressionStatus::NotSuppressed
    );

    EXPECT_EQ(runtime.runtime_suppression_snapshot().rejected_count, 1);
}

TEST(AlarmRuntimeSuppressionTests, SuppressionRuntimeSnapshotCountsModes)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(
        runtime.suppress(
            make_runtime_suppression_command(
                dispatcher::domain::AlarmId{ "alarm-shelved" },
                std::nullopt,
                dispatcher::alarm::AlarmSuppressionMode::Shelved,
                dispatcher::alarm::AlarmSuppressionReason::Maintenance
            )
        ).success()
    );

    ASSERT_TRUE(
        runtime.suppress(
            make_runtime_suppression_command(
                dispatcher::domain::AlarmId{ "alarm-suppressed" },
                std::nullopt,
                dispatcher::alarm::AlarmSuppressionMode::Suppressed,
                dispatcher::alarm::AlarmSuppressionReason::Testing
            )
        ).success()
    );

    ASSERT_TRUE(
        runtime.suppress(
            make_runtime_suppression_command(
                dispatcher::domain::AlarmId{ "alarm-inhibited" },
                std::nullopt,
                dispatcher::alarm::AlarmSuppressionMode::Inhibited,
                dispatcher::alarm::AlarmSuppressionReason::ExternalInterlock
            )
        ).success()
    );

    const auto snapshot = runtime.runtime_suppression_snapshot();

    EXPECT_EQ(snapshot.store_size, 3);
    EXPECT_EQ(snapshot.active_count, 3);

    EXPECT_EQ(snapshot.shelved_count, 1);
    EXPECT_EQ(snapshot.suppressed_count, 1);
    EXPECT_EQ(snapshot.inhibited_count, 1);

    EXPECT_EQ(snapshot.operator_controlled_count, 1);
    EXPECT_EQ(snapshot.system_controlled_count, 2);
}

TEST(AlarmRuntimeSuppressionTests, MainRuntimeSnapshotContainsSuppressionSnapshot)
{
    dispatcher::alarm::AlarmRuntime runtime;

    ASSERT_TRUE(
        runtime.suppress(
            make_runtime_suppression_command(
                dispatcher::domain::AlarmId{ "alarm-1" }
            )
        ).success()
    );

    const auto snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(snapshot.suppression.store_size, 1);
    EXPECT_EQ(snapshot.suppression.active_count, 1);
    EXPECT_EQ(snapshot.suppression.shelved_count, 1);
    EXPECT_EQ(snapshot.suppression.applied_count, 1);
}

TEST(AlarmRuntimeSuppressionTests, SuppressionDoesNotChangeAlarmPipelineState)
{
    dispatcher::alarm::AlarmRuntime runtime;

    const auto before = runtime.runtime_snapshot();

    const auto result = runtime.suppress(
        make_runtime_suppression_command(
            dispatcher::domain::AlarmId{ "alarm-1" }
        )
    );

    ASSERT_TRUE(result.success());

    const auto after = runtime.runtime_snapshot();

    EXPECT_EQ(after.state_store_size, before.state_store_size);
    EXPECT_EQ(after.event_store_size, before.event_store_size);

    EXPECT_EQ(
        after.acknowledgement_store_size,
        before.acknowledgement_store_size
    );

    EXPECT_EQ(after.total_count, before.total_count);
    EXPECT_EQ(after.evaluated_count, before.evaluated_count);
    EXPECT_EQ(after.skipped_count, before.skipped_count);
    EXPECT_EQ(after.activated_count, before.activated_count);
    EXPECT_EQ(after.acknowledged_count, before.acknowledged_count);
    EXPECT_EQ(after.cleared_count, before.cleared_count);
    EXPECT_EQ(after.stored_event_count, before.stored_event_count);

    EXPECT_EQ(after.suppression.store_size, 1);
    EXPECT_EQ(after.suppression.applied_count, 1);
}