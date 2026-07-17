#include <dispatcher/runtime/diagnostic_record.hpp>
#include <dispatcher/runtime/health_check_result.hpp>
#include <dispatcher/runtime/health_snapshot.hpp>
#include <dispatcher/runtime/readiness_status.hpp>
#include <dispatcher/runtime/runtime_consistency_issue.hpp>
#include <dispatcher/runtime/runtime_consistency_snapshot.hpp>
#include <dispatcher/runtime/runtime_diagnostics_snapshot.hpp>
#include <dispatcher/runtime/runtime_hardening_snapshot.hpp>
#include <dispatcher/runtime/runtime_hardening_status.hpp>

#include <gtest/gtest.h>

namespace
{
    dispatcher::runtime::HealthSnapshot make_healthy_health_snapshot()
    {
        dispatcher::runtime::HealthSnapshot health;

        health.add_check(
            dispatcher::runtime::HealthCheckResult::healthy(
                "runtime",
                "runtime",
                "runtime is healthy"
            )
        );

        health.add_check(
            dispatcher::runtime::HealthCheckResult::healthy(
                "telemetry",
                "telemetry",
                "telemetry is healthy"
            )
        );

        return health;
    }

    dispatcher::runtime::RuntimeDiagnosticsSnapshot
        make_empty_diagnostics_snapshot()
    {
        return dispatcher::runtime::RuntimeDiagnosticsSnapshot{};
    }

    dispatcher::runtime::RuntimeConsistencySnapshot
        make_empty_consistency_snapshot()
    {
        return dispatcher::runtime::RuntimeConsistencySnapshot{};
    }

    dispatcher::runtime::RuntimeHardeningSnapshot make_passing_snapshot()
    {
        return dispatcher::runtime::RuntimeHardeningSnapshot(
            make_healthy_health_snapshot(),
            make_empty_diagnostics_snapshot(),
            make_empty_consistency_snapshot()
        );
    }
}

TEST(RuntimeHardeningStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::RuntimeHardeningStatus::Unknown
        ),
        "unknown"
    );

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::RuntimeHardeningStatus::Passing
        ),
        "passing"
    );

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::RuntimeHardeningStatus::Warning
        ),
        "warning"
    );

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::RuntimeHardeningStatus::Failing
        ),
        "failing"
    );
}

TEST(RuntimeHardeningStatusTests, PredicatesWork)
{
    EXPECT_FALSE(
        dispatcher::runtime::is_known(
            dispatcher::runtime::RuntimeHardeningStatus::Unknown
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_known(
            dispatcher::runtime::RuntimeHardeningStatus::Passing
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_passing(
            dispatcher::runtime::RuntimeHardeningStatus::Passing
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_warning(
            dispatcher::runtime::RuntimeHardeningStatus::Warning
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_failing(
            dispatcher::runtime::RuntimeHardeningStatus::Failing
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::allows_release(
            dispatcher::runtime::RuntimeHardeningStatus::Passing
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::allows_release(
            dispatcher::runtime::RuntimeHardeningStatus::Warning
        )
    );

    EXPECT_FALSE(
        dispatcher::runtime::allows_release(
            dispatcher::runtime::RuntimeHardeningStatus::Failing
        )
    );

    EXPECT_FALSE(
        dispatcher::runtime::requires_action(
            dispatcher::runtime::RuntimeHardeningStatus::Passing
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::requires_action(
            dispatcher::runtime::RuntimeHardeningStatus::Warning
        )
    );
}

TEST(RuntimeHardeningSnapshotTests, EmptySnapshotIsUnknown)
{
    const auto snapshot =
        dispatcher::runtime::RuntimeHardeningSnapshot::empty();

    EXPECT_FALSE(snapshot.has_health_checks());
    EXPECT_FALSE(snapshot.has_diagnostics());
    EXPECT_FALSE(snapshot.has_consistency_issues());

    EXPECT_EQ(snapshot.health_check_count(), 0);
    EXPECT_EQ(snapshot.diagnostic_record_count(), 0);
    EXPECT_EQ(snapshot.consistency_issue_count(), 0);

    EXPECT_EQ(
        snapshot.health_status(),
        dispatcher::runtime::HealthStatus::Unknown
    );

    EXPECT_EQ(
        snapshot.readiness_status(),
        dispatcher::runtime::ReadinessStatus::Unknown
    );

    EXPECT_EQ(
        snapshot.consistency_status(),
        dispatcher::runtime::RuntimeConsistencyStatus::Consistent
    );

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeHardeningStatus::Unknown
    );

    EXPECT_TRUE(snapshot.unknown());
    EXPECT_FALSE(snapshot.passing());
    EXPECT_FALSE(snapshot.warning());
    EXPECT_FALSE(snapshot.failing());

    EXPECT_FALSE(snapshot.production_ready());
    EXPECT_FALSE(snapshot.release_blocked());
}

TEST(RuntimeHardeningSnapshotTests, HealthyCleanSnapshotIsPassingAndProductionReady)
{
    const auto snapshot = make_passing_snapshot();

    EXPECT_TRUE(snapshot.has_health_checks());
    EXPECT_FALSE(snapshot.has_diagnostics());
    EXPECT_FALSE(snapshot.has_consistency_issues());

    EXPECT_EQ(snapshot.health_check_count(), 2);
    EXPECT_EQ(snapshot.diagnostic_record_count(), 0);
    EXPECT_EQ(snapshot.consistency_issue_count(), 0);

    EXPECT_EQ(
        snapshot.health_status(),
        dispatcher::runtime::HealthStatus::Healthy
    );

    EXPECT_EQ(
        snapshot.readiness_status(),
        dispatcher::runtime::ReadinessStatus::Ready
    );

    EXPECT_EQ(
        snapshot.consistency_status(),
        dispatcher::runtime::RuntimeConsistencyStatus::Consistent
    );

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeHardeningStatus::Passing
    );

    EXPECT_TRUE(snapshot.passing());
    EXPECT_FALSE(snapshot.warning());
    EXPECT_FALSE(snapshot.failing());
    EXPECT_FALSE(snapshot.unknown());

    EXPECT_TRUE(snapshot.production_ready());
    EXPECT_FALSE(snapshot.release_blocked());
    EXPECT_FALSE(snapshot.requires_attention());

    EXPECT_EQ(snapshot.attention_item_count(), 0);
    EXPECT_EQ(snapshot.release_blocker_count(), 0);
}

TEST(RuntimeHardeningSnapshotTests, DegradedHealthProducesWarningButDoesNotBlockRelease)
{
    auto health = make_healthy_health_snapshot();

    health.add_check(
        dispatcher::runtime::HealthCheckResult::degraded(
            "history-lag",
            "history",
            "history writer is behind",
            true
        )
    );

    const dispatcher::runtime::RuntimeHardeningSnapshot snapshot(
        health,
        make_empty_diagnostics_snapshot(),
        make_empty_consistency_snapshot()
    );

    EXPECT_EQ(
        snapshot.health_status(),
        dispatcher::runtime::HealthStatus::Degraded
    );

    EXPECT_EQ(
        snapshot.readiness_status(),
        dispatcher::runtime::ReadinessStatus::Ready
    );

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeHardeningStatus::Warning
    );

    EXPECT_FALSE(snapshot.production_ready());
    EXPECT_FALSE(snapshot.release_blocked());
    EXPECT_TRUE(snapshot.requires_attention());

    EXPECT_EQ(snapshot.attention_item_count(), 1);
    EXPECT_EQ(snapshot.release_blocker_count(), 0);
}

TEST(RuntimeHardeningSnapshotTests, MandatoryUnhealthyHealthBlocksRelease)
{
    auto health = make_healthy_health_snapshot();

    health.add_check(
        dispatcher::runtime::HealthCheckResult::unhealthy(
            "storage",
            "storage",
            "storage unavailable",
            true
        )
    );

    const dispatcher::runtime::RuntimeHardeningSnapshot snapshot(
        health,
        make_empty_diagnostics_snapshot(),
        make_empty_consistency_snapshot()
    );

    EXPECT_EQ(
        snapshot.health_status(),
        dispatcher::runtime::HealthStatus::Unhealthy
    );

    EXPECT_EQ(
        snapshot.readiness_status(),
        dispatcher::runtime::ReadinessStatus::NotReady
    );

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeHardeningStatus::Failing
    );

    EXPECT_FALSE(snapshot.production_ready());
    EXPECT_TRUE(snapshot.release_blocked());
    EXPECT_TRUE(snapshot.requires_attention());

    EXPECT_EQ(snapshot.release_blocker_count(), 1);
}

TEST(RuntimeHardeningSnapshotTests, OptionalUnhealthyHealthProducesWarning)
{
    auto health = make_healthy_health_snapshot();

    health.add_check(
        dispatcher::runtime::HealthCheckResult::unhealthy(
            "notifications",
            "notifications",
            "notification adapter unavailable",
            false
        )
    );

    const dispatcher::runtime::RuntimeHardeningSnapshot snapshot(
        health,
        make_empty_diagnostics_snapshot(),
        make_empty_consistency_snapshot()
    );

    EXPECT_EQ(
        snapshot.readiness_status(),
        dispatcher::runtime::ReadinessStatus::Ready
    );

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeHardeningStatus::Warning
    );

    EXPECT_FALSE(snapshot.production_ready());
    EXPECT_FALSE(snapshot.release_blocked());
    EXPECT_TRUE(snapshot.requires_attention());
}

TEST(RuntimeHardeningSnapshotTests, WarningDiagnosticsProduceWarning)
{
    dispatcher::runtime::RuntimeDiagnosticsSnapshot diagnostics;

    diagnostics.add_record(
        dispatcher::runtime::DiagnosticRecord::warning(
            "history",
            "history.writer_lag",
            "history writer is behind"
        )
    );

    const dispatcher::runtime::RuntimeHardeningSnapshot snapshot(
        make_healthy_health_snapshot(),
        diagnostics,
        make_empty_consistency_snapshot()
    );

    EXPECT_EQ(snapshot.diagnostic_record_count(), 1);

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeHardeningStatus::Warning
    );

    EXPECT_FALSE(snapshot.production_ready());
    EXPECT_FALSE(snapshot.release_blocked());
    EXPECT_TRUE(snapshot.requires_attention());

    EXPECT_EQ(snapshot.attention_item_count(), 1);
}

TEST(RuntimeHardeningSnapshotTests, CriticalDiagnosticsBlockRelease)
{
    dispatcher::runtime::RuntimeDiagnosticsSnapshot diagnostics;

    diagnostics.add_record(
        dispatcher::runtime::DiagnosticRecord::critical(
            "runtime",
            "runtime.invariant_broken",
            "runtime invariant is broken"
        )
    );

    const dispatcher::runtime::RuntimeHardeningSnapshot snapshot(
        make_healthy_health_snapshot(),
        diagnostics,
        make_empty_consistency_snapshot()
    );

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeHardeningStatus::Failing
    );

    EXPECT_FALSE(snapshot.production_ready());
    EXPECT_TRUE(snapshot.release_blocked());
    EXPECT_TRUE(snapshot.requires_attention());

    EXPECT_EQ(snapshot.release_blocker_count(), 1);
}

TEST(RuntimeHardeningSnapshotTests, InvalidDiagnosticsBlockRelease)
{
    dispatcher::runtime::RuntimeDiagnosticsSnapshot diagnostics;

    diagnostics.add_record(
        dispatcher::runtime::DiagnosticRecord(
            dispatcher::runtime::DiagnosticSeverity::Unknown,
            "runtime",
            "runtime.unknown",
            "runtime status is unknown"
        )
    );

    const dispatcher::runtime::RuntimeHardeningSnapshot snapshot(
        make_healthy_health_snapshot(),
        diagnostics,
        make_empty_consistency_snapshot()
    );

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeHardeningStatus::Failing
    );

    EXPECT_TRUE(snapshot.release_blocked());
    EXPECT_TRUE(snapshot.requires_attention());

    EXPECT_EQ(snapshot.release_blocker_count(), 1);
}

TEST(RuntimeHardeningSnapshotTests, NonBlockingConsistencyWarningProducesWarning)
{
    dispatcher::runtime::RuntimeConsistencySnapshot consistency;

    consistency.add_issue(
        dispatcher::runtime::RuntimeConsistencyIssue::warning(
            "configuration",
            "configuration.unused_tag",
            "configuration contains an unused tag"
        )
    );

    const dispatcher::runtime::RuntimeHardeningSnapshot snapshot(
        make_healthy_health_snapshot(),
        make_empty_diagnostics_snapshot(),
        consistency
    );

    EXPECT_EQ(snapshot.consistency_issue_count(), 1);

    EXPECT_EQ(
        snapshot.consistency_status(),
        dispatcher::runtime::RuntimeConsistencyStatus::Consistent
    );

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeHardeningStatus::Warning
    );

    EXPECT_FALSE(snapshot.production_ready());
    EXPECT_FALSE(snapshot.release_blocked());
    EXPECT_TRUE(snapshot.requires_attention());
}

TEST(RuntimeHardeningSnapshotTests, BlockingConsistencyErrorBlocksRelease)
{
    dispatcher::runtime::RuntimeConsistencySnapshot consistency;

    consistency.add_issue(
        dispatcher::runtime::RuntimeConsistencyIssue::error(
            "runtime",
            "runtime.missing_configuration",
            "runtime has no active configuration"
        )
    );

    const dispatcher::runtime::RuntimeHardeningSnapshot snapshot(
        make_healthy_health_snapshot(),
        make_empty_diagnostics_snapshot(),
        consistency
    );

    EXPECT_EQ(
        snapshot.consistency_status(),
        dispatcher::runtime::RuntimeConsistencyStatus::Inconsistent
    );

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeHardeningStatus::Failing
    );

    EXPECT_FALSE(snapshot.production_ready());
    EXPECT_TRUE(snapshot.release_blocked());
    EXPECT_TRUE(snapshot.requires_attention());

    EXPECT_EQ(snapshot.release_blocker_count(), 1);
}

TEST(RuntimeHardeningSnapshotTests, InvalidConsistencyProducesUnknown)
{
    dispatcher::runtime::RuntimeConsistencySnapshot consistency;

    consistency.add_issue(
        dispatcher::runtime::RuntimeConsistencyIssue(
            dispatcher::runtime::DiagnosticSeverity::Unknown,
            "runtime",
            "runtime.unknown",
            "runtime status is unknown"
        )
    );

    const dispatcher::runtime::RuntimeHardeningSnapshot snapshot(
        make_healthy_health_snapshot(),
        make_empty_diagnostics_snapshot(),
        consistency
    );

    EXPECT_EQ(
        snapshot.consistency_status(),
        dispatcher::runtime::RuntimeConsistencyStatus::Unknown
    );

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeHardeningStatus::Unknown
    );

    EXPECT_TRUE(snapshot.unknown());
    EXPECT_FALSE(snapshot.production_ready());
    EXPECT_TRUE(snapshot.release_blocked());
    EXPECT_TRUE(snapshot.requires_attention());
}

TEST(RuntimeHardeningSnapshotTests, CountsAggregateAcrossSubsystems)
{
    auto health = make_healthy_health_snapshot();

    health.add_check(
        dispatcher::runtime::HealthCheckResult::degraded(
            "history-lag",
            "history",
            "history writer is behind"
        )
    );

    dispatcher::runtime::RuntimeDiagnosticsSnapshot diagnostics;

    diagnostics.add_record(
        dispatcher::runtime::DiagnosticRecord::warning(
            "telemetry",
            "telemetry.stale",
            "telemetry is stale"
        )
    );

    dispatcher::runtime::RuntimeConsistencySnapshot consistency;

    consistency.add_issue(
        dispatcher::runtime::RuntimeConsistencyIssue::warning(
            "configuration",
            "configuration.unused_tag",
            "configuration contains an unused tag"
        )
    );

    const dispatcher::runtime::RuntimeHardeningSnapshot snapshot(
        health,
        diagnostics,
        consistency
    );

    EXPECT_EQ(snapshot.health_check_count(), 3);
    EXPECT_EQ(snapshot.diagnostic_record_count(), 1);
    EXPECT_EQ(snapshot.consistency_issue_count(), 1);

    EXPECT_EQ(snapshot.attention_item_count(), 3);
    EXPECT_EQ(snapshot.release_blocker_count(), 0);

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeHardeningStatus::Warning
    );
}