#include <dispatcher/http/http_alarm_contract.hpp>

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

TEST(HttpAlarmContractTests, DefaultSummaryHasExpectedDefaults)
{
    const auto summary =
        dispatcher::http::HttpAlarmContract::default_summary();

    EXPECT_EQ(summary.schema_version, 1U);
    EXPECT_EQ(summary.status, "available");
    EXPECT_EQ(summary.endpoint, "alarms");
    EXPECT_EQ(summary.path, "/api/v1/alarms");
    EXPECT_EQ(summary.method, "GET");
    EXPECT_EQ(summary.source, "dispatcher-http");
    EXPECT_TRUE(summary.available);
    EXPECT_TRUE(summary.items.empty());

    EXPECT_EQ(summary.severity.critical_count, 0U);
    EXPECT_EQ(summary.severity.high_count, 0U);
    EXPECT_EQ(summary.severity.medium_count, 0U);
    EXPECT_EQ(summary.severity.low_count, 0U);
    EXPECT_EQ(summary.severity.info_count, 0U);

    EXPECT_EQ(summary.states.active_count, 0U);
    EXPECT_EQ(summary.states.acknowledged_count, 0U);
    EXPECT_EQ(summary.states.unacknowledged_count, 0U);
    EXPECT_EQ(summary.states.shelved_count, 0U);
    EXPECT_EQ(summary.states.suppressed_count, 0U);
    EXPECT_EQ(summary.states.inhibited_count, 0U);
}

TEST(HttpAlarmContractTests, DefaultJsonContainsBackwardCompatibleFields)
{
    const auto json =
        dispatcher::http::HttpAlarmContract::default_json();

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
        "\"endpoint\":\"alarms\""
    );

    expect_contains(
        json,
        "\"path\":\"/api/v1/alarms\""
    );

    expect_contains(
        json,
        "\"method\":\"GET\""
    );

    expect_contains(
        json,
        "\"source\":\"dispatcher-http\""
    );

    expect_contains(
        json,
        "\"items\":[]"
    );
}

TEST(HttpAlarmContractTests, DefaultJsonContainsSummaryObject)
{
    const auto json =
        dispatcher::http::HttpAlarmContract::default_json();

    expect_contains(
        json,
        "\"summary\":{"
    );

    expect_contains(
        json,
        "\"available\":true"
    );

    expect_contains(
        json,
        "\"total_count\":0"
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

    expect_contains(
        json,
        "\"inhibited_count\":0"
    );
}

TEST(HttpAlarmContractTests, DefaultJsonContainsSeverityObject)
{
    const auto json =
        dispatcher::http::HttpAlarmContract::default_json();

    expect_contains(
        json,
        "\"severity\":{"
    );

    expect_contains(
        json,
        "\"critical_count\":0"
    );

    expect_contains(
        json,
        "\"high_count\":0"
    );

    expect_contains(
        json,
        "\"medium_count\":0"
    );

    expect_contains(
        json,
        "\"low_count\":0"
    );

    expect_contains(
        json,
        "\"info_count\":0"
    );
}

TEST(HttpAlarmContractTests, DefaultJsonContainsStatesObject)
{
    const auto json =
        dispatcher::http::HttpAlarmContract::default_json();

    expect_contains(
        json,
        "\"states\":{"
    );

    expect_contains(
        json,
        "\"active_count\":0"
    );

    expect_contains(
        json,
        "\"acknowledged_count\":0"
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

    expect_contains(
        json,
        "\"inhibited_count\":0"
    );
}

TEST(HttpAlarmContractTests, ItemToJsonContainsAllFields)
{
    dispatcher::http::HttpAlarmItem item;

    item.id = "alarm-001";
    item.name = "High pressure";
    item.tag = "pump.pressure";
    item.severity = "critical";
    item.state = "active";
    item.message = "Pressure is above limit";
    item.source = "alarm-engine";
    item.active = true;
    item.acknowledged = false;
    item.shelved = false;
    item.suppressed = false;
    item.inhibited = false;

    const auto json =
        dispatcher::http::HttpAlarmContract::item_to_json(
            item
        );

    expect_contains(
        json,
        "\"id\":\"alarm-001\""
    );

    expect_contains(
        json,
        "\"name\":\"High pressure\""
    );

    expect_contains(
        json,
        "\"tag\":\"pump.pressure\""
    );

    expect_contains(
        json,
        "\"severity\":\"critical\""
    );

    expect_contains(
        json,
        "\"state\":\"active\""
    );

    expect_contains(
        json,
        "\"message\":\"Pressure is above limit\""
    );

    expect_contains(
        json,
        "\"source\":\"alarm-engine\""
    );

    expect_contains(
        json,
        "\"active\":true"
    );

    expect_contains(
        json,
        "\"acknowledged\":false"
    );

    expect_contains(
        json,
        "\"shelved\":false"
    );

    expect_contains(
        json,
        "\"suppressed\":false"
    );

    expect_contains(
        json,
        "\"inhibited\":false"
    );
}

TEST(HttpAlarmContractTests, CustomSummaryIsSerialized)
{
    dispatcher::http::HttpAlarmSummary summary;

    summary.severity.critical_count = 1;
    summary.severity.high_count = 2;
    summary.severity.medium_count = 3;
    summary.severity.low_count = 4;
    summary.severity.info_count = 5;

    summary.states.active_count = 6;
    summary.states.acknowledged_count = 7;
    summary.states.unacknowledged_count = 8;
    summary.states.shelved_count = 9;
    summary.states.suppressed_count = 10;
    summary.states.inhibited_count = 11;

    dispatcher::http::HttpAlarmItem item;
    item.id = "alarm-001";
    item.name = "High pressure";
    item.tag = "pump.pressure";
    item.severity = "critical";
    item.state = "active";
    item.message = "Pressure is above limit";
    item.active = true;

    summary.items.push_back(
        item
    );

    const auto json =
        dispatcher::http::HttpAlarmContract::to_json(
            summary
        );

    expect_contains(
        json,
        "\"total_count\":1"
    );

    expect_contains(
        json,
        "\"critical_count\":1"
    );

    expect_contains(
        json,
        "\"high_count\":2"
    );

    expect_contains(
        json,
        "\"medium_count\":3"
    );

    expect_contains(
        json,
        "\"low_count\":4"
    );

    expect_contains(
        json,
        "\"info_count\":5"
    );

    expect_contains(
        json,
        "\"active_count\":6"
    );

    expect_contains(
        json,
        "\"acknowledged_count\":7"
    );

    expect_contains(
        json,
        "\"unacknowledged_count\":8"
    );

    expect_contains(
        json,
        "\"shelved_count\":9"
    );

    expect_contains(
        json,
        "\"suppressed_count\":10"
    );

    expect_contains(
        json,
        "\"inhibited_count\":11"
    );

    expect_contains(
        json,
        "\"id\":\"alarm-001\""
    );

    expect_contains(
        json,
        "\"name\":\"High pressure\""
    );
}

TEST(HttpAlarmContractTests, EscapesAlarmItemStrings)
{
    dispatcher::http::HttpAlarmItem item;

    item.id = "alarm-quote";
    item.name = "High \"pressure\"";
    item.tag = "pump\\pressure";
    item.severity = "high";
    item.state = "active";
    item.message = "Line1\nLine2";

    const auto json =
        dispatcher::http::HttpAlarmContract::item_to_json(
            item
        );

    expect_contains(
        json,
        "\"name\":\"High \\\"pressure\\\"\""
    );

    expect_contains(
        json,
        "\"tag\":\"pump\\\\pressure\""
    );

    expect_contains(
        json,
        "\"message\":\"Line1\\nLine2\""
    );
}