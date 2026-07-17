#include <dispatcher/telemetry/telemetry_adapter_error.hpp>
#include <dispatcher/telemetry/telemetry_adapter_result.hpp>
#include <dispatcher/telemetry/telemetry_adapter_status.hpp>

#include <gtest/gtest.h>

#include <stdexcept>

TEST(TelemetryAdapterStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterStatus::Success
        ),
        "success"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterStatus::NotConnected
        ),
        "not_connected"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterStatus::ConnectionFailed
        ),
        "connection_failed"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterStatus::Timeout
        ),
        "timeout"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterStatus::InvalidConfiguration
        ),
        "invalid_configuration"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterStatus::InvalidRequest
        ),
        "invalid_request"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterStatus::UnsupportedOperation
        ),
        "unsupported_operation"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterStatus::UnknownTag
        ),
        "unknown_tag"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterStatus::BadPayload
        ),
        "bad_payload"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterStatus::DecodeError
        ),
        "decode_error"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterStatus::Backpressure
        ),
        "backpressure"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterStatus::IoError
        ),
        "io_error"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterStatus::UnknownError
        ),
        "unknown_error"
    );
}

TEST(TelemetryAdapterStatusTests, SuccessAndFailurePredicatesWork)
{
    EXPECT_TRUE(
        dispatcher::telemetry::is_success(
            dispatcher::telemetry::TelemetryAdapterStatus::Success
        )
    );

    EXPECT_FALSE(
        dispatcher::telemetry::is_failure(
            dispatcher::telemetry::TelemetryAdapterStatus::Success
        )
    );

    EXPECT_FALSE(
        dispatcher::telemetry::is_success(
            dispatcher::telemetry::TelemetryAdapterStatus::Timeout
        )
    );

    EXPECT_TRUE(
        dispatcher::telemetry::is_failure(
            dispatcher::telemetry::TelemetryAdapterStatus::Timeout
        )
    );
}

TEST(TelemetryAdapterStatusTests, RetryableClassifiesTransientFailures)
{
    EXPECT_TRUE(
        dispatcher::telemetry::is_retryable(
            dispatcher::telemetry::TelemetryAdapterStatus::NotConnected
        )
    );

    EXPECT_TRUE(
        dispatcher::telemetry::is_retryable(
            dispatcher::telemetry::TelemetryAdapterStatus::ConnectionFailed
        )
    );

    EXPECT_TRUE(
        dispatcher::telemetry::is_retryable(
            dispatcher::telemetry::TelemetryAdapterStatus::Timeout
        )
    );

    EXPECT_TRUE(
        dispatcher::telemetry::is_retryable(
            dispatcher::telemetry::TelemetryAdapterStatus::Backpressure
        )
    );

    EXPECT_FALSE(
        dispatcher::telemetry::is_retryable(
            dispatcher::telemetry::TelemetryAdapterStatus::InvalidConfiguration
        )
    );

    EXPECT_FALSE(
        dispatcher::telemetry::is_retryable(
            dispatcher::telemetry::TelemetryAdapterStatus::BadPayload
        )
    );

    EXPECT_FALSE(
        dispatcher::telemetry::is_retryable(
            dispatcher::telemetry::TelemetryAdapterStatus::DecodeError
        )
    );
}

TEST(TelemetryAdapterErrorTests, CapturesErrorContext)
{
    const dispatcher::telemetry::TelemetryAdapterError error(
        dispatcher::telemetry::TelemetryAdapterStatus::Timeout,
        "adapter.poll",
        "modbus-1",
        "tcp://127.0.0.1:502",
        "connect_timeout",
        "adapter poll timed out"
    );

    EXPECT_EQ(
        error.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::Timeout
    );

    EXPECT_EQ(error.operation(), "adapter.poll");
    EXPECT_EQ(error.adapter_name(), "modbus-1");
    EXPECT_EQ(error.resource(), "tcp://127.0.0.1:502");
    EXPECT_EQ(error.field(), "connect_timeout");
    EXPECT_EQ(error.message(), "adapter poll timed out");

    EXPECT_TRUE(error.has_operation());
    EXPECT_TRUE(error.has_adapter_name());
    EXPECT_TRUE(error.has_resource());
    EXPECT_TRUE(error.has_field());
    EXPECT_TRUE(error.has_message());

    EXPECT_TRUE(error.retryable());
}

TEST(TelemetryAdapterResultTests, SuccessHasNoError)
{
    const auto result =
        dispatcher::telemetry::TelemetryAdapterResult::success();

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::Success
    );

    EXPECT_FALSE(result.has_error());
    EXPECT_FALSE(result.retryable());

    EXPECT_THROW(
        (void)result.error(),
        std::logic_error
    );
}

TEST(TelemetryAdapterResultTests, FailureContainsError)
{
    const auto result =
        dispatcher::telemetry::TelemetryAdapterResult::failure(
            dispatcher::telemetry::TelemetryAdapterStatus::ConnectionFailed,
            "adapter.connect",
            "opcua-1",
            "opc.tcp://localhost:4840",
            "endpoint",
            "failed to connect"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::ConnectionFailed
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().operation(), "adapter.connect");
    EXPECT_EQ(result.error().adapter_name(), "opcua-1");
    EXPECT_EQ(result.error().resource(), "opc.tcp://localhost:4840");
    EXPECT_EQ(result.error().field(), "endpoint");
    EXPECT_EQ(result.error().message(), "failed to connect");

    EXPECT_TRUE(result.retryable());
    EXPECT_TRUE(result.error().retryable());
}

TEST(TelemetryAdapterResultTests, FailureRejectsSuccessStatus)
{
    const auto result =
        dispatcher::telemetry::TelemetryAdapterResult::failure(
            dispatcher::telemetry::TelemetryAdapterStatus::Success,
            "adapter.fail"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::UnknownError
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(
        result.error().status(),
        dispatcher::telemetry::TelemetryAdapterStatus::UnknownError
    );
}

TEST(TelemetryAdapterResultTests, FailureCanBeCreatedFromError)
{
    dispatcher::telemetry::TelemetryAdapterError error(
        dispatcher::telemetry::TelemetryAdapterStatus::InvalidRequest,
        "adapter.read",
        "simulator",
        "tag-temperature",
        "tag_id",
        "tag id is not configured"
    );

    const auto result =
        dispatcher::telemetry::TelemetryAdapterResult::failure(
            std::move(error)
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::InvalidRequest
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().operation(), "adapter.read");
    EXPECT_EQ(result.error().adapter_name(), "simulator");
    EXPECT_EQ(result.error().resource(), "tag-temperature");
    EXPECT_EQ(result.error().field(), "tag_id");
    EXPECT_EQ(result.error().message(), "tag id is not configured");

    EXPECT_FALSE(result.retryable());
}