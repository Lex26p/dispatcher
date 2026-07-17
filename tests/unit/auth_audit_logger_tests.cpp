#include <dispatcher/auth/audit/auth_audit.hpp>

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    dispatcher::auth::audit::AuthAuditEvent make_event(
        std::string event_id = "audit-event-1"
    )
    {
        dispatcher::auth::audit::AuthAuditEvent event;

        event.event_id =
            std::move(
                event_id
            );

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

        event.diagnostic_message = "Alarm acknowledgement accepted.";

        event.attributes.emplace(
            "tag_id",
            "pump.pressure"
        );

        return event;
    }

    void expect_logger_construction_throws(
        dispatcher::auth::audit::IAuthAuditSink& sink
    )
    {
        EXPECT_THROW(
            {
                dispatcher::auth::audit::AuthAuditLogger logger{
                    sink
                };

                static_cast<void>(
                    logger
                );
            },
            dispatcher::auth::audit::AuthAuditError
        );
    }

    void expect_record_throws(
        dispatcher::auth::audit::AuthAuditLogger& logger,
        const dispatcher::auth::audit::AuthAuditEvent& event
    )
    {
        EXPECT_THROW(
            {
                const auto result =
                    logger.record(
                        event
                    );

                static_cast<void>(
                    result
                );
            },
            dispatcher::auth::audit::AuthAuditError
        );
    }

    class EmptyNameAuditSink final
        : public dispatcher::auth::audit::IAuthAuditSink
    {
    public:
        [[nodiscard]] std::string sink_name() const override
        {
            return "";
        }

        [[nodiscard]] dispatcher::auth::audit::AuthAuditRecordResult record(
            const dispatcher::auth::audit::AuthAuditEvent&
        ) override
        {
            return dispatcher::auth::audit::AuthAuditRecordResult::accepted();
        }
    };

    class ThrowingAuditSink final
        : public dispatcher::auth::audit::IAuthAuditSink
    {
    public:
        [[nodiscard]] std::string sink_name() const override
        {
            return "throwing-auth-audit-sink";
        }

        [[nodiscard]] dispatcher::auth::audit::AuthAuditRecordResult record(
            const dispatcher::auth::audit::AuthAuditEvent&
        ) override
        {
            throw dispatcher::auth::audit::AuthAuditError(
                "sink failed unexpectedly"
            );
        }
    };

    class InvalidResultAuditSink final
        : public dispatcher::auth::audit::IAuthAuditSink
    {
    public:
        [[nodiscard]] std::string sink_name() const override
        {
            return "invalid-result-auth-audit-sink";
        }

        [[nodiscard]] dispatcher::auth::audit::AuthAuditRecordResult record(
            const dispatcher::auth::audit::AuthAuditEvent&
        ) override
        {
            auto result =
                dispatcher::auth::audit::AuthAuditRecordResult::accepted(
                    "provider-record"
                );

            result.error_message =
                "invalid error on accepted result";

            return result;
        }
    };
}

TEST(AuthAuditLoggerTests, InMemorySinkRecordsEvent)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    const auto result =
        sink.record(
            make_event()
        );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_EQ(
        result.status,
        dispatcher::auth::audit::AuthAuditRecordStatus::accepted
    );

    EXPECT_EQ(
        result.provider_record_id,
        "in-memory-auth-audit-sink:audit-event-1"
    );

    EXPECT_EQ(
        sink.record_attempt_count(),
        1
    );

    ASSERT_EQ(
        sink.recorded_events().size(),
        1U
    );

    EXPECT_EQ(
        sink.recorded_events()[0].event_id,
        "audit-event-1"
    );
}

TEST(AuthAuditLoggerTests, InMemorySinkCanBeForcedToFail)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    sink.set_failure(
        "storage unavailable"
    );

    const auto result =
        sink.record(
            make_event()
        );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_EQ(
        result.status,
        dispatcher::auth::audit::AuthAuditRecordStatus::failed
    );

    EXPECT_EQ(
        result.error_message,
        "storage unavailable"
    );

    EXPECT_TRUE(
        sink.recorded_events().empty()
    );

    EXPECT_EQ(
        sink.record_attempt_count(),
        1
    );
}

TEST(AuthAuditLoggerTests, InMemorySinkCanSkipWhenDisabled)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    sink.set_recording_enabled(
        false
    );

    const auto result =
        sink.record(
            make_event()
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

    EXPECT_TRUE(
        sink.recorded_events().empty()
    );

    EXPECT_EQ(
        sink.record_attempt_count(),
        1
    );
}

TEST(AuthAuditLoggerTests, InMemorySinkClearResetsState)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    const auto first_result =
        sink.record(
            make_event()
        );

    static_cast<void>(
        first_result
        );

    sink.set_failure(
        "storage unavailable"
    );

    sink.set_recording_enabled(
        false
    );

    sink.clear();

    EXPECT_TRUE(
        sink.recording_enabled()
    );

    EXPECT_EQ(
        sink.record_attempt_count(),
        0
    );

    EXPECT_TRUE(
        sink.recorded_events().empty()
    );

    const auto second_result =
        sink.record(
            make_event(
                "audit-event-2"
            )
        );

    EXPECT_TRUE(
        second_result.success()
    );

    ASSERT_EQ(
        sink.recorded_events().size(),
        1U
    );

    EXPECT_EQ(
        sink.recorded_events()[0].event_id,
        "audit-event-2"
    );
}

TEST(AuthAuditLoggerTests, LoggerRecordsThroughSink)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    const auto result =
        logger.record(
            make_event()
        );

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_TRUE(
        logger.enabled()
    );

    EXPECT_EQ(
        logger.sink_name(),
        "in-memory-auth-audit-sink"
    );

    ASSERT_EQ(
        sink.recorded_events().size(),
        1U
    );

    EXPECT_EQ(
        sink.recorded_events()[0].event_id,
        "audit-event-1"
    );
}

TEST(AuthAuditLoggerTests, LoggerCanBeDisabled)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    logger.set_enabled(
        false
    );

    const auto result =
        logger.record(
            make_event()
        );

    EXPECT_EQ(
        result.status,
        dispatcher::auth::audit::AuthAuditRecordStatus::skipped
    );

    EXPECT_EQ(
        result.diagnostic_message,
        "Auth audit logger is disabled."
    );

    EXPECT_EQ(
        sink.record_attempt_count(),
        0
    );

    EXPECT_TRUE(
        sink.recorded_events().empty()
    );
}

TEST(AuthAuditLoggerTests, LoggerRejectsEmptySinkName)
{
    EmptyNameAuditSink sink;

    expect_logger_construction_throws(
        sink
    );
}

TEST(AuthAuditLoggerTests, LoggerValidatesEventBeforeRecording)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    auto event =
        make_event();

    event.resource.resource_id = "";

    expect_record_throws(
        logger,
        event
    );

    EXPECT_EQ(
        sink.record_attempt_count(),
        0
    );

    EXPECT_TRUE(
        sink.recorded_events().empty()
    );
}

TEST(AuthAuditLoggerTests, LoggerConvertsSinkExceptionToFailedResult)
{
    ThrowingAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    const auto result =
        logger.record(
            make_event()
        );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_EQ(
        result.status,
        dispatcher::auth::audit::AuthAuditRecordStatus::failed
    );

    EXPECT_EQ(
        result.error_message,
        "sink failed unexpectedly"
    );

    EXPECT_EQ(
        result.diagnostic_message,
        "Auth audit sink threw an exception."
    );
}

TEST(AuthAuditLoggerTests, LoggerConvertsInvalidSinkResultToFailedResult)
{
    InvalidResultAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    const auto result =
        logger.record(
            make_event()
        );

    EXPECT_TRUE(
        result.failure()
    );

    EXPECT_EQ(
        result.status,
        dispatcher::auth::audit::AuthAuditRecordStatus::failed
    );

    EXPECT_NE(
        result.error_message.find(
            "Accepted auth audit record result must not contain error_message"
        ),
        std::string::npos
    );
}

TEST(AuthAuditLoggerTests, LoggerRecordsBatchInOrder)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    const std::vector<dispatcher::auth::audit::AuthAuditEvent> events{
        make_event(
            "audit-event-1"
        ),
        make_event(
            "audit-event-2"
        ),
        make_event(
            "audit-event-3"
        )
    };

    const auto results =
        logger.record_batch(
            events
        );

    ASSERT_EQ(
        results.size(),
        3U
    );

    EXPECT_TRUE(
        results[0].success()
    );

    EXPECT_TRUE(
        results[1].success()
    );

    EXPECT_TRUE(
        results[2].success()
    );

    ASSERT_EQ(
        sink.recorded_events().size(),
        3U
    );

    EXPECT_EQ(
        sink.recorded_events()[0].event_id,
        "audit-event-1"
    );

    EXPECT_EQ(
        sink.recorded_events()[1].event_id,
        "audit-event-2"
    );

    EXPECT_EQ(
        sink.recorded_events()[2].event_id,
        "audit-event-3"
    );
}

TEST(AuthAuditLoggerTests, LoggerBatchStopsOnInvalidEventByThrowing)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    auto valid =
        make_event(
            "audit-event-1"
        );

    auto invalid =
        make_event(
            "audit-event-2"
        );

    invalid.actor.actor_id = "";

    const std::vector<dispatcher::auth::audit::AuthAuditEvent> events{
        valid,
        invalid
    };

    EXPECT_THROW(
        {
            const auto results =
                logger.record_batch(
                    events
                );

            static_cast<void>(
                results
            );
        },
        dispatcher::auth::audit::AuthAuditError
    );

    ASSERT_EQ(
        sink.recorded_events().size(),
        1U
    );

    EXPECT_EQ(
        sink.recorded_events()[0].event_id,
        "audit-event-1"
    );
}

TEST(AuthAuditLoggerTests, LoggerBatchCanReturnFailuresWithoutThrowing)
{
    dispatcher::auth::audit::InMemoryAuthAuditSink sink;

    sink.set_failure(
        "storage unavailable"
    );

    dispatcher::auth::audit::AuthAuditLogger logger{
        sink
    };

    const std::vector<dispatcher::auth::audit::AuthAuditEvent> events{
        make_event(
            "audit-event-1"
        ),
        make_event(
            "audit-event-2"
        )
    };

    const auto results =
        logger.record_batch(
            events
        );

    ASSERT_EQ(
        results.size(),
        2U
    );

    EXPECT_TRUE(
        results[0].failure()
    );

    EXPECT_TRUE(
        results[1].failure()
    );

    EXPECT_EQ(
        sink.record_attempt_count(),
        2
    );

    EXPECT_TRUE(
        sink.recorded_events().empty()
    );
}