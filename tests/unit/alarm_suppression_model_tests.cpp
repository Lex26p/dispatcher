#include <dispatcher/alarm/alarm_suppression_command.hpp>
#include <dispatcher/alarm/alarm_suppression_mode.hpp>
#include <dispatcher/alarm/alarm_suppression_reason.hpp>
#include <dispatcher/alarm/alarm_suppression_record.hpp>
#include <dispatcher/alarm/alarm_suppression_result.hpp>
#include <dispatcher/alarm/alarm_suppression_status.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

TEST(AlarmSuppressionModeTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionMode::Shelved
        ),
        "shelved"
    );

    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionMode::Suppressed
        ),
        "suppressed"
    );

    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionMode::Inhibited
        ),
        "inhibited"
    );
}

TEST(AlarmSuppressionModeTests, ModePredicatesClassifyOwnership)
{
    EXPECT_TRUE(
        dispatcher::alarm::is_operator_controlled(
            dispatcher::alarm::AlarmSuppressionMode::Shelved
        )
    );

    EXPECT_FALSE(
        dispatcher::alarm::is_system_controlled(
            dispatcher::alarm::AlarmSuppressionMode::Shelved
        )
    );

    EXPECT_TRUE(
        dispatcher::alarm::is_system_controlled(
            dispatcher::alarm::AlarmSuppressionMode::Suppressed
        )
    );

    EXPECT_TRUE(
        dispatcher::alarm::is_system_controlled(
            dispatcher::alarm::AlarmSuppressionMode::Inhibited
        )
    );
}

TEST(AlarmSuppressionReasonTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionReason::Maintenance
        ),
        "maintenance"
    );

    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionReason::Nuisance
        ),
        "nuisance"
    );

    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionReason::Testing
        ),
        "testing"
    );

    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionReason::Commissioning
        ),
        "commissioning"
    );

    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionReason::OperatorDecision
        ),
        "operator_decision"
    );

    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionReason::ExternalInterlock
        ),
        "external_interlock"
    );

    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionReason::Unknown
        ),
        "unknown"
    );
}

TEST(AlarmSuppressionReasonTests, KnownReasonPredicateRejectsUnknown)
{
    EXPECT_TRUE(
        dispatcher::alarm::is_known_reason(
            dispatcher::alarm::AlarmSuppressionReason::Maintenance
        )
    );

    EXPECT_FALSE(
        dispatcher::alarm::is_known_reason(
            dispatcher::alarm::AlarmSuppressionReason::Unknown
        )
    );
}

TEST(AlarmSuppressionStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionStatus::Applied
        ),
        "applied"
    );

    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionStatus::Cleared
        ),
        "cleared"
    );

    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionStatus::UnknownAlarm
        ),
        "unknown_alarm"
    );

    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionStatus::AlreadySuppressed
        ),
        "already_suppressed"
    );

    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionStatus::NotSuppressed
        ),
        "not_suppressed"
    );

    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionStatus::Expired
        ),
        "expired"
    );

    EXPECT_STREQ(
        dispatcher::alarm::to_string(
            dispatcher::alarm::AlarmSuppressionStatus::InvalidCommand
        ),
        "invalid_command"
    );
}

TEST(AlarmSuppressionStatusTests, PredicatesClassifySuccessAndFailure)
{
    EXPECT_TRUE(
        dispatcher::alarm::is_success(
            dispatcher::alarm::AlarmSuppressionStatus::Applied
        )
    );

    EXPECT_TRUE(
        dispatcher::alarm::is_success(
            dispatcher::alarm::AlarmSuppressionStatus::Cleared
        )
    );

    EXPECT_FALSE(
        dispatcher::alarm::is_failure(
            dispatcher::alarm::AlarmSuppressionStatus::Applied
        )
    );

    EXPECT_TRUE(
        dispatcher::alarm::is_failure(
            dispatcher::alarm::AlarmSuppressionStatus::InvalidCommand
        )
    );
}

TEST(AlarmSuppressionCommandTests, ValidCommandCapturesFields)
{
    using Command = dispatcher::alarm::AlarmSuppressionCommand;

    const auto now = Command::Clock::now();
    const auto expires_at = now + std::chrono::minutes(30);

    const Command command(
        dispatcher::domain::AlarmId{ "alarm-1" },
        "operator-1",
        dispatcher::alarm::AlarmSuppressionMode::Shelved,
        dispatcher::alarm::AlarmSuppressionReason::Maintenance,
        "planned maintenance",
        expires_at
    );

    EXPECT_EQ(command.alarm_id(), dispatcher::domain::AlarmId{ "alarm-1" });
    EXPECT_EQ(command.operator_id(), "operator-1");

    EXPECT_EQ(
        command.mode(),
        dispatcher::alarm::AlarmSuppressionMode::Shelved
    );

    EXPECT_EQ(
        command.reason(),
        dispatcher::alarm::AlarmSuppressionReason::Maintenance
    );

    EXPECT_EQ(command.comment(), "planned maintenance");

    EXPECT_TRUE(command.has_alarm_id());
    EXPECT_TRUE(command.has_operator_id());
    EXPECT_TRUE(command.has_comment());
    EXPECT_TRUE(command.has_expiration());

    EXPECT_FALSE(command.expired_at(now));
    EXPECT_TRUE(command.valid());
}

TEST(AlarmSuppressionCommandTests, InvalidCommandRejectsMissingIdentity)
{
    const dispatcher::alarm::AlarmSuppressionCommand command(
        dispatcher::domain::AlarmId{ "" },
        "",
        dispatcher::alarm::AlarmSuppressionMode::Shelved,
        dispatcher::alarm::AlarmSuppressionReason::Maintenance
    );

    EXPECT_FALSE(command.has_alarm_id());
    EXPECT_FALSE(command.has_operator_id());
    EXPECT_FALSE(command.valid());
}

TEST(AlarmSuppressionCommandTests, InvalidCommandRejectsUnknownReason)
{
    const dispatcher::alarm::AlarmSuppressionCommand command(
        dispatcher::domain::AlarmId{ "alarm-1" },
        "operator-1",
        dispatcher::alarm::AlarmSuppressionMode::Shelved,
        dispatcher::alarm::AlarmSuppressionReason::Unknown
    );

    EXPECT_FALSE(command.valid());
}

TEST(AlarmSuppressionCommandTests, ExpiredCommandIsInvalid)
{
    using Command = dispatcher::alarm::AlarmSuppressionCommand;

    const auto now = Command::Clock::now();

    const Command command(
        dispatcher::domain::AlarmId{ "alarm-1" },
        "operator-1",
        dispatcher::alarm::AlarmSuppressionMode::Shelved,
        dispatcher::alarm::AlarmSuppressionReason::Maintenance,
        {},
        now - std::chrono::seconds(1)
    );

    EXPECT_TRUE(command.expired_at(now));
    EXPECT_FALSE(command.valid());
}

TEST(AlarmSuppressionRecordTests, RecordCanBeCreatedFromCommand)
{
    using Command = dispatcher::alarm::AlarmSuppressionCommand;
    using Record = dispatcher::alarm::AlarmSuppressionRecord;

    const auto now = Command::Clock::now();
    const auto expires_at = now + std::chrono::minutes(30);

    const Command command(
        dispatcher::domain::AlarmId{ "alarm-1" },
        "operator-1",
        dispatcher::alarm::AlarmSuppressionMode::Shelved,
        dispatcher::alarm::AlarmSuppressionReason::Maintenance,
        "planned maintenance",
        expires_at
    );

    const auto record = Record::from_command(command, now);

    EXPECT_EQ(record.alarm_id(), dispatcher::domain::AlarmId{ "alarm-1" });
    EXPECT_EQ(record.operator_id(), "operator-1");

    EXPECT_EQ(
        record.mode(),
        dispatcher::alarm::AlarmSuppressionMode::Shelved
    );

    EXPECT_EQ(
        record.reason(),
        dispatcher::alarm::AlarmSuppressionReason::Maintenance
    );

    EXPECT_EQ(record.applied_at(), now);
    EXPECT_EQ(record.comment(), "planned maintenance");
    EXPECT_TRUE(record.has_comment());
    EXPECT_TRUE(record.has_expiration());

    EXPECT_TRUE(record.active_at(now));
    EXPECT_FALSE(record.expired_at(now));
    EXPECT_FALSE(record.active_at(expires_at));
    EXPECT_TRUE(record.expired_at(expires_at));
}

TEST(AlarmSuppressionRecordTests, RecordWithoutExpirationRemainsActive)
{
    using Record = dispatcher::alarm::AlarmSuppressionRecord;

    const auto now = Record::Clock::now();

    const Record record(
        dispatcher::domain::AlarmId{ "alarm-1" },
        "operator-1",
        dispatcher::alarm::AlarmSuppressionMode::Suppressed,
        dispatcher::alarm::AlarmSuppressionReason::Testing,
        now
    );

    EXPECT_FALSE(record.has_comment());
    EXPECT_FALSE(record.has_expiration());

    EXPECT_TRUE(record.active_at(now));
    EXPECT_TRUE(record.active_at(now + std::chrono::hours(24)));
}

TEST(AlarmSuppressionResultTests, AppliedResultContainsRecord)
{
    using Record = dispatcher::alarm::AlarmSuppressionRecord;

    const auto now = Record::Clock::now();

    const Record record(
        dispatcher::domain::AlarmId{ "alarm-1" },
        "operator-1",
        dispatcher::alarm::AlarmSuppressionMode::Shelved,
        dispatcher::alarm::AlarmSuppressionReason::Maintenance,
        now
    );

    const auto result =
        dispatcher::alarm::AlarmSuppressionResult::applied(record);

    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.failed());
    EXPECT_TRUE(result.applied());
    EXPECT_FALSE(result.cleared());

    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::AlarmSuppressionStatus::Applied
    );

    ASSERT_TRUE(result.has_record());
    EXPECT_EQ(result.record().alarm_id(), dispatcher::domain::AlarmId{ "alarm-1" });
}

TEST(AlarmSuppressionResultTests, ClearedResultContainsRecord)
{
    using Record = dispatcher::alarm::AlarmSuppressionRecord;

    const auto now = Record::Clock::now();

    const Record record(
        dispatcher::domain::AlarmId{ "alarm-1" },
        "operator-1",
        dispatcher::alarm::AlarmSuppressionMode::Shelved,
        dispatcher::alarm::AlarmSuppressionReason::Maintenance,
        now
    );

    const auto result =
        dispatcher::alarm::AlarmSuppressionResult::cleared(record);

    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.cleared());
    EXPECT_FALSE(result.applied());

    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::AlarmSuppressionStatus::Cleared
    );

    ASSERT_TRUE(result.has_record());
    EXPECT_EQ(result.record().operator_id(), "operator-1");
}

TEST(AlarmSuppressionResultTests, FailureResultDoesNotContainRecord)
{
    const auto result =
        dispatcher::alarm::AlarmSuppressionResult::failure(
            dispatcher::alarm::AlarmSuppressionStatus::UnknownAlarm,
            "alarm is not configured"
        );

    EXPECT_FALSE(result.success());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::AlarmSuppressionStatus::UnknownAlarm
    );

    EXPECT_EQ(result.message(), "alarm is not configured");
    EXPECT_TRUE(result.has_message());
    EXPECT_FALSE(result.has_record());

    EXPECT_THROW(
        (void)result.record(),
        std::logic_error
    );
}

TEST(AlarmSuppressionResultTests, FailureWithSuccessStatusBecomesInvalidCommand)
{
    const auto result =
        dispatcher::alarm::AlarmSuppressionResult::failure(
            dispatcher::alarm::AlarmSuppressionStatus::Applied,
            "invalid failure status"
        );

    EXPECT_FALSE(result.success());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::alarm::AlarmSuppressionStatus::InvalidCommand
    );

    EXPECT_EQ(result.message(), "invalid failure status");
}