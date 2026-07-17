#include <dispatcher/runtime/diagnostic_record.hpp>
#include <dispatcher/runtime/diagnostic_severity.hpp>
#include <dispatcher/runtime/runtime_diagnostics_snapshot.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <string>

TEST(DiagnosticSeverityTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::DiagnosticSeverity::Unknown
        ),
        "unknown"
    );

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::DiagnosticSeverity::Trace
        ),
        "trace"
    );

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::DiagnosticSeverity::Debug
        ),
        "debug"
    );

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::DiagnosticSeverity::Info
        ),
        "info"
    );

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::DiagnosticSeverity::Warning
        ),
        "warning"
    );

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::DiagnosticSeverity::Error
        ),
        "error"
    );

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::DiagnosticSeverity::Critical
        ),
        "critical"
    );
}

TEST(DiagnosticSeverityTests, SeverityRanksAreOrdered)
{
    EXPECT_LT(
        dispatcher::runtime::severity_rank(
            dispatcher::runtime::DiagnosticSeverity::Trace
        ),
        dispatcher::runtime::severity_rank(
            dispatcher::runtime::DiagnosticSeverity::Debug
        )
    );

    EXPECT_LT(
        dispatcher::runtime::severity_rank(
            dispatcher::runtime::DiagnosticSeverity::Info
        ),
        dispatcher::runtime::severity_rank(
            dispatcher::runtime::DiagnosticSeverity::Warning
        )
    );

    EXPECT_LT(
        dispatcher::runtime::severity_rank(
            dispatcher::runtime::DiagnosticSeverity::Error
        ),
        dispatcher::runtime::severity_rank(
            dispatcher::runtime::DiagnosticSeverity::Critical
        )
    );
}

TEST(DiagnosticSeverityTests, PredicatesWork)
{
    EXPECT_FALSE(
        dispatcher::runtime::is_known(
            dispatcher::runtime::DiagnosticSeverity::Unknown
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_known(
            dispatcher::runtime::DiagnosticSeverity::Info
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_operational_note(
            dispatcher::runtime::DiagnosticSeverity::Trace
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_operational_note(
            dispatcher::runtime::DiagnosticSeverity::Debug
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_operational_note(
            dispatcher::runtime::DiagnosticSeverity::Info
        )
    );

    EXPECT_FALSE(
        dispatcher::runtime::is_operational_note(
            dispatcher::runtime::DiagnosticSeverity::Warning
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_warning_or_higher(
            dispatcher::runtime::DiagnosticSeverity::Warning
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_error_or_higher(
            dispatcher::runtime::DiagnosticSeverity::Error
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_critical(
            dispatcher::runtime::DiagnosticSeverity::Critical
        )
    );

    EXPECT_FALSE(
        dispatcher::runtime::requires_attention(
            dispatcher::runtime::DiagnosticSeverity::Info
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::requires_attention(
            dispatcher::runtime::DiagnosticSeverity::Warning
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::requires_attention(
            dispatcher::runtime::DiagnosticSeverity::Unknown
        )
    );
}

TEST(DiagnosticRecordTests, InfoFactoryCreatesValidRecord)
{
    const auto record =
        dispatcher::runtime::DiagnosticRecord::info(
            "runtime",
            "runtime.started",
            "dispatcher runtime started"
        );

    EXPECT_EQ(
        record.severity(),
        dispatcher::runtime::DiagnosticSeverity::Info
    );

    EXPECT_EQ(record.component(), "runtime");
    EXPECT_EQ(record.code(), "runtime.started");
    EXPECT_EQ(record.message(), "dispatcher runtime started");

    EXPECT_TRUE(record.has_component());
    EXPECT_TRUE(record.has_code());
    EXPECT_TRUE(record.has_message());

    EXPECT_FALSE(record.has_metadata());

    EXPECT_TRUE(record.valid());
    EXPECT_TRUE(record.operational_note());
    EXPECT_FALSE(record.warning_or_higher());
    EXPECT_FALSE(record.error_or_higher());
    EXPECT_FALSE(record.critical());
    EXPECT_FALSE(record.requires_attention());
}

TEST(DiagnosticRecordTests, WarningFactoryCreatesAttentionRecord)
{
    const auto record =
        dispatcher::runtime::DiagnosticRecord::warning(
            "history",
            "history.writer_lag",
            "history writer is behind"
        );

    EXPECT_TRUE(record.valid());

    EXPECT_EQ(
        record.severity(),
        dispatcher::runtime::DiagnosticSeverity::Warning
    );

    EXPECT_FALSE(record.operational_note());
    EXPECT_TRUE(record.warning_or_higher());
    EXPECT_FALSE(record.error_or_higher());
    EXPECT_FALSE(record.critical());
    EXPECT_TRUE(record.requires_attention());
}

TEST(DiagnosticRecordTests, ErrorAndCriticalPredicatesWork)
{
    const auto error =
        dispatcher::runtime::DiagnosticRecord::error(
            "storage",
            "storage.write_failed",
            "storage write failed"
        );

    EXPECT_TRUE(error.valid());
    EXPECT_TRUE(error.warning_or_higher());
    EXPECT_TRUE(error.error_or_higher());
    EXPECT_FALSE(error.critical());
    EXPECT_TRUE(error.requires_attention());

    const auto critical =
        dispatcher::runtime::DiagnosticRecord::critical(
            "runtime",
            "runtime.invariant_broken",
            "runtime invariant is broken"
        );

    EXPECT_TRUE(critical.valid());
    EXPECT_TRUE(critical.warning_or_higher());
    EXPECT_TRUE(critical.error_or_higher());
    EXPECT_TRUE(critical.critical());
    EXPECT_TRUE(critical.requires_attention());
}

TEST(DiagnosticRecordTests, MetadataLookupWorks)
{
    dispatcher::runtime::DiagnosticRecord::Metadata metadata{
        {"tag_id", "pressure.main"},
        {"device_id", "plc-1"}
    };

    const auto record =
        dispatcher::runtime::DiagnosticRecord::warning(
            "telemetry",
            "telemetry.stale_value",
            "telemetry value is stale",
            metadata
        );

    EXPECT_TRUE(record.has_metadata());

    EXPECT_TRUE(record.has_metadata_key("tag_id"));
    EXPECT_TRUE(record.has_metadata_key("device_id"));

    const auto tag_id = record.metadata_value("tag_id");

    ASSERT_TRUE(tag_id.has_value());
    EXPECT_EQ(tag_id.value(), "pressure.main");

    EXPECT_FALSE(record.metadata_value("missing").has_value());
}

TEST(DiagnosticRecordTests, MissingFieldsOrUnknownSeverityAreInvalid)
{
    const dispatcher::runtime::DiagnosticRecord missing_component(
        dispatcher::runtime::DiagnosticSeverity::Info,
        "",
        "runtime.started",
        "runtime started"
    );

    EXPECT_FALSE(missing_component.valid());

    const dispatcher::runtime::DiagnosticRecord missing_code(
        dispatcher::runtime::DiagnosticSeverity::Info,
        "runtime",
        "",
        "runtime started"
    );

    EXPECT_FALSE(missing_code.valid());

    const dispatcher::runtime::DiagnosticRecord missing_message(
        dispatcher::runtime::DiagnosticSeverity::Info,
        "runtime",
        "runtime.started",
        ""
    );

    EXPECT_FALSE(missing_message.valid());

    const dispatcher::runtime::DiagnosticRecord unknown_severity(
        dispatcher::runtime::DiagnosticSeverity::Unknown,
        "runtime",
        "runtime.unknown",
        "runtime status is unknown"
    );

    EXPECT_FALSE(unknown_severity.valid());
    EXPECT_TRUE(unknown_severity.requires_attention());
}

TEST(RuntimeDiagnosticsSnapshotTests, EmptySnapshotHasUnknownHighestSeverity)
{
    const dispatcher::runtime::RuntimeDiagnosticsSnapshot snapshot;

    EXPECT_TRUE(snapshot.empty());
    EXPECT_FALSE(snapshot.has_records());

    EXPECT_EQ(snapshot.record_count(), 0);
    EXPECT_EQ(snapshot.valid_count(), 0);
    EXPECT_EQ(snapshot.invalid_count(), 0);

    EXPECT_EQ(snapshot.trace_count(), 0);
    EXPECT_EQ(snapshot.debug_count(), 0);
    EXPECT_EQ(snapshot.info_count(), 0);
    EXPECT_EQ(snapshot.warning_count(), 0);
    EXPECT_EQ(snapshot.error_count(), 0);
    EXPECT_EQ(snapshot.critical_count(), 0);
    EXPECT_EQ(snapshot.attention_count(), 0);
    EXPECT_EQ(snapshot.component_count(), 0);

    EXPECT_FALSE(snapshot.has_invalid_records());
    EXPECT_FALSE(snapshot.has_warnings());
    EXPECT_FALSE(snapshot.has_errors());
    EXPECT_FALSE(snapshot.has_critical_records());
    EXPECT_FALSE(snapshot.requires_attention());

    EXPECT_EQ(
        snapshot.highest_severity(),
        dispatcher::runtime::DiagnosticSeverity::Unknown
    );
}

TEST(RuntimeDiagnosticsSnapshotTests, CountsSeverityAndHighestSeverity)
{
    dispatcher::runtime::RuntimeDiagnosticsSnapshot snapshot;

    snapshot.add_record(
        dispatcher::runtime::DiagnosticRecord::info(
            "runtime",
            "runtime.started",
            "runtime started"
        )
    );

    snapshot.add_record(
        dispatcher::runtime::DiagnosticRecord::warning(
            "history",
            "history.writer_lag",
            "history writer is behind"
        )
    );

    snapshot.add_record(
        dispatcher::runtime::DiagnosticRecord::error(
            "storage",
            "storage.write_failed",
            "storage write failed"
        )
    );

    snapshot.add_record(
        dispatcher::runtime::DiagnosticRecord::critical(
            "runtime",
            "runtime.invariant_broken",
            "runtime invariant is broken"
        )
    );

    EXPECT_FALSE(snapshot.empty());
    EXPECT_TRUE(snapshot.has_records());

    EXPECT_EQ(snapshot.record_count(), 4);
    EXPECT_EQ(snapshot.valid_count(), 4);
    EXPECT_EQ(snapshot.invalid_count(), 0);

    EXPECT_EQ(snapshot.info_count(), 1);
    EXPECT_EQ(snapshot.warning_count(), 1);
    EXPECT_EQ(snapshot.error_count(), 1);
    EXPECT_EQ(snapshot.critical_count(), 1);

    EXPECT_EQ(snapshot.attention_count(), 3);
    EXPECT_EQ(snapshot.component_count(), 3);

    EXPECT_TRUE(snapshot.has_warnings());
    EXPECT_TRUE(snapshot.has_errors());
    EXPECT_TRUE(snapshot.has_critical_records());
    EXPECT_TRUE(snapshot.requires_attention());

    EXPECT_EQ(
        snapshot.highest_severity(),
        dispatcher::runtime::DiagnosticSeverity::Critical
    );
}

TEST(RuntimeDiagnosticsSnapshotTests, InvalidRecordsAreCounted)
{
    dispatcher::runtime::RuntimeDiagnosticsSnapshot snapshot;

    snapshot.add_record(
        dispatcher::runtime::DiagnosticRecord(
            dispatcher::runtime::DiagnosticSeverity::Unknown,
            "runtime",
            "runtime.unknown",
            "runtime unknown"
        )
    );

    snapshot.add_record(
        dispatcher::runtime::DiagnosticRecord::info(
            "runtime",
            "runtime.started",
            "runtime started"
        )
    );

    EXPECT_EQ(snapshot.record_count(), 2);
    EXPECT_EQ(snapshot.valid_count(), 1);
    EXPECT_EQ(snapshot.invalid_count(), 1);

    EXPECT_TRUE(snapshot.has_invalid_records());
    EXPECT_TRUE(snapshot.requires_attention());

    EXPECT_EQ(
        snapshot.highest_severity(),
        dispatcher::runtime::DiagnosticSeverity::Info
    );
}

TEST(RuntimeDiagnosticsSnapshotTests, RecordsCanBeFilteredByComponent)
{
    dispatcher::runtime::RuntimeDiagnosticsSnapshot snapshot;

    snapshot.add_record(
        dispatcher::runtime::DiagnosticRecord::info(
            "runtime",
            "runtime.started",
            "runtime started"
        )
    );

    snapshot.add_record(
        dispatcher::runtime::DiagnosticRecord::warning(
            "runtime",
            "runtime.slow_tick",
            "runtime tick is slow"
        )
    );

    snapshot.add_record(
        dispatcher::runtime::DiagnosticRecord::error(
            "storage",
            "storage.write_failed",
            "storage write failed"
        )
    );

    const auto runtime_records =
        snapshot.records_for_component("runtime");

    ASSERT_EQ(runtime_records.size(), 2);

    EXPECT_EQ(runtime_records[0].component(), "runtime");
    EXPECT_EQ(runtime_records[1].component(), "runtime");

    const auto missing_records =
        snapshot.records_for_component("missing");

    EXPECT_TRUE(missing_records.empty());
}

TEST(RuntimeDiagnosticsSnapshotTests, RecordsCanBeFilteredBySeverity)
{
    dispatcher::runtime::RuntimeDiagnosticsSnapshot snapshot;

    snapshot.add_record(
        dispatcher::runtime::DiagnosticRecord::warning(
            "runtime",
            "runtime.slow_tick",
            "runtime tick is slow"
        )
    );

    snapshot.add_record(
        dispatcher::runtime::DiagnosticRecord::warning(
            "history",
            "history.writer_lag",
            "history writer is behind"
        )
    );

    snapshot.add_record(
        dispatcher::runtime::DiagnosticRecord::error(
            "storage",
            "storage.write_failed",
            "storage write failed"
        )
    );

    const auto warnings =
        snapshot.records_with_severity(
            dispatcher::runtime::DiagnosticSeverity::Warning
        );

    ASSERT_EQ(warnings.size(), 2);

    EXPECT_EQ(
        warnings[0].severity(),
        dispatcher::runtime::DiagnosticSeverity::Warning
    );

    EXPECT_EQ(
        warnings[1].severity(),
        dispatcher::runtime::DiagnosticSeverity::Warning
    );

    const auto critical =
        snapshot.records_with_severity(
            dispatcher::runtime::DiagnosticSeverity::Critical
        );

    EXPECT_TRUE(critical.empty());
}

TEST(RuntimeDiagnosticsSnapshotTests, TraceAndDebugCountsWork)
{
    dispatcher::runtime::RuntimeDiagnosticsSnapshot snapshot;

    snapshot.add_record(
        dispatcher::runtime::DiagnosticRecord::trace(
            "runtime",
            "runtime.tick",
            "runtime tick"
        )
    );

    snapshot.add_record(
        dispatcher::runtime::DiagnosticRecord::debug(
            "telemetry",
            "telemetry.batch",
            "telemetry batch processed"
        )
    );

    EXPECT_EQ(snapshot.record_count(), 2);
    EXPECT_EQ(snapshot.trace_count(), 1);
    EXPECT_EQ(snapshot.debug_count(), 1);
    EXPECT_EQ(snapshot.attention_count(), 0);

    EXPECT_FALSE(snapshot.requires_attention());

    EXPECT_EQ(
        snapshot.highest_severity(),
        dispatcher::runtime::DiagnosticSeverity::Debug
    );
}

TEST(RuntimeDiagnosticsSnapshotTests, ClearRemovesRecords)
{
    dispatcher::runtime::RuntimeDiagnosticsSnapshot snapshot;

    snapshot.add_record(
        dispatcher::runtime::DiagnosticRecord::info(
            "runtime",
            "runtime.started",
            "runtime started"
        )
    );

    ASSERT_EQ(snapshot.record_count(), 1);

    snapshot.clear();

    EXPECT_TRUE(snapshot.empty());
    EXPECT_EQ(snapshot.record_count(), 0);
    EXPECT_EQ(snapshot.component_count(), 0);
}