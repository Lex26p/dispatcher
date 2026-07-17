#include <dispatcher/runtime/diagnostic_severity.hpp>
#include <dispatcher/runtime/runtime_consistency_issue.hpp>
#include <dispatcher/runtime/runtime_consistency_snapshot.hpp>
#include <dispatcher/runtime/runtime_consistency_status.hpp>

#include <gtest/gtest.h>

#include <string>

TEST(RuntimeConsistencyStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::RuntimeConsistencyStatus::Unknown
        ),
        "unknown"
    );

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::RuntimeConsistencyStatus::Consistent
        ),
        "consistent"
    );

    EXPECT_STREQ(
        dispatcher::runtime::to_string(
            dispatcher::runtime::RuntimeConsistencyStatus::Inconsistent
        ),
        "inconsistent"
    );
}

TEST(RuntimeConsistencyStatusTests, PredicatesWork)
{
    EXPECT_FALSE(
        dispatcher::runtime::is_known(
            dispatcher::runtime::RuntimeConsistencyStatus::Unknown
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_known(
            dispatcher::runtime::RuntimeConsistencyStatus::Consistent
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_consistent(
            dispatcher::runtime::RuntimeConsistencyStatus::Consistent
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::is_inconsistent(
            dispatcher::runtime::RuntimeConsistencyStatus::Inconsistent
        )
    );

    EXPECT_FALSE(
        dispatcher::runtime::requires_action(
            dispatcher::runtime::RuntimeConsistencyStatus::Consistent
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::requires_action(
            dispatcher::runtime::RuntimeConsistencyStatus::Unknown
        )
    );

    EXPECT_TRUE(
        dispatcher::runtime::requires_action(
            dispatcher::runtime::RuntimeConsistencyStatus::Inconsistent
        )
    );
}

TEST(RuntimeConsistencyIssueTests, WarningFactoryCreatesValidNonBlockingIssue)
{
    const auto issue =
        dispatcher::runtime::RuntimeConsistencyIssue::warning(
            "configuration",
            "configuration.unused_tag",
            "configuration contains an unused tag",
            "tag:pressure.backup",
            "tag is referenced",
            "tag is not referenced"
        );

    EXPECT_EQ(
        issue.severity(),
        dispatcher::runtime::DiagnosticSeverity::Warning
    );

    EXPECT_EQ(issue.component(), "configuration");
    EXPECT_EQ(issue.code(), "configuration.unused_tag");
    EXPECT_EQ(issue.message(), "configuration contains an unused tag");
    EXPECT_EQ(issue.subject(), "tag:pressure.backup");
    EXPECT_EQ(issue.expected(), "tag is referenced");
    EXPECT_EQ(issue.actual(), "tag is not referenced");

    EXPECT_TRUE(issue.has_component());
    EXPECT_TRUE(issue.has_code());
    EXPECT_TRUE(issue.has_message());
    EXPECT_TRUE(issue.has_subject());
    EXPECT_TRUE(issue.has_expected());
    EXPECT_TRUE(issue.has_actual());

    EXPECT_TRUE(issue.valid());
    EXPECT_TRUE(issue.warning_or_higher());
    EXPECT_FALSE(issue.error_or_higher());
    EXPECT_FALSE(issue.critical());

    EXPECT_TRUE(issue.requires_attention());

    EXPECT_FALSE(issue.blocking());
    EXPECT_TRUE(issue.non_blocking());
    EXPECT_FALSE(issue.blocks_release());
}

TEST(RuntimeConsistencyIssueTests, ErrorFactoryCreatesBlockingIssue)
{
    const auto issue =
        dispatcher::runtime::RuntimeConsistencyIssue::error(
            "runtime",
            "runtime.missing_configuration",
            "runtime has no active configuration",
            "configuration_snapshot",
            "loaded",
            "missing"
        );

    EXPECT_EQ(
        issue.severity(),
        dispatcher::runtime::DiagnosticSeverity::Error
    );

    EXPECT_TRUE(issue.valid());
    EXPECT_TRUE(issue.error_or_higher());
    EXPECT_FALSE(issue.critical());

    EXPECT_TRUE(issue.blocking());
    EXPECT_FALSE(issue.non_blocking());
    EXPECT_TRUE(issue.blocks_release());
}

TEST(RuntimeConsistencyIssueTests, CriticalFactoryCreatesBlockingIssue)
{
    const auto issue =
        dispatcher::runtime::RuntimeConsistencyIssue::critical(
            "runtime",
            "runtime.invariant_broken",
            "runtime invariant is broken",
            "alarm-runtime",
            "configured",
            "not configured"
        );

    EXPECT_EQ(
        issue.severity(),
        dispatcher::runtime::DiagnosticSeverity::Critical
    );

    EXPECT_TRUE(issue.valid());
    EXPECT_TRUE(issue.error_or_higher());
    EXPECT_TRUE(issue.critical());
    EXPECT_TRUE(issue.blocks_release());
}

TEST(RuntimeConsistencyIssueTests, MetadataLookupWorks)
{
    dispatcher::runtime::RuntimeConsistencyIssue::Metadata metadata{
        {"tag_id", "pressure.main"},
        {"device_id", "plc-1"}
    };

    const auto issue =
        dispatcher::runtime::RuntimeConsistencyIssue::error(
            "telemetry",
            "telemetry.missing_tag",
            "telemetry references missing tag",
            "pressure.main",
            "tag exists",
            "tag missing",
            true,
            metadata
        );

    EXPECT_TRUE(issue.has_metadata());

    EXPECT_TRUE(issue.has_metadata_key("tag_id"));
    EXPECT_TRUE(issue.has_metadata_key("device_id"));

    const auto tag_id = issue.metadata_value("tag_id");

    ASSERT_TRUE(tag_id.has_value());
    EXPECT_EQ(tag_id.value(), "pressure.main");

    EXPECT_FALSE(issue.metadata_value("missing").has_value());
}

TEST(RuntimeConsistencyIssueTests, UnknownOrMissingFieldsAreInvalid)
{
    const dispatcher::runtime::RuntimeConsistencyIssue missing_component(
        dispatcher::runtime::DiagnosticSeverity::Error,
        "",
        "runtime.error",
        "runtime error"
    );

    EXPECT_FALSE(missing_component.valid());

    const dispatcher::runtime::RuntimeConsistencyIssue missing_code(
        dispatcher::runtime::DiagnosticSeverity::Error,
        "runtime",
        "",
        "runtime error"
    );

    EXPECT_FALSE(missing_code.valid());

    const dispatcher::runtime::RuntimeConsistencyIssue missing_message(
        dispatcher::runtime::DiagnosticSeverity::Error,
        "runtime",
        "runtime.error",
        ""
    );

    EXPECT_FALSE(missing_message.valid());

    const dispatcher::runtime::RuntimeConsistencyIssue unknown_severity(
        dispatcher::runtime::DiagnosticSeverity::Unknown,
        "runtime",
        "runtime.unknown",
        "runtime status is unknown"
    );

    EXPECT_FALSE(unknown_severity.valid());
    EXPECT_TRUE(unknown_severity.requires_attention());
    EXPECT_TRUE(unknown_severity.blocks_release());
}

TEST(RuntimeConsistencyIssueTests, InfoSeverityIsInvalidForConsistencyIssue)
{
    const dispatcher::runtime::RuntimeConsistencyIssue info_issue(
        dispatcher::runtime::DiagnosticSeverity::Info,
        "runtime",
        "runtime.note",
        "runtime note"
    );

    EXPECT_FALSE(info_issue.valid());
    EXPECT_FALSE(info_issue.warning_or_higher());
}

TEST(RuntimeConsistencySnapshotTests, EmptySnapshotIsConsistent)
{
    const dispatcher::runtime::RuntimeConsistencySnapshot snapshot;

    EXPECT_TRUE(snapshot.empty());
    EXPECT_FALSE(snapshot.has_issues());

    EXPECT_EQ(snapshot.issue_count(), 0);
    EXPECT_EQ(snapshot.valid_count(), 0);
    EXPECT_EQ(snapshot.invalid_count(), 0);

    EXPECT_EQ(snapshot.warning_count(), 0);
    EXPECT_EQ(snapshot.error_count(), 0);
    EXPECT_EQ(snapshot.critical_count(), 0);
    EXPECT_EQ(snapshot.attention_count(), 0);

    EXPECT_EQ(snapshot.blocking_issue_count(), 0);
    EXPECT_EQ(snapshot.non_blocking_issue_count(), 0);
    EXPECT_EQ(snapshot.component_count(), 0);

    EXPECT_FALSE(snapshot.has_invalid_issues());
    EXPECT_FALSE(snapshot.has_warnings());
    EXPECT_FALSE(snapshot.has_errors());
    EXPECT_FALSE(snapshot.has_critical_issues());
    EXPECT_FALSE(snapshot.has_blocking_issues());

    EXPECT_FALSE(snapshot.requires_attention());
    EXPECT_FALSE(snapshot.blocks_release());

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeConsistencyStatus::Consistent
    );

    EXPECT_TRUE(snapshot.consistent());
    EXPECT_FALSE(snapshot.inconsistent());

    EXPECT_EQ(
        snapshot.highest_severity(),
        dispatcher::runtime::DiagnosticSeverity::Unknown
    );
}

TEST(RuntimeConsistencySnapshotTests, NonBlockingWarningRequiresAttentionButDoesNotBlockRelease)
{
    dispatcher::runtime::RuntimeConsistencySnapshot snapshot;

    snapshot.add_issue(
        dispatcher::runtime::RuntimeConsistencyIssue::warning(
            "configuration",
            "configuration.unused_tag",
            "configuration contains an unused tag"
        )
    );

    EXPECT_EQ(snapshot.issue_count(), 1);
    EXPECT_EQ(snapshot.valid_count(), 1);
    EXPECT_EQ(snapshot.invalid_count(), 0);

    EXPECT_EQ(snapshot.warning_count(), 1);
    EXPECT_EQ(snapshot.error_count(), 0);
    EXPECT_EQ(snapshot.critical_count(), 0);
    EXPECT_EQ(snapshot.attention_count(), 1);

    EXPECT_EQ(snapshot.blocking_issue_count(), 0);
    EXPECT_EQ(snapshot.non_blocking_issue_count(), 1);

    EXPECT_TRUE(snapshot.has_warnings());
    EXPECT_FALSE(snapshot.has_errors());
    EXPECT_FALSE(snapshot.has_blocking_issues());

    EXPECT_TRUE(snapshot.requires_attention());
    EXPECT_FALSE(snapshot.blocks_release());

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeConsistencyStatus::Consistent
    );

    EXPECT_TRUE(snapshot.consistent());

    EXPECT_EQ(
        snapshot.highest_severity(),
        dispatcher::runtime::DiagnosticSeverity::Warning
    );
}

TEST(RuntimeConsistencySnapshotTests, BlockingErrorProducesInconsistentSnapshot)
{
    dispatcher::runtime::RuntimeConsistencySnapshot snapshot;

    snapshot.add_issue(
        dispatcher::runtime::RuntimeConsistencyIssue::error(
            "runtime",
            "runtime.missing_configuration",
            "runtime has no active configuration"
        )
    );

    EXPECT_EQ(snapshot.issue_count(), 1);
    EXPECT_EQ(snapshot.error_count(), 1);
    EXPECT_EQ(snapshot.blocking_issue_count(), 1);

    EXPECT_TRUE(snapshot.has_errors());
    EXPECT_TRUE(snapshot.has_blocking_issues());
    EXPECT_TRUE(snapshot.requires_attention());
    EXPECT_TRUE(snapshot.blocks_release());

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeConsistencyStatus::Inconsistent
    );

    EXPECT_FALSE(snapshot.consistent());
    EXPECT_TRUE(snapshot.inconsistent());

    EXPECT_EQ(
        snapshot.highest_severity(),
        dispatcher::runtime::DiagnosticSeverity::Error
    );
}

TEST(RuntimeConsistencySnapshotTests, CriticalIssueProducesCriticalHighestSeverity)
{
    dispatcher::runtime::RuntimeConsistencySnapshot snapshot;

    snapshot.add_issue(
        dispatcher::runtime::RuntimeConsistencyIssue::warning(
            "configuration",
            "configuration.unused_tag",
            "configuration contains an unused tag"
        )
    );

    snapshot.add_issue(
        dispatcher::runtime::RuntimeConsistencyIssue::critical(
            "runtime",
            "runtime.invariant_broken",
            "runtime invariant is broken"
        )
    );

    EXPECT_EQ(snapshot.issue_count(), 2);
    EXPECT_EQ(snapshot.warning_count(), 1);
    EXPECT_EQ(snapshot.critical_count(), 1);

    EXPECT_TRUE(snapshot.has_critical_issues());
    EXPECT_TRUE(snapshot.blocks_release());

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeConsistencyStatus::Inconsistent
    );

    EXPECT_EQ(
        snapshot.highest_severity(),
        dispatcher::runtime::DiagnosticSeverity::Critical
    );
}

TEST(RuntimeConsistencySnapshotTests, InvalidIssueProducesUnknownStatus)
{
    dispatcher::runtime::RuntimeConsistencySnapshot snapshot;

    snapshot.add_issue(
        dispatcher::runtime::RuntimeConsistencyIssue(
            dispatcher::runtime::DiagnosticSeverity::Unknown,
            "runtime",
            "runtime.unknown",
            "runtime status is unknown"
        )
    );

    EXPECT_EQ(snapshot.issue_count(), 1);
    EXPECT_EQ(snapshot.valid_count(), 0);
    EXPECT_EQ(snapshot.invalid_count(), 1);

    EXPECT_TRUE(snapshot.has_invalid_issues());
    EXPECT_TRUE(snapshot.requires_attention());

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeConsistencyStatus::Unknown
    );

    EXPECT_FALSE(snapshot.consistent());
    EXPECT_FALSE(snapshot.inconsistent());
}

TEST(RuntimeConsistencySnapshotTests, IssuesCanBeFilteredByComponent)
{
    dispatcher::runtime::RuntimeConsistencySnapshot snapshot;

    snapshot.add_issue(
        dispatcher::runtime::RuntimeConsistencyIssue::warning(
            "configuration",
            "configuration.unused_tag",
            "configuration contains an unused tag"
        )
    );

    snapshot.add_issue(
        dispatcher::runtime::RuntimeConsistencyIssue::error(
            "runtime",
            "runtime.missing_configuration",
            "runtime has no active configuration"
        )
    );

    snapshot.add_issue(
        dispatcher::runtime::RuntimeConsistencyIssue::critical(
            "runtime",
            "runtime.invariant_broken",
            "runtime invariant is broken"
        )
    );

    EXPECT_EQ(snapshot.component_count(), 2);

    const auto runtime_issues =
        snapshot.issues_for_component("runtime");

    ASSERT_EQ(runtime_issues.size(), 2);

    EXPECT_EQ(runtime_issues[0].component(), "runtime");
    EXPECT_EQ(runtime_issues[1].component(), "runtime");

    const auto missing_issues =
        snapshot.issues_for_component("missing");

    EXPECT_TRUE(missing_issues.empty());
}

TEST(RuntimeConsistencySnapshotTests, IssuesCanBeFilteredBySeverity)
{
    dispatcher::runtime::RuntimeConsistencySnapshot snapshot;

    snapshot.add_issue(
        dispatcher::runtime::RuntimeConsistencyIssue::warning(
            "configuration",
            "configuration.unused_tag",
            "configuration contains an unused tag"
        )
    );

    snapshot.add_issue(
        dispatcher::runtime::RuntimeConsistencyIssue::warning(
            "configuration",
            "configuration.unused_device",
            "configuration contains an unused device"
        )
    );

    snapshot.add_issue(
        dispatcher::runtime::RuntimeConsistencyIssue::error(
            "runtime",
            "runtime.missing_configuration",
            "runtime has no active configuration"
        )
    );

    const auto warnings =
        snapshot.issues_with_severity(
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
        snapshot.issues_with_severity(
            dispatcher::runtime::DiagnosticSeverity::Critical
        );

    EXPECT_TRUE(critical.empty());
}

TEST(RuntimeConsistencySnapshotTests, ClearRemovesIssues)
{
    dispatcher::runtime::RuntimeConsistencySnapshot snapshot;

    snapshot.add_issue(
        dispatcher::runtime::RuntimeConsistencyIssue::error(
            "runtime",
            "runtime.missing_configuration",
            "runtime has no active configuration"
        )
    );

    ASSERT_EQ(snapshot.issue_count(), 1);

    snapshot.clear();

    EXPECT_TRUE(snapshot.empty());
    EXPECT_EQ(snapshot.issue_count(), 0);

    EXPECT_EQ(
        snapshot.status(),
        dispatcher::runtime::RuntimeConsistencyStatus::Consistent
    );
}