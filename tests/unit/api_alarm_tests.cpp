#include <dispatcher/api/alarm_acknowledgement_api_result.hpp>
#include <dispatcher/api/alarm_acknowledgement_request.hpp>
#include <dispatcher/api/alarm_api.hpp>
#include <dispatcher/api/api_status.hpp>
#include <dispatcher/api/dispatcher_alarm_api.hpp>
#include <dispatcher/api/unacknowledged_alarms_api_result.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/runtime/dispatcher_runtime.hpp>

#include <gtest/gtest.h>

#include <stdexcept>

TEST(AlarmAcknowledgementRequestTests, RequestPredicatesReflectFields)
{
    const dispatcher::api::AlarmAcknowledgementRequest request{
        .alarm_id = dispatcher::domain::AlarmId{"alarm-1"},
        .operator_id = "operator-1",
        .comment = "acknowledged from api"
    };

    EXPECT_TRUE(request.has_alarm_id());
    EXPECT_TRUE(request.has_operator_id());
    EXPECT_TRUE(request.has_comment());
}

TEST(AlarmAcknowledgementRequestTests, EmptyRequestPredicatesAreFalse)
{
    const dispatcher::api::AlarmAcknowledgementRequest request;

    EXPECT_FALSE(request.has_alarm_id());
    EXPECT_FALSE(request.has_operator_id());
    EXPECT_FALSE(request.has_comment());
}

TEST(AlarmAcknowledgementRequestTests, RequestConvertsToCommand)
{
    const dispatcher::api::AlarmAcknowledgementRequest request{
        .alarm_id = dispatcher::domain::AlarmId{"alarm-1"},
        .operator_id = "operator-1",
        .comment = "acknowledged from api"
    };

    const auto command = request.to_command();

    EXPECT_EQ(command.alarm_id(), dispatcher::domain::AlarmId{ "alarm-1" });
    EXPECT_EQ(command.operator_id(), "operator-1");
    EXPECT_EQ(command.comment(), "acknowledged from api");
}

TEST(AlarmAcknowledgementApiResultTests, FailureResultDoesNotContainAcknowledgement)
{
    const auto result =
        dispatcher::api::AlarmAcknowledgementApiResult::failure(
            dispatcher::api::ApiStatus::NotFound,
            "alarm.acknowledge",
            "alarm-404",
            {},
            "unknown_alarm"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(result.status(), dispatcher::api::ApiStatus::NotFound);
    EXPECT_FALSE(result.has_acknowledgement());

    EXPECT_EQ(result.error().operation, "alarm.acknowledge");
    EXPECT_EQ(result.error().resource, "alarm-404");
    EXPECT_EQ(result.error().message, "unknown_alarm");

    EXPECT_THROW(
        (void)result.acknowledgement(),
        std::logic_error
    );
}

TEST(UnacknowledgedAlarmsApiResultTests, SuccessResultContainsAlarmList)
{
    dispatcher::runtime::DispatcherRuntime runtime;

    const auto result =
        dispatcher::api::UnacknowledgedAlarmsApiResult::success(
            runtime.unacknowledged_alarms()
        );

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(result.status(), dispatcher::api::ApiStatus::Success);
    EXPECT_TRUE(result.has_alarms());
    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.alarm_count(), 0);
    EXPECT_TRUE(result.alarms().empty());
}

TEST(UnacknowledgedAlarmsApiResultTests, FailureResultDoesNotContainAlarmList)
{
    const auto result =
        dispatcher::api::UnacknowledgedAlarmsApiResult::failure(
            dispatcher::api::ApiStatus::InternalError,
            "alarm.unacknowledged_alarms",
            "runtime",
            {},
            "runtime is not available"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(result.status(), dispatcher::api::ApiStatus::InternalError);
    EXPECT_FALSE(result.has_alarms());
    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.alarm_count(), 0);

    EXPECT_EQ(result.error().operation, "alarm.unacknowledged_alarms");
    EXPECT_EQ(result.error().resource, "runtime");
    EXPECT_EQ(result.error().message, "runtime is not available");

    EXPECT_THROW(
        (void)result.alarms(),
        std::logic_error
    );
}

TEST(DispatcherAlarmApiTests, AlarmApiReturnsOperatorSnapshot)
{
    dispatcher::runtime::DispatcherRuntime runtime;
    dispatcher::api::DispatcherAlarmApi api(runtime);

    dispatcher::api::AlarmApi& alarm_api = api;

    const auto result = alarm_api.operator_snapshot();

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_snapshot());

    EXPECT_EQ(result.snapshot().configured_alarm_count, 0);
    EXPECT_EQ(result.snapshot().active_alarm_count, 0);
    EXPECT_EQ(result.snapshot().acknowledged_alarm_count, 0);
    EXPECT_EQ(result.snapshot().unacknowledged_alarm_count, 0);
    EXPECT_FALSE(result.snapshot().requires_operator_attention());
}

TEST(DispatcherAlarmApiTests, AlarmApiReturnsUnacknowledgedAlarms)
{
    dispatcher::runtime::DispatcherRuntime runtime;
    dispatcher::api::DispatcherAlarmApi api(runtime);

    dispatcher::api::AlarmApi& alarm_api = api;

    const auto result = alarm_api.unacknowledged_alarms();

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_alarms());

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.alarm_count(), 0);
}

TEST(DispatcherAlarmApiTests, AcknowledgeUnknownAlarmReturnsNotFound)
{
    dispatcher::runtime::DispatcherRuntime runtime;
    dispatcher::api::DispatcherAlarmApi api(runtime);

    dispatcher::api::AlarmApi& alarm_api = api;

    const auto result = alarm_api.acknowledge(
        dispatcher::api::AlarmAcknowledgementRequest{
            .alarm_id = dispatcher::domain::AlarmId{"alarm-404"},
            .operator_id = "operator-1",
            .comment = "acknowledged from api"
        }
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(result.status(), dispatcher::api::ApiStatus::NotFound);
    EXPECT_FALSE(result.has_acknowledgement());

    EXPECT_EQ(result.error().operation, "alarm.acknowledge");
    EXPECT_EQ(result.error().resource, "alarm-404");
    EXPECT_EQ(result.error().message, "unknown_alarm");
}

TEST(DispatcherAlarmApiTests, AcknowledgeInvalidCommandReturnsValidationError)
{
    dispatcher::runtime::DispatcherRuntime runtime;
    dispatcher::api::DispatcherAlarmApi api(runtime);

    dispatcher::api::AlarmApi& alarm_api = api;

    const auto result = alarm_api.acknowledge(
        dispatcher::api::AlarmAcknowledgementRequest{
            .alarm_id = dispatcher::domain::AlarmId{"alarm-1"},
            .operator_id = "",
            .comment = "missing operator id"
        }
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::ApiStatus::ValidationError
    );

    EXPECT_FALSE(result.has_acknowledgement());

    EXPECT_EQ(result.error().operation, "alarm.acknowledge");
    EXPECT_EQ(result.error().resource, "alarm-1");
    EXPECT_EQ(result.error().message, "invalid_command");
}

TEST(DispatcherAlarmApiTests, ExposesWrappedRuntime)
{
    dispatcher::runtime::DispatcherRuntime runtime;
    dispatcher::api::DispatcherAlarmApi api(runtime);

    EXPECT_EQ(&api.runtime(), &runtime);

    const auto& const_api = api;

    EXPECT_EQ(&const_api.runtime(), &runtime);
}