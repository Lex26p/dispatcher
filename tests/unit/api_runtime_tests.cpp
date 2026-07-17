#include <dispatcher/api/alarm_operator_snapshot_api_result.hpp>
#include <dispatcher/api/api_status.hpp>
#include <dispatcher/api/dispatcher_runtime_api.hpp>
#include <dispatcher/api/runtime_api.hpp>
#include <dispatcher/api/runtime_snapshot_api_result.hpp>
#include <dispatcher/runtime/dispatcher_runtime.hpp>

#include <gtest/gtest.h>

#include <stdexcept>

TEST(RuntimeSnapshotApiResultTests, SuccessResultContainsSnapshot)
{
    dispatcher::runtime::DispatcherRuntime runtime;

    const auto runtime_snapshot = runtime.runtime_snapshot();

    const auto result =
        dispatcher::api::RuntimeSnapshotApiResult::success(runtime_snapshot);

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(result.status(), dispatcher::api::ApiStatus::Success);
    EXPECT_TRUE(result.has_snapshot());

    EXPECT_EQ(
        result.snapshot().telemetry.current_state_size,
        runtime_snapshot.telemetry.current_state_size
    );
}

TEST(RuntimeSnapshotApiResultTests, FailureResultDoesNotContainSnapshot)
{
    const auto result =
        dispatcher::api::RuntimeSnapshotApiResult::failure(
            dispatcher::api::ApiStatus::InternalError,
            "runtime.snapshot",
            "runtime",
            {},
            "runtime is not available"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(result.status(), dispatcher::api::ApiStatus::InternalError);
    EXPECT_FALSE(result.has_snapshot());

    EXPECT_EQ(result.error().operation, "runtime.snapshot");
    EXPECT_EQ(result.error().resource, "runtime");
    EXPECT_EQ(result.error().message, "runtime is not available");

    EXPECT_THROW(
        (void)result.snapshot(),
        std::logic_error
    );
}

TEST(AlarmOperatorSnapshotApiResultTests, SuccessResultContainsSnapshot)
{
    dispatcher::alarm::AlarmOperatorSnapshot snapshot{
        .configuration_version = 7,
        .configured_alarm_count = 10,
        .active_alarm_count = 3,
        .unacknowledged_alarm_count = 3
    };

    const auto result =
        dispatcher::api::AlarmOperatorSnapshotApiResult::success(snapshot);

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(result.status(), dispatcher::api::ApiStatus::Success);
    EXPECT_TRUE(result.has_snapshot());

    EXPECT_EQ(result.snapshot().configuration_version, 7);
    EXPECT_EQ(result.snapshot().configured_alarm_count, 10);
    EXPECT_EQ(result.snapshot().active_alarm_count, 3);
    EXPECT_EQ(result.snapshot().unacknowledged_alarm_count, 3);
}

TEST(AlarmOperatorSnapshotApiResultTests, FailureResultDoesNotContainSnapshot)
{
    const auto result =
        dispatcher::api::AlarmOperatorSnapshotApiResult::failure(
            dispatcher::api::ApiStatus::InternalError,
            "runtime.alarm_operator_snapshot",
            "runtime",
            {},
            "runtime is not available"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(result.status(), dispatcher::api::ApiStatus::InternalError);
    EXPECT_FALSE(result.has_snapshot());

    EXPECT_EQ(result.error().operation, "runtime.alarm_operator_snapshot");
    EXPECT_EQ(result.error().resource, "runtime");
    EXPECT_EQ(result.error().message, "runtime is not available");

    EXPECT_THROW(
        (void)result.snapshot(),
        std::logic_error
    );
}

TEST(DispatcherRuntimeApiTests, RuntimeApiReturnsRuntimeSnapshot)
{
    dispatcher::runtime::DispatcherRuntime runtime;
    dispatcher::api::DispatcherRuntimeApi api(runtime);

    dispatcher::api::RuntimeApi& runtime_api = api;

    const auto result = runtime_api.runtime_snapshot();

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_snapshot());

    EXPECT_EQ(
        result.snapshot().telemetry.current_state_size,
        0
    );

    EXPECT_EQ(
        result.snapshot().history.store_size,
        0
    );

    EXPECT_EQ(
        result.snapshot().alarm.event_store_size,
        0
    );
}

TEST(DispatcherRuntimeApiTests, RuntimeApiReturnsAlarmOperatorSnapshot)
{
    dispatcher::runtime::DispatcherRuntime runtime;
    dispatcher::api::DispatcherRuntimeApi api(runtime);

    dispatcher::api::RuntimeApi& runtime_api = api;

    const auto result = runtime_api.alarm_operator_snapshot();

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_snapshot());

    EXPECT_EQ(result.snapshot().configured_alarm_count, 0);
    EXPECT_EQ(result.snapshot().active_alarm_count, 0);
    EXPECT_EQ(result.snapshot().acknowledged_alarm_count, 0);
    EXPECT_EQ(result.snapshot().unacknowledged_alarm_count, 0);
    EXPECT_FALSE(result.snapshot().requires_operator_attention());
}

TEST(DispatcherRuntimeApiTests, ExposesWrappedRuntime)
{
    dispatcher::runtime::DispatcherRuntime runtime;
    dispatcher::api::DispatcherRuntimeApi api(runtime);

    EXPECT_EQ(&api.runtime(), &runtime);

    const auto& const_api = api;

    EXPECT_EQ(&const_api.runtime(), &runtime);
}