#include <dispatcher/domain/quality.hpp>
#include <dispatcher/history/history_sample.hpp>
#include <dispatcher/history/in_memory_history_store.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

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

    dispatcher::telemetry::TelemetryValue make_telemetry_value_at(
        std::string tag_id,
        double value,
        std::uint64_t sequence,
        dispatcher::telemetry::TelemetryValue::TimePoint source_timestamp
    )
    {
        using dispatcher::domain::Quality;
        using dispatcher::domain::TagId;
        using dispatcher::telemetry::TagValue;
        using dispatcher::telemetry::TelemetryValue;

        return TelemetryValue(
            TagId{ std::move(tag_id) },
            TagValue(value),
            Quality::Good,
            source_timestamp,
            source_timestamp,
            sequence
        );
    }
}

TEST(HistorySampleTests, StoresTelemetryValue)
{
    auto telemetry_value = make_telemetry_value(
        "tag-temperature",
        21.5,
        7
    );

    const auto source_timestamp = telemetry_value.source_timestamp();
    const auto ingest_timestamp = telemetry_value.ingest_timestamp();

    const dispatcher::history::HistorySample sample(
        std::move(telemetry_value)
    );

    EXPECT_EQ(sample.tag_id(), dispatcher::domain::TagId{ "tag-temperature" });
    EXPECT_DOUBLE_EQ(sample.value().as<double>(), 21.5);
    EXPECT_EQ(sample.quality(), dispatcher::domain::Quality::Good);
    EXPECT_EQ(sample.source_timestamp(), source_timestamp);
    EXPECT_EQ(sample.ingest_timestamp(), ingest_timestamp);
    EXPECT_EQ(sample.sequence(), 7);
}

TEST(HistorySampleTests, CanCreateFromTelemetryValue)
{
    const auto sample = dispatcher::history::HistorySample::from_telemetry_value(
        make_telemetry_value(
            "tag-pressure",
            12.3,
            3
        )
    );

    EXPECT_EQ(sample.tag_id(), dispatcher::domain::TagId{ "tag-pressure" });
    EXPECT_DOUBLE_EQ(sample.value().as<double>(), 12.3);
    EXPECT_EQ(sample.sequence(), 3);
}

TEST(InMemoryHistoryStoreTests, StartsEmpty)
{
    const dispatcher::history::InMemoryHistoryStore store;

    EXPECT_TRUE(store.empty());
    EXPECT_EQ(store.size(), 0);
    EXPECT_TRUE(store.samples().empty());
}

TEST(InMemoryHistoryStoreTests, AppendsHistorySample)
{
    dispatcher::history::InMemoryHistoryStore store;

    store.append(
        dispatcher::history::HistorySample::from_telemetry_value(
            make_telemetry_value(
                "tag-temperature",
                10.0,
                1
            )
        )
    );

    EXPECT_FALSE(store.empty());
    EXPECT_EQ(store.size(), 1);

    ASSERT_EQ(store.samples().size(), 1);
    EXPECT_EQ(store.samples()[0].tag_id(), dispatcher::domain::TagId{ "tag-temperature" });
    EXPECT_DOUBLE_EQ(store.samples()[0].value().as<double>(), 10.0);
    EXPECT_EQ(store.samples()[0].sequence(), 1);
}

TEST(InMemoryHistoryStoreTests, AppendsTelemetryValue)
{
    dispatcher::history::InMemoryHistoryStore store;

    store.append_telemetry(
        make_telemetry_value(
            "tag-temperature",
            11.0,
            2
        )
    );

    EXPECT_EQ(store.size(), 1);
    EXPECT_EQ(store.samples()[0].tag_id(), dispatcher::domain::TagId{ "tag-temperature" });
    EXPECT_DOUBLE_EQ(store.samples()[0].value().as<double>(), 11.0);
    EXPECT_EQ(store.samples()[0].sequence(), 2);
}

TEST(InMemoryHistoryStoreTests, AppendsBatch)
{
    dispatcher::history::InMemoryHistoryStore store;

    std::vector<dispatcher::telemetry::TelemetryValue> values;

    values.push_back(
        make_telemetry_value(
            "tag-temperature",
            10.0,
            1
        )
    );

    values.push_back(
        make_telemetry_value(
            "tag-pressure",
            20.0,
            2
        )
    );

    store.append_batch(std::move(values));

    EXPECT_EQ(store.size(), 2);

    EXPECT_EQ(store.samples()[0].tag_id(), dispatcher::domain::TagId{ "tag-temperature" });
    EXPECT_EQ(store.samples()[1].tag_id(), dispatcher::domain::TagId{ "tag-pressure" });
}

TEST(InMemoryHistoryStoreTests, FindsSamplesByTagId)
{
    dispatcher::history::InMemoryHistoryStore store;

    store.append_telemetry(
        make_telemetry_value(
            "tag-temperature",
            10.0,
            1
        )
    );

    store.append_telemetry(
        make_telemetry_value(
            "tag-pressure",
            20.0,
            2
        )
    );

    store.append_telemetry(
        make_telemetry_value(
            "tag-temperature",
            11.0,
            3
        )
    );

    const auto samples = store.find_by_tag_id(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_EQ(samples.size(), 2);

    EXPECT_DOUBLE_EQ(samples[0].value().as<double>(), 10.0);
    EXPECT_EQ(samples[0].sequence(), 1);

    EXPECT_DOUBLE_EQ(samples[1].value().as<double>(), 11.0);
    EXPECT_EQ(samples[1].sequence(), 3);
}

TEST(InMemoryHistoryStoreTests, FindByTagIdReturnsEmptyVectorWhenTagIsMissing)
{
    dispatcher::history::InMemoryHistoryStore store;

    store.append_telemetry(
        make_telemetry_value(
            "tag-temperature",
            10.0,
            1
        )
    );

    const auto samples = store.find_by_tag_id(
        dispatcher::domain::TagId{ "missing-tag" }
    );

    EXPECT_TRUE(samples.empty());
}

TEST(InMemoryHistoryStoreTests, ClearRemovesAllSamples)
{
    dispatcher::history::InMemoryHistoryStore store;

    store.append_telemetry(
        make_telemetry_value(
            "tag-temperature",
            10.0,
            1
        )
    );

    ASSERT_EQ(store.size(), 1);

    store.clear();

    EXPECT_TRUE(store.empty());
    EXPECT_EQ(store.size(), 0);
    EXPECT_TRUE(store.samples().empty());
}

TEST(InMemoryHistoryStoreTests, FindsSamplesByTagIdAndSourceTimeRange)
{
    using dispatcher::telemetry::TelemetryValue;

    dispatcher::history::InMemoryHistoryStore store;

    const auto base = TelemetryValue::Clock::now();

    store.append_telemetry(
        make_telemetry_value_at(
            "tag-temperature",
            10.0,
            1,
            base + std::chrono::seconds{ 1 }
        )
    );

    store.append_telemetry(
        make_telemetry_value_at(
            "tag-temperature",
            11.0,
            2,
            base + std::chrono::seconds{ 2 }
        )
    );

    store.append_telemetry(
        make_telemetry_value_at(
            "tag-temperature",
            12.0,
            3,
            base + std::chrono::seconds{ 3 }
        )
    );

    store.append_telemetry(
        make_telemetry_value_at(
            "tag-pressure",
            99.0,
            4,
            base + std::chrono::seconds{ 2 }
        )
    );

    const auto result = store.find_by_tag_id_and_source_time_range(
        dispatcher::domain::TagId{ "tag-temperature" },
        base + std::chrono::seconds{ 2 },
        base + std::chrono::seconds{ 3 }
    );

    ASSERT_EQ(result.size(), 2);

    EXPECT_DOUBLE_EQ(result[0].value().as<double>(), 11.0);
    EXPECT_EQ(result[0].sequence(), 2);

    EXPECT_DOUBLE_EQ(result[1].value().as<double>(), 12.0);
    EXPECT_EQ(result[1].sequence(), 3);
}

TEST(InMemoryHistoryStoreTests, SourceTimeRangeIsInclusive)
{
    using dispatcher::telemetry::TelemetryValue;

    dispatcher::history::InMemoryHistoryStore store;

    const auto base = TelemetryValue::Clock::now();

    store.append_telemetry(
        make_telemetry_value_at(
            "tag-temperature",
            10.0,
            1,
            base
        )
    );

    store.append_telemetry(
        make_telemetry_value_at(
            "tag-temperature",
            20.0,
            2,
            base + std::chrono::seconds{ 10 }
        )
    );

    const auto result = store.find_by_tag_id_and_source_time_range(
        dispatcher::domain::TagId{ "tag-temperature" },
        base,
        base + std::chrono::seconds{ 10 }
    );

    ASSERT_EQ(result.size(), 2);

    EXPECT_DOUBLE_EQ(result[0].value().as<double>(), 10.0);
    EXPECT_DOUBLE_EQ(result[1].value().as<double>(), 20.0);
}

TEST(InMemoryHistoryStoreTests, SourceTimeRangeReturnsEmptyWhenToIsBeforeFrom)
{
    using dispatcher::telemetry::TelemetryValue;

    dispatcher::history::InMemoryHistoryStore store;

    const auto base = TelemetryValue::Clock::now();

    store.append_telemetry(
        make_telemetry_value_at(
            "tag-temperature",
            10.0,
            1,
            base
        )
    );

    const auto result = store.find_by_tag_id_and_source_time_range(
        dispatcher::domain::TagId{ "tag-temperature" },
        base + std::chrono::seconds{ 10 },
        base
    );

    EXPECT_TRUE(result.empty());
}

TEST(InMemoryHistoryStoreTests, SourceTimeRangeReturnsEmptyWhenNoSamplesMatch)
{
    using dispatcher::telemetry::TelemetryValue;

    dispatcher::history::InMemoryHistoryStore store;

    const auto base = TelemetryValue::Clock::now();

    store.append_telemetry(
        make_telemetry_value_at(
            "tag-temperature",
            10.0,
            1,
            base
        )
    );

    const auto result = store.find_by_tag_id_and_source_time_range(
        dispatcher::domain::TagId{ "tag-temperature" },
        base + std::chrono::seconds{ 1 },
        base + std::chrono::seconds{ 2 }
    );

    EXPECT_TRUE(result.empty());
}

TEST(InMemoryHistoryStoreRetentionTests, DefaultStoreHasNoMaxSamplesLimit)
{
    const dispatcher::history::InMemoryHistoryStore store;

    EXPECT_FALSE(store.max_samples().has_value());
}

TEST(InMemoryHistoryStoreRetentionTests, ConstructorSetsMaxSamples)
{
    const dispatcher::history::InMemoryHistoryStore store(3);

    ASSERT_TRUE(store.max_samples().has_value());
    EXPECT_EQ(store.max_samples().value(), 3);
}

TEST(InMemoryHistoryStoreRetentionTests, KeepsOnlyNewestSamplesWhenLimitIsExceeded)
{
    dispatcher::history::InMemoryHistoryStore store(3);

    store.append_telemetry(make_telemetry_value("tag-temperature", 10.0, 1));
    store.append_telemetry(make_telemetry_value("tag-temperature", 20.0, 2));
    store.append_telemetry(make_telemetry_value("tag-temperature", 30.0, 3));
    store.append_telemetry(make_telemetry_value("tag-temperature", 40.0, 4));

    ASSERT_EQ(store.size(), 3);

    EXPECT_EQ(store.samples()[0].sequence(), 2);
    EXPECT_EQ(store.samples()[1].sequence(), 3);
    EXPECT_EQ(store.samples()[2].sequence(), 4);
}

TEST(InMemoryHistoryStoreRetentionTests, SetMaxSamplesAppliesRetentionImmediately)
{
    dispatcher::history::InMemoryHistoryStore store;

    store.append_telemetry(make_telemetry_value("tag-temperature", 10.0, 1));
    store.append_telemetry(make_telemetry_value("tag-temperature", 20.0, 2));
    store.append_telemetry(make_telemetry_value("tag-temperature", 30.0, 3));
    store.append_telemetry(make_telemetry_value("tag-temperature", 40.0, 4));

    ASSERT_EQ(store.size(), 4);

    store.set_max_samples(2);

    ASSERT_EQ(store.size(), 2);

    EXPECT_EQ(store.samples()[0].sequence(), 3);
    EXPECT_EQ(store.samples()[1].sequence(), 4);
}

TEST(InMemoryHistoryStoreRetentionTests, MaxSamplesZeroKeepsNoSamples)
{
    dispatcher::history::InMemoryHistoryStore store(0);

    store.append_telemetry(make_telemetry_value("tag-temperature", 10.0, 1));

    EXPECT_TRUE(store.empty());
    EXPECT_EQ(store.size(), 0);
}

TEST(InMemoryHistoryStoreRetentionTests, ClearingMaxSamplesDisablesRetention)
{
    dispatcher::history::InMemoryHistoryStore store(1);

    store.append_telemetry(make_telemetry_value("tag-temperature", 10.0, 1));
    store.append_telemetry(make_telemetry_value("tag-temperature", 20.0, 2));

    ASSERT_EQ(store.size(), 1);
    EXPECT_EQ(store.samples()[0].sequence(), 2);

    store.set_max_samples(std::nullopt);

    store.append_telemetry(make_telemetry_value("tag-temperature", 30.0, 3));
    store.append_telemetry(make_telemetry_value("tag-temperature", 40.0, 4));

    ASSERT_EQ(store.size(), 3);

    EXPECT_EQ(store.samples()[0].sequence(), 2);
    EXPECT_EQ(store.samples()[1].sequence(), 3);
    EXPECT_EQ(store.samples()[2].sequence(), 4);
}

TEST(InMemoryHistoryStoreRetentionStatisticsTests, RetainedSampleCountStartsAtZero)
{
    const dispatcher::history::InMemoryHistoryStore store(1);

    EXPECT_EQ(store.retained_sample_count(), 0);
}

TEST(InMemoryHistoryStoreRetentionStatisticsTests, RetainedSampleCountIncrementsWhenRetentionRemovesSamples)
{
    dispatcher::history::InMemoryHistoryStore store(2);

    store.append_telemetry(make_telemetry_value("tag-temperature", 10.0, 1));
    store.append_telemetry(make_telemetry_value("tag-temperature", 20.0, 2));
    store.append_telemetry(make_telemetry_value("tag-temperature", 30.0, 3));

    EXPECT_EQ(store.size(), 2);
    EXPECT_EQ(store.retained_sample_count(), 1);

    store.append_telemetry(make_telemetry_value("tag-temperature", 40.0, 4));

    EXPECT_EQ(store.size(), 2);
    EXPECT_EQ(store.retained_sample_count(), 2);
}

TEST(InMemoryHistoryStoreRetentionStatisticsTests, SetMaxSamplesRecordsRetainedSamples)
{
    dispatcher::history::InMemoryHistoryStore store;

    store.append_telemetry(make_telemetry_value("tag-temperature", 10.0, 1));
    store.append_telemetry(make_telemetry_value("tag-temperature", 20.0, 2));
    store.append_telemetry(make_telemetry_value("tag-temperature", 30.0, 3));
    store.append_telemetry(make_telemetry_value("tag-temperature", 40.0, 4));

    ASSERT_EQ(store.retained_sample_count(), 0);

    store.set_max_samples(2);

    EXPECT_EQ(store.size(), 2);
    EXPECT_EQ(store.retained_sample_count(), 2);
}

TEST(InMemoryHistoryStoreRetentionStatisticsTests, ResetRetainedSampleCountClearsCounter)
{
    dispatcher::history::InMemoryHistoryStore store(1);

    store.append_telemetry(make_telemetry_value("tag-temperature", 10.0, 1));
    store.append_telemetry(make_telemetry_value("tag-temperature", 20.0, 2));

    ASSERT_EQ(store.retained_sample_count(), 1);

    store.reset_retained_sample_count();

    EXPECT_EQ(store.retained_sample_count(), 0);
}