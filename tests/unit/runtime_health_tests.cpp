#include <dispatcher/runtime/health_check_result.hpp>
#include <dispatcher/runtime/health_snapshot.hpp>
#include <dispatcher/runtime/health_status.hpp>
#include <dispatcher/runtime/readiness_status.hpp>

#include <gtest/gtest.h>

#include <chrono>

TEST(HealthStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::HealthStatus::Unknown
        ),
        "unknown"
    );

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::HealthStatus::Healthy
        ),
        "healthy"
    );

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::HealthStatus::Degraded
        ),
        "degraded"
    );

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::HealthStatus::Unhealthy
        ),
        "unhealthy"
    );
}

TEST(HealthStatusTests, PredicatesWork)
{
    EXPECT_FALSE(
        dispatcher::runtime::is_known(
            dispatcher::runtime::HealthStatus::Unknown
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_known(
            dispatcher::runtime::HealthStatus::Healthy
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_healthy(
            dispatcher::runtime::HealthStatus::Healthy
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_degraded(
            dispatcher::runtime::HealthStatus::Degraded
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_unhealthy(
            dispatcher::runtime::HealthStatus::Unhealthy
        )
    );

    EXPECT_FALSE(
        dispatcher::runtime::requires_attention(
            dispatcher::runtime::HealthStatus::Healthy
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::requires_attention(
            dispatcher::runtime::HealthStatus::Degraded
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::requires_attention(
            dispatcher::runtime::HealthStatus::Unhealthy
        )
    );
}

TEST(ReadinessStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::ReadinessStatus::Unknown
        ),
        "unknown"
    );

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::ReadinessStatus::Ready
        ),
        "ready"
    );

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::ReadinessStatus::NotReady
        ),
        "not_ready"
    );
}

TEST(ReadinessStatusTests, PredicatesWork)
{
    EXPECT_FALSE(
        dispatcher::runtime::is_known(
            dispatcher::runtime::ReadinessStatus::Unknown
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_ready(
            dispatcher::runtime::ReadinessStatus::Ready
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_not_ready(
            dispatcher::runtime::ReadinessStatus::NotReady
        )
    );
}

TEST(HealthCheckResultTests, HealthyFactoryCreatesValidCheck)
{
    const auto check =
        dispatcher::runtime::HealthCheckResult::healthy(
            "runtime-ready",
            "runtime",
            "runtime is ready",
            true,
            std::chrono::milliseconds{ 7 }
        );

    EXPECT_EQ(check.name(), "runtime-ready");
    EXPECT_EQ(check.component(), "runtime");
    EXPECT_EQ(check.message(), "runtime is ready");
    EXPECT_EQ(check.duration(), std::chrono::milliseconds{ 7 });

    EXPECT_TRUE(check.mandatory());
    EXPECT_FALSE(check.optional());

    EXPECT_TRUE(check.has_name());
    EXPECT_TRUE(check.has_component());
    EXPECT_TRUE(check.has_message());

    EXPECT_TRUE(check.valid());
    EXPECT_TRUE(check.healthy());
    EXPECT_FALSE(check.degraded());
    EXPECT_FALSE(check.unhealthy());

    EXPECT_FALSE(check.requires_attention());
    EXPECT_FALSE(check.blocks_readiness());
}

TEST(HealthCheckResultTests, DegradedCheckRequiresAttentionButDoesNotBlockReadiness)
{
    const auto check =
        dispatcher::runtime::HealthCheckResult::degraded(
            "history-lag",
            "history",
            "history writer is behind",
            true
        );

    EXPECT_TRUE(check.valid());

    EXPECT_EQ(
        check.status(),
        dispatcher::runtime::HealthStatus::Degraded
    );

    EXPECT_FALSE(check.healthy());
    EXPECT_TRUE(check.degraded());
    EXPECT_FALSE(check.unhealthy());

    EXPECT_TRUE(check.requires_attention());
    EXPECT_FALSE(check.blocks_readiness());
}

TEST(HealthCheckResultTests, MandatoryUnhealthyCheckBlocksReadiness)
{
    const auto check =
        dispatcher::runtime::HealthCheckResult::unhealthy(
            "storage",
            "storage",
            "storage unavailable",
            true
        );

    EXPECT_TRUE(check.valid());
    EXPECT_TRUE(check.unhealthy());
    EXPECT_TRUE(check.requires_attention());
    EXPECT_TRUE(check.blocks_readiness());
}

TEST(HealthCheckResultTests, OptionalUnhealthyCheckDoesNotBlockReadiness)
{
    const auto check =
        dispatcher::runtime::HealthCheckResult::unhealthy(
            "notifications",
            "notifications",
            "notification adapter unavailable",
            false
        );

    EXPECT_TRUE(check.valid());
    EXPECT_TRUE(check.optional());
    EXPECT_TRUE(check.unhealthy());
    EXPECT_TRUE(check.requires_attention());
    EXPECT_FALSE(check.blocks_readiness());
}

TEST(HealthCheckResultTests, UnknownOrMissingFieldsAreInvalid)
{
    const dispatcher::runtime::HealthCheckResult missing_name(
        "",
        "runtime",
        dispatcher::runtime::HealthStatus::Healthy
    );

    EXPECT_FALSE(missing_name.valid());

    const dispatcher::runtime::HealthCheckResult missing_component(
        "runtime-ready",
        "",
        dispatcher::runtime::HealthStatus::Healthy
    );

    EXPECT_FALSE(missing_component.valid());

    const dispatcher::runtime::HealthCheckResult unknown_status(
        "runtime-ready",
        "runtime",
        dispatcher::runtime::HealthStatus::Unknown
    );

    EXPECT_FALSE(unknown_status.valid());
    EXPECT_TRUE(unknown_status.blocks_readiness());
}

TEST(HealthSnapshotTests, EmptySnapshotIsUnknown)
{
    const dispatcher::runtime::HealthSnapshot snapshot;

    EXPECT_TRUE(snapshot.empty());
    EXPECT_FALSE(snapshot.has_checks());

    EXPECT_EQ(snapshot.check_count(), 0);
    EXPECT_EQ(snapshot.healthy_count(), 0);
    EXPECT_EQ(snapshot.degraded_count(), 0);
    EXPECT_EQ(snapshot.unhealthy_count(), 0);
    EXPECT_EQ(snapshot.invalid_count(), 0);

    EXPECT_EQ(
        snapshot.overall_status(),
        dispatcher::runtime::HealthStatus::Unknown
    );

    EXPECT_EQ(
        snapshot.readiness_status(),
        dispatcher::runtime::ReadinessStatus::Unknown
    );

    EXPECT_FALSE(snapshot.healthy());
    EXPECT_FALSE(snapshot.ready());
}

TEST(HealthSnapshotTests, HealthyChecksProduceHealthyReadySnapshot)
{
    dispatcher::runtime::HealthSnapshot snapshot;

    snapshot.add_check(
        dispatcher::runtime::HealthCheckResult::healthy(
            "runtime",
            "runtime"
        )
    );

    snapshot.add_check(
        dispatcher::runtime::HealthCheckResult::healthy(
            "telemetry",
            "telemetry"
        )
    );

    EXPECT_FALSE(snapshot.empty());
    EXPECT_EQ(snapshot.check_count(), 2);
    EXPECT_EQ(snapshot.healthy_count(), 2);
    EXPECT_EQ(snapshot.degraded_count(), 0);
    EXPECT_EQ(snapshot.unhealthy_count(), 0);

    EXPECT_EQ(snapshot.mandatory_count(), 2);
    EXPECT_EQ(snapshot.optional_count(), 0);

    EXPECT_EQ(
        snapshot.overall_status(),
        dispatcher::runtime::HealthStatus::Healthy
    );

    EXPECT_EQ(
        snapshot.readiness_status(),
        dispatcher::runtime::ReadinessStatus::Ready
    );

    EXPECT_TRUE(snapshot.healthy());
    EXPECT_TRUE(snapshot.ready());
}

TEST(HealthSnapshotTests, DegradedCheckProducesDegradedButReadySnapshot)
{
    dispatcher::runtime::HealthSnapshot snapshot;

    snapshot.add_check(
        dispatcher::runtime::HealthCheckResult::healthy(
            "runtime",
            "runtime"
        )
    );

    snapshot.add_check(
        dispatcher::runtime::HealthCheckResult::degraded(
            "history-lag",
            "history",
            "history writer is behind"
        )
    );

    EXPECT_EQ(snapshot.healthy_count(), 1);
    EXPECT_EQ(snapshot.degraded_count(), 1);
    EXPECT_EQ(snapshot.unhealthy_count(), 0);

    EXPECT_TRUE(snapshot.has_degraded_checks());
    EXPECT_FALSE(snapshot.has_readiness_blockers());

    EXPECT_EQ(
        snapshot.overall_status(),
        dispatcher::runtime::HealthStatus::Degraded
    );

    EXPECT_EQ(
        snapshot.readiness_status(),
        dispatcher::runtime::ReadinessStatus::Ready
    );

    EXPECT_FALSE(snapshot.healthy());
    EXPECT_TRUE(snapshot.ready());
}

TEST(HealthSnapshotTests, MandatoryUnhealthyCheckProducesUnhealthyNotReadySnapshot)
{
    dispatcher::runtime::HealthSnapshot snapshot;

    snapshot.add_check(
        dispatcher::runtime::HealthCheckResult::healthy(
            "runtime",
            "runtime"
        )
    );

    snapshot.add_check(
        dispatcher::runtime::HealthCheckResult::unhealthy(
            "storage",
            "storage",
            "storage unavailable",
            true
        )
    );

    EXPECT_EQ(snapshot.healthy_count(), 1);
    EXPECT_EQ(snapshot.unhealthy_count(), 1);

    EXPECT_TRUE(snapshot.has_unhealthy_checks());
    EXPECT_TRUE(snapshot.has_readiness_blockers());

    EXPECT_EQ(
        snapshot.overall_status(),
        dispatcher::runtime::HealthStatus::Unhealthy
    );

    EXPECT_EQ(
        snapshot.readiness_status(),
        dispatcher::runtime::ReadinessStatus::NotReady
    );

    EXPECT_FALSE(snapshot.healthy());
    EXPECT_FALSE(snapshot.ready());
}

TEST(HealthSnapshotTests, OptionalUnhealthyCheckProducesUnhealthyButReadySnapshot)
{
    dispatcher::runtime::HealthSnapshot snapshot;

    snapshot.add_check(
        dispatcher::runtime::HealthCheckResult::healthy(
            "runtime",
            "runtime"
        )
    );

    snapshot.add_check(
        dispatcher::runtime::HealthCheckResult::unhealthy(
            "notifications",
            "notifications",
            "notification adapter down",
            false
        )
    );

    EXPECT_EQ(snapshot.mandatory_count(), 1);
    EXPECT_EQ(snapshot.optional_count(), 1);

    EXPECT_TRUE(snapshot.has_unhealthy_checks());
    EXPECT_FALSE(snapshot.has_readiness_blockers());

    EXPECT_EQ(
        snapshot.overall_status(),
        dispatcher::runtime::HealthStatus::Unhealthy
    );

    EXPECT_EQ(
        snapshot.readiness_status(),
        dispatcher::runtime::ReadinessStatus::Ready
    );

    EXPECT_FALSE(snapshot.healthy());
    EXPECT_TRUE(snapshot.ready());
}

TEST(HealthSnapshotTests, InvalidCheckProducesUnknownNotReadySnapshot)
{
    dispatcher::runtime::HealthSnapshot snapshot;

    snapshot.add_check(
        dispatcher::runtime::HealthCheckResult(
            "",
            "runtime",
            dispatcher::runtime::HealthStatus::Healthy
        )
    );

    EXPECT_TRUE(snapshot.has_invalid_checks());

    EXPECT_EQ(
        snapshot.overall_status(),
        dispatcher::runtime::HealthStatus::Unknown
    );

    EXPECT_EQ(
        snapshot.readiness_status(),
        dispatcher::runtime::ReadinessStatus::NotReady
    );
}

TEST(HealthSnapshotTests, ChecksCanBeFilteredByComponent)
{
    dispatcher::runtime::HealthSnapshot snapshot;

    snapshot.add_check(
        dispatcher::runtime::HealthCheckResult::healthy(
            "runtime-ready",
            "runtime"
        )
    );

    snapshot.add_check(
        dispatcher::runtime::HealthCheckResult::healthy(
            "runtime-threads",
            "runtime"
        )
    );

    snapshot.add_check(
        dispatcher::runtime::HealthCheckResult::healthy(
            "storage-ready",
            "storage"
        )
    );

    const auto runtime_checks =
        snapshot.checks_for_component("runtime");

    ASSERT_EQ(runtime_checks.size(), 2);

    EXPECT_EQ(runtime_checks[0].component(), "runtime");
    EXPECT_EQ(runtime_checks[1].component(), "runtime");

    const auto missing_checks =
        snapshot.checks_for_component("missing");

    EXPECT_TRUE(missing_checks.empty());
}

TEST(HealthSnapshotTests, ClearRemovesChecks)
{
    dispatcher::runtime::HealthSnapshot snapshot;

    snapshot.add_check(
        dispatcher::runtime::HealthCheckResult::healthy(
            "runtime",
            "runtime"
        )
    );

    ASSERT_EQ(snapshot.check_count(), 1);

    snapshot.clear();

    EXPECT_TRUE(snapshot.empty());
    EXPECT_EQ(snapshot.check_count(), 0);
}