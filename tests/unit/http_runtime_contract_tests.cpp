#include <dispatcher/http/http_runtime_contract.hpp>

#include <gtest/gtest.h>

#include <string>

namespace
{
    void expect_contains(
        const std::string& value,
        const std::string& expected
    )
    {
        EXPECT_NE(
            value.find(expected),
            std::string::npos
        ) << value;
    }
}

TEST(HttpRuntimeContractTests, DefaultSummaryHasExpectedDefaults)
{
    const auto summary =
        dispatcher::http::HttpRuntimeContract::default_summary();

    EXPECT_EQ(summary.schema_version, 1U);
    EXPECT_EQ(summary.status, "available");
    EXPECT_EQ(summary.endpoint, "runtime");
    EXPECT_EQ(summary.path, "/api/v1/runtime");
    EXPECT_EQ(summary.method, "GET");
    EXPECT_EQ(summary.source, "dispatcher-http");
    EXPECT_EQ(summary.service_name, "dispatcher");
    EXPECT_EQ(summary.service_component, "runtime");
    EXPECT_EQ(summary.runtime_state, "running");
    EXPECT_TRUE(summary.started);
    EXPECT_TRUE(summary.configured);
    EXPECT_TRUE(summary.accepting_requests);
    EXPECT_TRUE(summary.telemetry.configured);
    EXPECT_TRUE(summary.alarms.available);
}

TEST(HttpRuntimeContractTests, DefaultJsonContainsBackwardCompatibleFields)
{
    const auto json =
        dispatcher::http::HttpRuntimeContract::default_json();

    expect_contains(
        json,
        "\"schema_version\":1"
    );

    expect_contains(
        json,
        "\"status\":\"available\""
    );

    expect_contains(
        json,
        "\"endpoint\":\"runtime\""
    );

    expect_contains(
        json,
        "\"path\":\"/api/v1/runtime\""
    );

    expect_contains(
        json,
        "\"method\":\"GET\""
    );

    expect_contains(
        json,
        "\"source\":\"dispatcher-http\""
    );
}

TEST(HttpRuntimeContractTests, DefaultJsonContainsStructuredServiceFields)
{
    const auto json =
        dispatcher::http::HttpRuntimeContract::default_json();

    expect_contains(
        json,
        "\"service\":{"
    );

    expect_contains(
        json,
        "\"name\":\"dispatcher\""
    );

    expect_contains(
        json,
        "\"component\":\"runtime\""
    );
}

TEST(HttpRuntimeContractTests, DefaultJsonContainsStructuredRuntimeFields)
{
    const auto json =
        dispatcher::http::HttpRuntimeContract::default_json();

    expect_contains(
        json,
        "\"runtime\":{"
    );

    expect_contains(
        json,
        "\"state\":\"running\""
    );

    expect_contains(
        json,
        "\"started\":true"
    );

    expect_contains(
        json,
        "\"configured\":true"
    );

    expect_contains(
        json,
        "\"accepting_requests\":true"
    );
}

TEST(HttpRuntimeContractTests, DefaultJsonContainsStructuredTelemetryFields)
{
    const auto json =
        dispatcher::http::HttpRuntimeContract::default_json();

    expect_contains(
        json,
        "\"telemetry\":{"
    );

    expect_contains(
        json,
        "\"configured\":true"
    );

    expect_contains(
        json,
        "\"device_count\":0"
    );

    expect_contains(
        json,
        "\"tag_count\":0"
    );

    expect_contains(
        json,
        "\"last_batch_sequence\":null"
    );

    expect_contains(
        json,
        "\"last_ingest_timestamp\":null"
    );
}

TEST(HttpRuntimeContractTests, DefaultJsonContainsStructuredAlarmFields)
{
    const auto json =
        dispatcher::http::HttpRuntimeContract::default_json();

    expect_contains(
        json,
        "\"alarms\":{"
    );

    expect_contains(
        json,
        "\"available\":true"
    );

    expect_contains(
        json,
        "\"active_count\":0"
    );

    expect_contains(
        json,
        "\"unacknowledged_count\":0"
    );

    expect_contains(
        json,
        "\"shelved_count\":0"
    );

    expect_contains(
        json,
        "\"suppressed_count\":0"
    );
}

TEST(HttpRuntimeContractTests, CustomSummaryIsSerialized)
{
    dispatcher::http::HttpRuntimeSummary summary;

    summary.status = "degraded";
    summary.runtime_state = "degraded";
    summary.started = true;
    summary.configured = false;
    summary.accepting_requests = true;

    summary.telemetry.configured = false;
    summary.telemetry.source = "test-source";
    summary.telemetry.device_count = 3;
    summary.telemetry.tag_count = 12;

    summary.alarms.active_count = 2;
    summary.alarms.unacknowledged_count = 1;
    summary.alarms.shelved_count = 4;
    summary.alarms.suppressed_count = 5;

    const auto json =
        dispatcher::http::HttpRuntimeContract::to_json(
            summary
        );

    expect_contains(
        json,
        "\"status\":\"degraded\""
    );

    expect_contains(
        json,
        "\"state\":\"degraded\""
    );

    expect_contains(
        json,
        "\"configured\":false"
    );

    expect_contains(
        json,
        "\"source\":\"test-source\""
    );

    expect_contains(
        json,
        "\"device_count\":3"
    );

    expect_contains(
        json,
        "\"tag_count\":12"
    );

    expect_contains(
        json,
        "\"active_count\":2"
    );

    expect_contains(
        json,
        "\"unacknowledged_count\":1"
    );

    expect_contains(
        json,
        "\"shelved_count\":4"
    );

    expect_contains(
        json,
        "\"suppressed_count\":5"
    );
}