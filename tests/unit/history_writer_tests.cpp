#include <dispatcher/core/telemetry_ingest_result.hpp>
#include <dispatcher/core/telemetry_ingest_status.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/history/history_write_result.hpp>
#include <dispatcher/history/history_writer.hpp>
#include <dispatcher/history/in_memory_history_store.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>
#include <dispatcher/domain/history_policy.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>

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

TEST(HistoryWriteStatusTests, ToStringReturnsExpectedValues)
{
    using dispatcher::history::HistoryWriteStatus;
    using dispatcher::history::to_string;

    EXPECT_EQ(to_string(HistoryWriteStatus::Written), "written");
    EXPECT_EQ(to_string(HistoryWriteStatus::SkippedNotStored), "skipped_not_stored");
}

TEST(HistoryWriteResultTests, WrittenResultReportsWritten)
{
    const dispatcher::history::HistoryWriteResult result(
        dispatcher::history::HistoryWriteStatus::Written
    );

    EXPECT_TRUE(result.written());
    EXPECT_FALSE(result.skipped());
}

TEST(HistoryWriteResultTests, SkippedResultReportsSkipped)
{
    const dispatcher::history::HistoryWriteResult result(
        dispatcher::history::HistoryWriteStatus::SkippedNotStored
    );

    EXPECT_FALSE(result.written());
    EXPECT_TRUE(result.skipped());
}

TEST(HistoryWriterTests, WritesWhenIngestResultIsStored)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    const dispatcher::core::TelemetryIngestResult ingest_result(
        dispatcher::core::TelemetryIngestStatus::Accepted
    );

    const auto result = writer.write_if_stored(
        ingest_result,
        make_telemetry_value(
            "tag-temperature",
            21.5,
            1
        ),
        dispatcher::domain::HistoryPolicy::OnChange
    );

    EXPECT_TRUE(result.written());
    EXPECT_EQ(result.status(), dispatcher::history::HistoryWriteStatus::Written);
    EXPECT_EQ(store.size(), 1);

    ASSERT_EQ(store.samples().size(), 1);
    EXPECT_EQ(store.samples()[0].tag_id(), dispatcher::domain::TagId{ "tag-temperature" });
    EXPECT_DOUBLE_EQ(store.samples()[0].value().as<double>(), 21.5);
    EXPECT_EQ(store.samples()[0].sequence(), 1);
}

TEST(HistoryWriterTests, SkipsWhenIngestResultIsAcceptedNoChange)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    const dispatcher::core::TelemetryIngestResult ingest_result(
        dispatcher::core::TelemetryIngestStatus::AcceptedNoChange
    );

    const auto result = writer.write_if_stored(
        ingest_result,
        make_telemetry_value(
            "tag-temperature",
            21.5,
            1
        ),
        dispatcher::domain::HistoryPolicy::OnChange
    );

    EXPECT_TRUE(result.skipped());
    EXPECT_EQ(result.status(), dispatcher::history::HistoryWriteStatus::SkippedNotStored);
    EXPECT_EQ(store.size(), 0);
}

TEST(HistoryWriterTests, SkipsWhenIngestResultIsRejected)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    const dispatcher::core::TelemetryIngestResult ingest_result(
        dispatcher::core::TelemetryIngestStatus::UnknownTag
    );

    const auto result = writer.write_if_stored(
        ingest_result,
        make_telemetry_value(
            "unknown-tag",
            21.5,
            1
        ),
        dispatcher::domain::HistoryPolicy::OnChange
    );

    EXPECT_TRUE(result.skipped());
    EXPECT_EQ(result.status(), dispatcher::history::HistoryWriteStatus::SkippedNotStored);
    EXPECT_EQ(store.size(), 0);
}

TEST(HistoryWriterTests, StoreAccessorReturnsUnderlyingStore)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    store.append_telemetry(
        make_telemetry_value(
            "tag-temperature",
            10.0,
            1
        )
    );

    EXPECT_EQ(writer.store().size(), 1);
}

TEST(HistoryStatisticsTests, StartsAtZero)
{
    const dispatcher::history::HistoryStatistics statistics;

    EXPECT_EQ(statistics.written_count(), 0);
    EXPECT_EQ(statistics.skipped_not_stored_count(), 0);
    EXPECT_EQ(statistics.total_count(), 0);
}

TEST(HistoryWriterTests, InitialStatisticsAreZero)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    EXPECT_EQ(writer.statistics().written_count(), 0);
    EXPECT_EQ(writer.statistics().skipped_not_stored_count(), 0);
    EXPECT_EQ(writer.statistics().total_count(), 0);
}

TEST(HistoryWriterTests, WritingStoredValueIncrementsWrittenStatistics)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    const dispatcher::core::TelemetryIngestResult ingest_result(
        dispatcher::core::TelemetryIngestStatus::Accepted
    );

    ASSERT_TRUE(
        writer.write_if_stored(
            ingest_result,
            make_telemetry_value(
                "tag-temperature",
                21.5,
                1
            ),
            dispatcher::domain::HistoryPolicy::OnChange
        ).written()
    );

    EXPECT_EQ(writer.statistics().written_count(), 1);
    EXPECT_EQ(writer.statistics().skipped_not_stored_count(), 0);
    EXPECT_EQ(writer.statistics().total_count(), 1);
}

TEST(HistoryWriterTests, SkippingValueIncrementsSkippedStatistics)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    const dispatcher::core::TelemetryIngestResult ingest_result(
        dispatcher::core::TelemetryIngestStatus::AcceptedNoChange
    );

    ASSERT_TRUE(
        writer.write_if_stored(
            ingest_result,
            make_telemetry_value(
                "tag-temperature",
                21.5,
                1
            ),
            dispatcher::domain::HistoryPolicy::OnChange
        ).skipped()
    );

    EXPECT_EQ(writer.statistics().written_count(), 0);
    EXPECT_EQ(writer.statistics().skipped_not_stored_count(), 1);
    EXPECT_EQ(writer.statistics().total_count(), 1);
}

TEST(HistoryWriterTests, ResetStatisticsClearsCounters)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    const dispatcher::core::TelemetryIngestResult written_result(
        dispatcher::core::TelemetryIngestStatus::Accepted
    );

    const dispatcher::core::TelemetryIngestResult skipped_result(
        dispatcher::core::TelemetryIngestStatus::UnknownTag
    );

    EXPECT_TRUE(
        writer.write_if_stored(
            written_result,
            make_telemetry_value(
                "tag-temperature",
                21.5,
                1
            ),
            dispatcher::domain::HistoryPolicy::OnChange
        ).written()
    );

    EXPECT_TRUE(
        writer.write_if_stored(
            skipped_result,
            make_telemetry_value(
                "unknown-tag",
                21.5,
                2
            ),
            dispatcher::domain::HistoryPolicy::OnChange
        ).skipped()
    );

    ASSERT_EQ(writer.statistics().total_count(), 2);

    writer.reset_statistics();

    EXPECT_EQ(writer.statistics().written_count(), 0);
    EXPECT_EQ(writer.statistics().skipped_not_stored_count(), 0);
    EXPECT_EQ(writer.statistics().total_count(), 0);
}

TEST(HistoryRuntimeSnapshotTests, InitialRuntimeSnapshotReflectsEmptyHistoryRuntime)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    const auto snapshot = writer.runtime_snapshot();

    EXPECT_EQ(snapshot.store_size, 0);
    EXPECT_FALSE(snapshot.max_samples_enabled);
    EXPECT_EQ(snapshot.max_samples, 0);
    EXPECT_EQ(snapshot.retained_sample_count, 0);

    EXPECT_EQ(snapshot.written_count, 0);
    EXPECT_EQ(snapshot.skipped_not_stored_count, 0);
    EXPECT_EQ(snapshot.total_write_count, 0);
}

TEST(HistoryRuntimeSnapshotTests, RuntimeSnapshotReflectsMaxSamplesLimit)
{
    dispatcher::history::InMemoryHistoryStore store(10);
    dispatcher::history::HistoryWriter writer(store);

    const auto snapshot = writer.runtime_snapshot();

    EXPECT_TRUE(snapshot.max_samples_enabled);
    EXPECT_EQ(snapshot.max_samples, 10);
}

TEST(HistoryRuntimeSnapshotTests, RuntimeSnapshotReflectsWrittenAndSkippedValues)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    const dispatcher::core::TelemetryIngestResult written_result(
        dispatcher::core::TelemetryIngestStatus::Accepted
    );

    const dispatcher::core::TelemetryIngestResult skipped_result(
        dispatcher::core::TelemetryIngestStatus::AcceptedNoChange
    );

    ASSERT_TRUE(
        writer.write_if_stored(
            written_result,
            make_telemetry_value(
                "tag-temperature",
                10.0,
                1
            ),
            dispatcher::domain::HistoryPolicy::OnChange
        ).written()
    );

    ASSERT_TRUE(
        writer.write_if_stored(
            skipped_result,
            make_telemetry_value(
                "tag-temperature",
                10.1,
                2
            ),
            dispatcher::domain::HistoryPolicy::OnChange
        ).skipped()
    );

    const auto snapshot = writer.runtime_snapshot();

    EXPECT_EQ(snapshot.store_size, 1);
    EXPECT_EQ(snapshot.written_count, 1);
    EXPECT_EQ(snapshot.skipped_not_stored_count, 1);
    EXPECT_EQ(snapshot.total_write_count, 2);
}

TEST(HistoryRuntimeSnapshotTests, RuntimeSnapshotReflectsRetainedSamples)
{
    dispatcher::history::InMemoryHistoryStore store(1);
    dispatcher::history::HistoryWriter writer(store);

    const dispatcher::core::TelemetryIngestResult written_result(
        dispatcher::core::TelemetryIngestStatus::Accepted
    );

    ASSERT_TRUE(
        writer.write_if_stored(
            written_result,
            make_telemetry_value(
                "tag-temperature",
                10.0,
                1
            ),
            dispatcher::domain::HistoryPolicy::OnChange
        ).written()
    );

    ASSERT_TRUE(
        writer.write_if_stored(
            written_result,
            make_telemetry_value(
                "tag-temperature",
                20.0,
                2
            ),
            dispatcher::domain::HistoryPolicy::OnChange
        ).written()
    );

    const auto snapshot = writer.runtime_snapshot();

    EXPECT_EQ(snapshot.store_size, 1);
    EXPECT_EQ(snapshot.retained_sample_count, 1);
    EXPECT_EQ(snapshot.written_count, 2);
    EXPECT_EQ(snapshot.total_write_count, 2);
}

TEST(HistoryWriteBatchResultTests, EmptyBatchResultReportsEmpty)
{
    const dispatcher::history::HistoryWriteBatchResult result;

    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(result.all_written());
    EXPECT_FALSE(result.has_skipped());
    EXPECT_EQ(result.total_count(), 0);
    EXPECT_EQ(result.written_count(), 0);
    EXPECT_EQ(result.skipped_not_stored_count(), 0);
}

TEST(HistoryWriteBatchResultTests, RecordsWrittenStatus)
{
    dispatcher::history::HistoryWriteBatchResult result;

    result.record(dispatcher::history::HistoryWriteStatus::Written);

    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.all_written());
    EXPECT_FALSE(result.has_skipped());

    EXPECT_EQ(result.total_count(), 1);
    EXPECT_EQ(result.written_count(), 1);
    EXPECT_EQ(result.skipped_not_stored_count(), 0);
}

TEST(HistoryWriteBatchResultTests, RecordsSkippedStatus)
{
    dispatcher::history::HistoryWriteBatchResult result;

    result.record(dispatcher::history::HistoryWriteStatus::SkippedNotStored);

    EXPECT_FALSE(result.empty());
    EXPECT_FALSE(result.all_written());
    EXPECT_TRUE(result.has_skipped());

    EXPECT_EQ(result.total_count(), 1);
    EXPECT_EQ(result.written_count(), 0);
    EXPECT_EQ(result.skipped_not_stored_count(), 1);
}

TEST(HistoryWriterBatchTests, WritesBatchWithStoredValues)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    std::vector<dispatcher::history::HistoryWriteCandidate> candidates;

    candidates.push_back(
        dispatcher::history::HistoryWriteCandidate{
            dispatcher::core::TelemetryIngestResult{
                dispatcher::core::TelemetryIngestStatus::Accepted
            },
            make_telemetry_value(
                "tag-temperature",
                10.0,
                1
            ),
            dispatcher::domain::HistoryPolicy::OnChange
        }
    );

    candidates.push_back(
        dispatcher::history::HistoryWriteCandidate{
            dispatcher::core::TelemetryIngestResult{
                dispatcher::core::TelemetryIngestStatus::Accepted
            },
            make_telemetry_value(
                "tag-pressure",
                20.0,
                2
            ),
            dispatcher::domain::HistoryPolicy::OnChange
        }
    );

    const auto result = writer.write_batch_if_stored(
        std::move(candidates)
    );

    EXPECT_EQ(result.total_count(), 2);
    EXPECT_EQ(result.written_count(), 2);
    EXPECT_EQ(result.skipped_not_stored_count(), 0);
    EXPECT_TRUE(result.all_written());
    EXPECT_FALSE(result.has_skipped());

    EXPECT_EQ(store.size(), 2);
    EXPECT_EQ(writer.statistics().written_count(), 2);
    EXPECT_EQ(writer.statistics().skipped_not_stored_count(), 0);
    EXPECT_EQ(writer.statistics().total_count(), 2);
}

TEST(HistoryWriterBatchTests, SkipsBatchItemsThatWereNotStored)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    std::vector<dispatcher::history::HistoryWriteCandidate> candidates;

    candidates.push_back(
        dispatcher::history::HistoryWriteCandidate{
            dispatcher::core::TelemetryIngestResult{
                dispatcher::core::TelemetryIngestStatus::Accepted
            },
            make_telemetry_value(
                "tag-temperature",
                10.0,
                1
            ),
            dispatcher::domain::HistoryPolicy::OnChange
        }
    );

    candidates.push_back(
        dispatcher::history::HistoryWriteCandidate{
            dispatcher::core::TelemetryIngestResult{
                dispatcher::core::TelemetryIngestStatus::AcceptedNoChange
            },
            make_telemetry_value(
                "tag-temperature",
                10.1,
                2
            ),
            dispatcher::domain::HistoryPolicy::OnChange
        }
    );

    candidates.push_back(
        dispatcher::history::HistoryWriteCandidate{
            dispatcher::core::TelemetryIngestResult{
                dispatcher::core::TelemetryIngestStatus::UnknownTag
            },
            make_telemetry_value(
                "unknown-tag",
                99.0,
                3
            ),
            dispatcher::domain::HistoryPolicy::OnChange
        }
    );

    const auto result = writer.write_batch_if_stored(
        std::move(candidates)
    );

    EXPECT_EQ(result.total_count(), 3);
    EXPECT_EQ(result.written_count(), 1);
    EXPECT_EQ(result.skipped_not_stored_count(), 2);
    EXPECT_FALSE(result.all_written());
    EXPECT_TRUE(result.has_skipped());

    EXPECT_EQ(store.size(), 1);
    ASSERT_EQ(store.samples().size(), 1);
    EXPECT_EQ(store.samples()[0].tag_id(), dispatcher::domain::TagId{ "tag-temperature" });
    EXPECT_DOUBLE_EQ(store.samples()[0].value().as<double>(), 10.0);
    EXPECT_EQ(store.samples()[0].sequence(), 1);

    EXPECT_EQ(writer.statistics().written_count(), 1);
    EXPECT_EQ(writer.statistics().skipped_not_stored_count(), 2);
    EXPECT_EQ(writer.statistics().total_count(), 3);
}

TEST(HistoryWriterBatchTests, EmptyBatchDoesNothing)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    std::vector<dispatcher::history::HistoryWriteCandidate> candidates;

    const auto result = writer.write_batch_if_stored(
        std::move(candidates)
    );

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.total_count(), 0);
    EXPECT_EQ(result.written_count(), 0);
    EXPECT_EQ(result.skipped_not_stored_count(), 0);

    EXPECT_EQ(store.size(), 0);
    EXPECT_EQ(writer.statistics().total_count(), 0);
}

TEST(HistoryWriteStatusTests, ToStringReturnsSkippedByPolicy)
{
    EXPECT_EQ(
        dispatcher::history::to_string(
            dispatcher::history::HistoryWriteStatus::SkippedByPolicy
        ),
        "skipped_by_policy"
    );
}

TEST(HistoryWriteResultTests, SkippedByPolicyReportsSkipped)
{
    const dispatcher::history::HistoryWriteResult result(
        dispatcher::history::HistoryWriteStatus::SkippedByPolicy
    );

    EXPECT_FALSE(result.written());
    EXPECT_TRUE(result.skipped());
    EXPECT_TRUE(result.skipped_by_policy());
}

TEST(HistoryWriterPolicyTests, SkipsDisabledHistoryPolicy)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    const dispatcher::core::TelemetryIngestResult ingest_result(
        dispatcher::core::TelemetryIngestStatus::Accepted
    );

    const auto result = writer.write_if_stored(
        ingest_result,
        make_telemetry_value(
            "tag-temperature",
            10.0,
            1
        ),
        dispatcher::domain::HistoryPolicy::Disabled
    );

    EXPECT_EQ(result.status(), dispatcher::history::HistoryWriteStatus::SkippedByPolicy);
    EXPECT_TRUE(result.skipped_by_policy());

    EXPECT_EQ(store.size(), 0);
    EXPECT_EQ(writer.statistics().written_count(), 0);
    EXPECT_EQ(writer.statistics().skipped_by_policy_count(), 1);
    EXPECT_EQ(writer.statistics().skipped_count(), 1);
    EXPECT_EQ(writer.statistics().total_count(), 1);
}

TEST(HistoryWriterPolicyTests, SkipsLiveOnlyHistoryPolicy)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    const dispatcher::core::TelemetryIngestResult ingest_result(
        dispatcher::core::TelemetryIngestStatus::Accepted
    );

    const auto result = writer.write_if_stored(
        ingest_result,
        make_telemetry_value(
            "tag-temperature",
            10.0,
            1
        ),
        dispatcher::domain::HistoryPolicy::LiveOnly
    );

    EXPECT_EQ(result.status(), dispatcher::history::HistoryWriteStatus::SkippedByPolicy);
    EXPECT_TRUE(result.skipped_by_policy());

    EXPECT_EQ(store.size(), 0);
    EXPECT_EQ(writer.statistics().skipped_by_policy_count(), 1);
}

TEST(HistoryWriterPolicyTests, WritesOnChangeHistoryPolicy)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    const dispatcher::core::TelemetryIngestResult ingest_result(
        dispatcher::core::TelemetryIngestStatus::Accepted
    );

    const auto result = writer.write_if_stored(
        ingest_result,
        make_telemetry_value(
            "tag-temperature",
            10.0,
            1
        ),
        dispatcher::domain::HistoryPolicy::OnChange
    );

    EXPECT_TRUE(result.written());
    EXPECT_EQ(store.size(), 1);
    EXPECT_EQ(writer.statistics().written_count(), 1);
}

TEST(HistoryWriterPolicyTests, SkippedNotStoredTakesPrecedenceOverPolicy)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    const dispatcher::core::TelemetryIngestResult ingest_result(
        dispatcher::core::TelemetryIngestStatus::AcceptedNoChange
    );

    const auto result = writer.write_if_stored(
        ingest_result,
        make_telemetry_value(
            "tag-temperature",
            10.0,
            1
        ),
        dispatcher::domain::HistoryPolicy::Disabled
    );

    EXPECT_EQ(result.status(), dispatcher::history::HistoryWriteStatus::SkippedNotStored);
    EXPECT_EQ(writer.statistics().skipped_not_stored_count(), 1);
    EXPECT_EQ(writer.statistics().skipped_by_policy_count(), 0);
}

TEST(HistoryWriteBatchResultTests, RecordsSkippedByPolicyStatus)
{
    dispatcher::history::HistoryWriteBatchResult result;

    result.record(dispatcher::history::HistoryWriteStatus::SkippedByPolicy);

    EXPECT_FALSE(result.empty());
    EXPECT_FALSE(result.all_written());
    EXPECT_TRUE(result.has_skipped());

    EXPECT_EQ(result.total_count(), 1);
    EXPECT_EQ(result.written_count(), 0);
    EXPECT_EQ(result.skipped_not_stored_count(), 0);
    EXPECT_EQ(result.skipped_by_policy_count(), 1);
    EXPECT_EQ(result.skipped_count(), 1);
}

TEST(HistoryWriterBatchTests, BatchSkipsItemsByPolicy)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    std::vector<dispatcher::history::HistoryWriteCandidate> candidates;

    candidates.push_back(
        dispatcher::history::HistoryWriteCandidate{
            dispatcher::core::TelemetryIngestResult{
                dispatcher::core::TelemetryIngestStatus::Accepted
            },
            make_telemetry_value(
                "tag-temperature",
                10.0,
                1
            ),
            dispatcher::domain::HistoryPolicy::Disabled
        }
    );

    candidates.push_back(
        dispatcher::history::HistoryWriteCandidate{
            dispatcher::core::TelemetryIngestResult{
                dispatcher::core::TelemetryIngestStatus::Accepted
            },
            make_telemetry_value(
                "tag-pressure",
                20.0,
                2
            ),
            dispatcher::domain::HistoryPolicy::OnChange
        }
    );

    const auto result = writer.write_batch_if_stored(
        std::move(candidates)
    );

    EXPECT_EQ(result.total_count(), 2);
    EXPECT_EQ(result.written_count(), 1);
    EXPECT_EQ(result.skipped_by_policy_count(), 1);
    EXPECT_EQ(result.skipped_count(), 1);

    EXPECT_EQ(store.size(), 1);
    ASSERT_EQ(store.samples().size(), 1);
    EXPECT_EQ(store.samples()[0].tag_id(), dispatcher::domain::TagId{ "tag-pressure" });

    EXPECT_EQ(writer.statistics().written_count(), 1);
    EXPECT_EQ(writer.statistics().skipped_by_policy_count(), 1);
    EXPECT_EQ(writer.statistics().total_count(), 2);
}

TEST(HistoryRuntimeSnapshotTests, RuntimeSnapshotReflectsPolicySkips)
{
    dispatcher::history::InMemoryHistoryStore store;
    dispatcher::history::HistoryWriter writer(store);

    const dispatcher::core::TelemetryIngestResult ingest_result(
        dispatcher::core::TelemetryIngestStatus::Accepted
    );

    ASSERT_TRUE(
        writer.write_if_stored(
            ingest_result,
            make_telemetry_value(
                "tag-temperature",
                10.0,
                1
            ),
            dispatcher::domain::HistoryPolicy::Disabled
        ).skipped_by_policy()
    );

    const auto snapshot = writer.runtime_snapshot();

    EXPECT_EQ(snapshot.store_size, 0);
    EXPECT_EQ(snapshot.written_count, 0);
    EXPECT_EQ(snapshot.skipped_not_stored_count, 0);
    EXPECT_EQ(snapshot.skipped_by_policy_count, 1);
    EXPECT_EQ(snapshot.skipped_count, 1);
    EXPECT_EQ(snapshot.total_write_count, 1);
}