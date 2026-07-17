#include <dispatcher/runtime/production_hardening.hpp>

#include <gtest/gtest.h>

TEST(RuntimeProductionHardeningHeaderTests, UmbrellaHeaderCanBeIncluded)
{
    dispatcher::runtime::HealthSnapshot health;

    health.add_check(
        dispatcher::runtime::HealthCheckResult::healthy(
            "runtime",
            "runtime",
            "runtime is healthy"
        )
    );

    dispatcher::runtime::RuntimeDiagnosticsSnapshot diagnostics;

    diagnostics.add_record(
        dispatcher::runtime::DiagnosticRecord::info(
            "runtime",
            "runtime.started",
            "runtime started"
        )
    );

    dispatcher::runtime::RuntimeConsistencySnapshot consistency;

    const dispatcher::runtime::RuntimeHardeningSnapshot hardening(
        health,
        diagnostics,
        consistency
    );

    EXPECT_EQ(
        hardening.health_status(),
        dispatcher::runtime::HealthStatus::Healthy
    );

    EXPECT_EQ(
        hardening.readiness_status(),
        dispatcher::runtime::ReadinessStatus::Ready
    );

    EXPECT_EQ(
        hardening.consistency_status(),
        dispatcher::runtime::RuntimeConsistencyStatus::Consistent
    );

    EXPECT_EQ(
        hardening.status(),
        dispatcher::runtime::RuntimeHardeningStatus::Passing
    );

    EXPECT_TRUE(hardening.production_ready());
    EXPECT_FALSE(hardening.release_blocked());
    EXPECT_FALSE(hardening.requires_attention());

    EXPECT_EQ(hardening.health_check_count(), 1);
    EXPECT_EQ(hardening.diagnostic_record_count(), 1);
    EXPECT_EQ(hardening.consistency_issue_count(), 0);

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::RuntimeHardeningStatus::Passing
        ),
        "passing"
    );
}