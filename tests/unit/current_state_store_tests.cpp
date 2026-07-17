#include <dispatcher/core/current_state_store.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>

namespace
{
    dispatcher::telemetry::TelemetryValue make_telemetry_value(
        std::string tag_id,
        double value,
        std::uint64_t sequence
    )
    {
        using dispatcher::domain::Quality;
        using dispatcher::domain::TagId;
        using dispatcher::telemetry::TagValue;
        using dispatcher::telemetry::TelemetryValue;

        const auto now = TelemetryValue::Clock::now();

        return TelemetryValue(
            TagId{ std::move(tag_id) },
            TagValue(value),
            Quality::Good,
            now,
            now,
            sequence
        );
    }
}

TEST(CurrentStateStoreTests, StoreIsInitiallyEmpty)
{
    const dispatcher::core::CurrentStateStore store;

    EXPECT_EQ(store.size(), 0);
    EXPECT_FALSE(store.contains(dispatcher::domain::TagId{ "tag-1" }));
}

TEST(CurrentStateStoreTests, StoresNewTelemetryValue)
{
    dispatcher::core::CurrentStateStore store;

    store.update(make_telemetry_value("tag-1", 42.5, 1));

    EXPECT_EQ(store.size(), 1);
    EXPECT_TRUE(store.contains(dispatcher::domain::TagId{ "tag-1" }));

    const auto result = store.find(dispatcher::domain::TagId{ "tag-1" });
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result->tag_id().value(), "tag-1");
    EXPECT_EQ(result->sequence(), 1);
    EXPECT_DOUBLE_EQ(result->value().as<double>(), 42.5);
}

TEST(CurrentStateStoreTests, ReplacesExistingTelemetryValueForSameTag)
{
    dispatcher::core::CurrentStateStore store;

    store.update(make_telemetry_value("tag-1", 10.0, 1));
    store.update(make_telemetry_value("tag-1", 20.0, 2));

    EXPECT_EQ(store.size(), 1);

    const auto result = store.find(dispatcher::domain::TagId{ "tag-1" });
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result->sequence(), 2);
    EXPECT_DOUBLE_EQ(result->value().as<double>(), 20.0);
}

TEST(CurrentStateStoreTests, ReturnsEmptyOptionalForUnknownTag)
{
    dispatcher::core::CurrentStateStore store;

    const auto result = store.find(dispatcher::domain::TagId{ "unknown-tag" });

    EXPECT_FALSE(result.has_value());
}

TEST(CurrentStateStoreTests, ClearRemovesAllValues)
{
    dispatcher::core::CurrentStateStore store;

    store.update(make_telemetry_value("tag-1", 10.0, 1));
    store.update(make_telemetry_value("tag-2", 20.0, 1));

    EXPECT_EQ(store.size(), 2);

    store.clear();

    EXPECT_EQ(store.size(), 0);
    EXPECT_FALSE(store.contains(dispatcher::domain::TagId{ "tag-1" }));
    EXPECT_FALSE(store.contains(dispatcher::domain::TagId{ "tag-2" }));
}