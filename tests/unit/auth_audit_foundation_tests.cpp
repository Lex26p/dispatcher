#include <dispatcher/auth/audit/auth_audit.hpp>

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace
{
    dispatcher::auth::audit::AuthAuditEvent make_event()
    {
        dispatcher::auth::audit::AuthAuditEvent event;

        event.event_id = "audit-event-1";
        event.correlation_id = "operation-1";
        event.source = "http-api";

        event.actor.actor_id = "operator-1";
        event.actor.display_name = "Operator One";
        event.actor.actor_type =
            dispatcher::auth::audit::AuthAuditActorType::operator_user;

        event.action =
            dispatcher::auth::audit::AuthAuditAction::alarm_acknowledge;

        event.outcome =
            dispatcher::auth::audit::AuthAuditOutcome::success;

        event.severity =
            dispatcher::auth::audit::AuthAuditSeverity::info;

        event.resource.resource_type = "alarm";
        event.resource.resource_id = "alarm-1";
        event.resource.display_name = "Pump pressure high";

        event.reason = "";
        event.diagnostic_message = "Alarm acknowledgement accepted.";

        event.attributes.emplace(
            "tag_id",
            "pump.pressure"
        );

        event.attributes.emplace(
            "station",
            "plant-a"
        );

        return event;
    }

    void expect_validate_event_throws(
        const dispatcher::auth::audit::AuthAuditEvent& event
    )
    {
        EXPECT_THROW(
            dispatcher::auth::audit::AuthAuditValidator::validate_event(
                event
            ),
            dispatcher::auth::audit::AuthAuditError
        );
    }

    void expect_validate_record_result_throws(
        const dispatcher::auth::audit::AuthAuditRecordResult& result
    )
    {
        EXPECT_THROW(
            dispatcher::auth::audit::AuthAuditValidator::validate_record_result(
                result
            ),
            dispatcher::auth::audit::AuthAuditError
        );
    }

    class RecordingAuthAuditSink final
        : public dispatcher::auth::audit::IAuthAuditSink
    {
    public:
        [[nodiscard]] std::string sink_name() const override
        {
            return "recording-auth-audit-sink";
        }

        [[nodiscard]] dispatcher::auth::audit::AuthAuditRecordResult record(
            const dispatcher::auth::audit::AuthAuditEvent& event
        ) override
        {
            dispatcher::auth::audit::AuthAuditValidator::validate_event(
                event
            );

            recorded_events.push_back(
                event
            );

            return dispatcher::auth::audit::AuthAuditRecordResult::accepted(
                "recording:" + event.event_id,
                "recorded"
            );
        }

        std::vector<dispatcher::auth::audit::AuthAuditEvent> recorded_events{};
    };
}

TEST(AuthAuditFoundationTests, ValidatesGoodAuditEvent)
{
    const auto event =
        make_event();

    EXPECT_NO_THROW(
        dispatcher::auth::audit::AuthAuditValidator::validate_event(
            event
        )
    );
}

TEST(AuthAuditFoundationTests, RejectsEmptyEventId)
{
    auto event =
        make_event();

    event.event_id = "";

    expect_validate_event_throws(
        event
    );
}

TEST(AuthAuditFoundationTests, RejectsEmptySource)
{
    auto event =
        make_event();

    event.source = "";

    expect_validate_event_throws(
        event
    );
}

TEST(AuthAuditFoundationTests, RejectsInvalidActorType)
{
    auto event =
        make_event();

    event.actor.actor_type =
        static_cast<dispatcher::auth::audit::AuthAuditActorType>(
            999
            );

    expect_validate_event_throws(
        event
    );
}

TEST(AuthAuditFoundationTests, RejectsEmptyActorId)
{
    auto event =
        make_event();

    event.actor.actor_id = "";

    expect_validate_event_throws(
        event
    );
}

TEST(AuthAuditFoundationTests, RejectsUnknownAction)
{
    auto event =
        make_event();

    event.action =
        dispatcher::auth::audit::AuthAuditAction::unknown;

    expect_validate_event_throws(
        event
    );
}

TEST(AuthAuditFoundationTests, RejectsInvalidAction)
{
    auto event =
        make_event();

    event.action =
        static_cast<dispatcher::auth::audit::AuthAuditAction>(
            999
            );

    expect_validate_event_throws(
        event
    );
}

TEST(AuthAuditFoundationTests, RejectsInvalidOutcome)
{
    auto event =
        make_event();

    event.outcome =
        static_cast<dispatcher::auth::audit::AuthAuditOutcome>(
            999
            );

    expect_validate_event_throws(
        event
    );
}

TEST(AuthAuditFoundationTests, RejectsInvalidSeverity)
{
    auto event =
        make_event();

    event.severity =
        static_cast<dispatcher::auth::audit::AuthAuditSeverity>(
            999
            );

    expect_validate_event_throws(
        event
    );
}

TEST(AuthAuditFoundationTests, RejectsEmptyResourceType)
{
    auto event =
        make_event();

    event.resource.resource_type = "";

    expect_validate_event_throws(
        event
    );
}

TEST(AuthAuditFoundationTests, RejectsEmptyResourceId)
{
    auto event =
        make_event();

    event.resource.resource_id = "";

    expect_validate_event_throws(
        event
    );
}

TEST(AuthAuditFoundationTests, RejectsDeniedOutcomeWithoutReason)
{
    auto event =
        make_event();

    event.outcome =
        dispatcher::auth::audit::AuthAuditOutcome::denied;

    event.reason = "";

    expect_validate_event_throws(
        event
    );
}

TEST(AuthAuditFoundationTests, AllowsDeniedOutcomeWithReason)
{
    auto event =
        make_event();

    event.outcome =
        dispatcher::auth::audit::AuthAuditOutcome::denied;

    event.reason =
        "operator lacks permission";

    EXPECT_NO_THROW(
        dispatcher::auth::audit::AuthAuditValidator::validate_event(
            event
        )
    );
}

TEST(AuthAuditFoundationTests, CreatesAcceptedRecordResult)
{
    const auto result =
        dispatcher::auth::audit::AuthAuditRecordResult::accepted(
            "provider-record-1",
            "stored"
        );

    EXPECT_EQ(
        result.status,
        dispatcher::auth::audit::AuthAuditRecordStatus::accepted
    );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_FALSE(
        result.failure()
    );

    EXPECT_EQ(
        result.provider_record_id,
        "provider-record-1"
    );

    EXPECT_EQ(
        result.diagnostic_message,
        "stored"
    );

    EXPECT_NO_THROW(
        dispatcher::auth::audit::AuthAuditValidator::validate_record_result(
            result
        )
    );
}

TEST(AuthAuditFoundationTests, CreatesFailedRecordResult)
{
    const auto result =
        dispatcher::auth::audit::AuthAuditRecordResult::failed(
            "sink unavailable",
            "write failed"
        );

    EXPECT_EQ(
        result.status,
        dispatcher::auth::audit::AuthAuditRecordStatus::failed
    );

    EXPECT_FALSE(
        result.success()
    );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_EQ(
        result.error_message,
        "sink unavailable"
    );

    EXPECT_NO_THROW(
        dispatcher::auth::audit::AuthAuditValidator::validate_record_result(
            result
        )
    );
}

TEST(AuthAuditFoundationTests, CreatesSkippedRecordResult)
{
    const auto result =
        dispatcher::auth::audit::AuthAuditRecordResult::skipped(
            "audit disabled"
        );

    EXPECT_EQ(
        result.status,
        dispatcher::auth::audit::AuthAuditRecordStatus::skipped
    );

    EXPECT_FALSE(
        result.success()
    );

    EXPECT_FALSE(
        result.failure()
    );

    EXPECT_EQ(
        result.diagnostic_message,
        "audit disabled"
    );

    EXPECT_NO_THROW(
        dispatcher::auth::audit::AuthAuditValidator::validate_record_result(
            result
        )
    );
}

TEST(AuthAuditFoundationTests, RejectsFailedRecordResultWithoutErrorMessage)
{
    dispatcher::auth::audit::AuthAuditRecordResult result;

    result.status =
        dispatcher::auth::audit::AuthAuditRecordStatus::failed;

    result.error_message = "";

    expect_validate_record_result_throws(
        result
    );
}

TEST(AuthAuditFoundationTests, RejectsAcceptedRecordResultWithErrorMessage)
{
    auto result =
        dispatcher::auth::audit::AuthAuditRecordResult::accepted();

    result.error_message =
        "unexpected error";

    expect_validate_record_result_throws(
        result
    );
}

TEST(AuthAuditFoundationTests, RejectsSkippedRecordResultWithoutDiagnosticMessage)
{
    dispatcher::auth::audit::AuthAuditRecordResult result;

    result.status =
        dispatcher::auth::audit::AuthAuditRecordStatus::skipped;

    result.diagnostic_message = "";

    expect_validate_record_result_throws(
        result
    );
}

TEST(AuthAuditFoundationTests, ConvertsEnumsToStrings)
{
    EXPECT_EQ(
        dispatcher::auth::audit::AuthAuditValidator::actor_type_to_string(
            dispatcher::auth::audit::AuthAuditActorType::operator_user
        ),
        "operator_user"
    );

    EXPECT_EQ(
        dispatcher::auth::audit::AuthAuditValidator::action_to_string(
            dispatcher::auth::audit::AuthAuditAction::alarm_acknowledge
        ),
        "alarm_acknowledge"
    );

    EXPECT_EQ(
        dispatcher::auth::audit::AuthAuditValidator::outcome_to_string(
            dispatcher::auth::audit::AuthAuditOutcome::denied
        ),
        "denied"
    );

    EXPECT_EQ(
        dispatcher::auth::audit::AuthAuditValidator::severity_to_string(
            dispatcher::auth::audit::AuthAuditSeverity::critical
        ),
        "critical"
    );

    EXPECT_EQ(
        dispatcher::auth::audit::AuthAuditValidator::record_status_to_string(
            dispatcher::auth::audit::AuthAuditRecordStatus::accepted
        ),
        "accepted"
    );
}

TEST(AuthAuditFoundationTests, UnknownEnumValuesConvertToUnknown)
{
    EXPECT_EQ(
        dispatcher::auth::audit::AuthAuditValidator::actor_type_to_string(
            static_cast<dispatcher::auth::audit::AuthAuditActorType>(
                999
                )
        ),
        "unknown"
    );

    EXPECT_EQ(
        dispatcher::auth::audit::AuthAuditValidator::action_to_string(
            static_cast<dispatcher::auth::audit::AuthAuditAction>(
                999
                )
        ),
        "unknown"
    );

    EXPECT_EQ(
        dispatcher::auth::audit::AuthAuditValidator::outcome_to_string(
            static_cast<dispatcher::auth::audit::AuthAuditOutcome>(
                999
                )
        ),
        "unknown"
    );

    EXPECT_EQ(
        dispatcher::auth::audit::AuthAuditValidator::severity_to_string(
            static_cast<dispatcher::auth::audit::AuthAuditSeverity>(
                999
                )
        ),
        "unknown"
    );

    EXPECT_EQ(
        dispatcher::auth::audit::AuthAuditValidator::record_status_to_string(
            static_cast<dispatcher::auth::audit::AuthAuditRecordStatus>(
                999
                )
        ),
        "unknown"
    );
}

TEST(AuthAuditFoundationTests, AuditSinkInterfaceCanRecordEvent)
{
    RecordingAuthAuditSink sink;

    const auto event =
        make_event();

    const auto result =
        sink.record(
            event
        );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_EQ(
        result.provider_record_id,
        "recording:audit-event-1"
    );

    EXPECT_EQ(
        sink.sink_name(),
        "recording-auth-audit-sink"
    );

    ASSERT_EQ(
        sink.recorded_events.size(),
        1U
    );

    EXPECT_EQ(
        sink.recorded_events[0].event_id,
        "audit-event-1"
    );

    EXPECT_EQ(
        sink.recorded_events[0].attributes.at(
            "tag_id"
        ),
        "pump.pressure"
    );
}

TEST(AuthAuditFoundationTests, AuditSinkRejectsInvalidEvent)
{
    RecordingAuthAuditSink sink;

    auto event =
        make_event();

    event.resource.resource_id = "";

    EXPECT_THROW(
        {
            const auto result =
                sink.record(
                    event
                );

            static_cast<void>(
                result
            );
        },
        dispatcher::auth::audit::AuthAuditError
    );

    EXPECT_TRUE(
        sink.recorded_events.empty()
    );
}