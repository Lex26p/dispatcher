#include <dispatcher/runtime/dispatcher_runtime_batch_summary.hpp>
#include <dispatcher/runtime/dispatcher_runtime_process_summary.hpp>

#include <dispatcher/core/telemetry_ingest_status.hpp>
#include <dispatcher/history/history_write_result.hpp>

#include <gtest/gtest.h>

TEST(DispatcherRuntimeProcessSummaryTests, DefaultSummaryIsRejectedAndUnsuccessful)
{
    const dispatcher::runtime::DispatcherRuntimeProcessSummary summary;

    EXPECT_FALSE(summary.telemetry_accepted());
    EXPECT_FALSE(summary.telemetry_stored());
    EXPECT_FALSE(summary.telemetry_no_change());
    EXPECT_TRUE(summary.telemetry_rejected());

    EXPECT_FALSE(summary.history_written());
    EXPECT_TRUE(summary.history_skipped());

    EXPECT_FALSE(summary.has_configured_alarms());
    EXPECT_FALSE(summary.has_missing_conditions());
    EXPECT_FALSE(summary.alarm_evaluated());
    EXPECT_FALSE(summary.alarm_skipped());
    EXPECT_FALSE(summary.has_alarm_events());
    EXPECT_FALSE(summary.has_alarm_transitions());

    EXPECT_FALSE(summary.successful());
}

TEST(DispatcherRuntimeProcessSummaryTests, AcceptedWrittenActivatedSummaryWorks)
{
    dispatcher::runtime::DispatcherRuntimeProcessSummary summary;

    summary.telemetry_status =
        dispatcher::core::TelemetryIngestStatus::Accepted;

    summary.history_status =
        dispatcher::history::HistoryWriteStatus::Written;

    summary.configured_alarm_count = 1;
    summary.alarm_total_count = 1;
    summary.alarm_evaluated_count = 1;
    summary.alarm_activated_count = 1;
    summary.alarm_stored_event_count = 1;

    EXPECT_TRUE(summary.telemetry_accepted());
    EXPECT_TRUE(summary.telemetry_stored());
    EXPECT_FALSE(summary.telemetry_no_change());
    EXPECT_FALSE(summary.telemetry_rejected());

    EXPECT_TRUE(summary.history_written());
    EXPECT_FALSE(summary.history_skipped());

    EXPECT_TRUE(summary.has_configured_alarms());
    EXPECT_FALSE(summary.has_missing_conditions());

    EXPECT_TRUE(summary.alarm_evaluated());
    EXPECT_FALSE(summary.alarm_skipped());

    EXPECT_TRUE(summary.has_alarm_events());
    EXPECT_TRUE(summary.has_alarm_transitions());

    EXPECT_TRUE(summary.successful());
}

TEST(DispatcherRuntimeProcessSummaryTests, AcceptedNoChangeIsAcceptedButNotStored)
{
    dispatcher::runtime::DispatcherRuntimeProcessSummary summary;

    summary.telemetry_status =
        dispatcher::core::TelemetryIngestStatus::AcceptedNoChange;

    summary.history_status =
        dispatcher::history::HistoryWriteStatus::SkippedNotStored;

    EXPECT_TRUE(summary.telemetry_accepted());
    EXPECT_FALSE(summary.telemetry_stored());
    EXPECT_TRUE(summary.telemetry_no_change());
    EXPECT_FALSE(summary.telemetry_rejected());

    EXPECT_FALSE(summary.history_written());
    EXPECT_TRUE(summary.history_skipped());

    EXPECT_TRUE(summary.successful());
}

TEST(DispatcherRuntimeProcessSummaryTests, MissingConditionsMakeSummaryUnsuccessful)
{
    dispatcher::runtime::DispatcherRuntimeProcessSummary summary;

    summary.telemetry_status =
        dispatcher::core::TelemetryIngestStatus::Accepted;

    summary.history_status =
        dispatcher::history::HistoryWriteStatus::Written;

    summary.configured_alarm_count = 1;
    summary.missing_condition_count = 1;

    EXPECT_TRUE(summary.telemetry_accepted());
    EXPECT_TRUE(summary.has_missing_conditions());
    EXPECT_FALSE(summary.successful());
}

TEST(DispatcherRuntimeBatchSummaryTests, DefaultBatchSummaryIsEmpty)
{
    const dispatcher::runtime::DispatcherRuntimeBatchSummary summary;

    EXPECT_TRUE(summary.empty());
    EXPECT_FALSE(summary.all_telemetry_accepted());
    EXPECT_FALSE(summary.has_telemetry_rejections());
    EXPECT_FALSE(summary.has_history_writes());
    EXPECT_FALSE(summary.has_alarm_evaluations());
    EXPECT_FALSE(summary.has_alarm_events());
    EXPECT_FALSE(summary.has_alarm_transitions());
    EXPECT_FALSE(summary.has_missing_conditions());
    EXPECT_FALSE(summary.successful());
}

TEST(DispatcherRuntimeBatchSummaryTests, RecordAggregatesProcessSummaries)
{
    dispatcher::runtime::DispatcherRuntimeProcessSummary first;

    first.telemetry_status =
        dispatcher::core::TelemetryIngestStatus::Accepted;

    first.history_status =
        dispatcher::history::HistoryWriteStatus::Written;

    first.configured_alarm_count = 1;
    first.alarm_total_count = 1;
    first.alarm_evaluated_count = 1;
    first.alarm_activated_count = 1;
    first.alarm_stored_event_count = 1;

    dispatcher::runtime::DispatcherRuntimeProcessSummary second;

    second.telemetry_status =
        dispatcher::core::TelemetryIngestStatus::AcceptedNoChange;

    second.history_status =
        dispatcher::history::HistoryWriteStatus::SkippedNotStored;

    dispatcher::runtime::DispatcherRuntimeBatchSummary batch;

    batch.record(first);
    batch.record(second);

    EXPECT_FALSE(batch.empty());

    EXPECT_EQ(batch.total_count, 2);

    EXPECT_EQ(batch.telemetry_accepted_count, 2);
    EXPECT_EQ(batch.telemetry_stored_count, 1);
    EXPECT_EQ(batch.telemetry_no_change_count, 1);
    EXPECT_EQ(batch.telemetry_rejected_count, 0);

    EXPECT_EQ(batch.history_written_count, 1);
    EXPECT_EQ(batch.history_skipped_count, 1);

    EXPECT_EQ(batch.configured_alarm_count, 1);
    EXPECT_EQ(batch.missing_condition_count, 0);

    EXPECT_EQ(batch.alarm_total_count, 1);
    EXPECT_EQ(batch.alarm_evaluated_count, 1);
    EXPECT_EQ(batch.alarm_skipped_count, 0);

    EXPECT_EQ(batch.alarm_activated_count, 1);
    EXPECT_EQ(batch.alarm_acknowledged_count, 0);
    EXPECT_EQ(batch.alarm_cleared_count, 0);
    EXPECT_EQ(batch.alarm_stored_event_count, 1);

    EXPECT_TRUE(batch.all_telemetry_accepted());
    EXPECT_FALSE(batch.has_telemetry_rejections());
    EXPECT_TRUE(batch.has_history_writes());
    EXPECT_TRUE(batch.has_alarm_evaluations());
    EXPECT_TRUE(batch.has_alarm_events());
    EXPECT_TRUE(batch.has_alarm_transitions());
    EXPECT_FALSE(batch.has_missing_conditions());
    EXPECT_TRUE(batch.successful());
}

TEST(DispatcherRuntimeBatchSummaryTests, RejectedTelemetryMakesBatchUnsuccessful)
{
    dispatcher::runtime::DispatcherRuntimeProcessSummary accepted;

    accepted.telemetry_status =
        dispatcher::core::TelemetryIngestStatus::Accepted;

    accepted.history_status =
        dispatcher::history::HistoryWriteStatus::Written;

    dispatcher::runtime::DispatcherRuntimeProcessSummary rejected;

    rejected.telemetry_status =
        dispatcher::core::TelemetryIngestStatus::UnknownTag;

    rejected.history_status =
        dispatcher::history::HistoryWriteStatus::SkippedNotStored;

    dispatcher::runtime::DispatcherRuntimeBatchSummary batch;

    batch.record(accepted);
    batch.record(rejected);

    EXPECT_EQ(batch.total_count, 2);
    EXPECT_EQ(batch.telemetry_accepted_count, 1);
    EXPECT_EQ(batch.telemetry_rejected_count, 1);

    EXPECT_FALSE(batch.all_telemetry_accepted());
    EXPECT_TRUE(batch.has_telemetry_rejections());
    EXPECT_FALSE(batch.successful());
}