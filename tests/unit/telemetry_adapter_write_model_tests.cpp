#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_adapter_error.hpp>
#include <dispatcher/telemetry/telemetry_adapter_result.hpp>
#include <dispatcher/telemetry/telemetry_adapter_status.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>
#include <dispatcher/telemetry/telemetry_write_request.hpp>
#include <dispatcher/telemetry/telemetry_write_result.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
    dispatcher::telemetry::TelemetryValue make_write_model_value(
        std::string tag_id,
        dispatcher::telemetry::TagValue value,
        std::uint64_t sequence = 1
    )
    {
        using dispatcher::domain::Quality;
        using dispatcher::domain::TagId;
        using dispatcher::telemetry::TelemetryValue;

        const auto now = TelemetryValue::Clock::now();

        return TelemetryValue(
            TagId{ std::move(tag_id) },
            std::move(value),
            Quality::Good,
            now,
            now,
            sequence
        );
    }
}

TEST(TelemetryWriteRequestTests, DefaultRequestIsEmpty)
{
    const dispatcher::telemetry::TelemetryWriteRequest request;

    EXPECT_TRUE(request.empty());
    EXPECT_FALSE(request.single());
    EXPECT_FALSE(request.batch());
    EXPECT_EQ(request.size(), 0);
    EXPECT_FALSE(request.has_source());
    EXPECT_TRUE(request.values().empty());
}

TEST(TelemetryWriteRequestTests, SingleRequestCapturesValueAndSource)
{
    const auto request =
        dispatcher::telemetry::TelemetryWriteRequest::single(
            make_write_model_value(
                "tag-temperature",
                dispatcher::telemetry::TagValue(42.0),
                7
            ),
            "operator-api"
        );

    EXPECT_FALSE(request.empty());
    EXPECT_TRUE(request.single());
    EXPECT_FALSE(request.batch());
    EXPECT_EQ(request.size(), 1);

    EXPECT_TRUE(request.has_source());
    EXPECT_EQ(request.source(), "operator-api");

    ASSERT_EQ(request.values().size(), 1);
    EXPECT_EQ(
        request.values().front().tag_id(),
        dispatcher::domain::TagId{ "tag-temperature" }
    );
    EXPECT_EQ(request.values().front().sequence(), 7);

    EXPECT_TRUE(
        request.contains_tag(
            dispatcher::domain::TagId{ "tag-temperature" }
        )
    );

    EXPECT_FALSE(
        request.contains_tag(
            dispatcher::domain::TagId{ "tag-pressure" }
        )
    );
}

TEST(TelemetryWriteRequestTests, BatchRequestCapturesValues)
{
    std::vector<dispatcher::telemetry::TelemetryValue> values;

    values.push_back(
        make_write_model_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(42.0),
            1
        )
    );

    values.push_back(
        make_write_model_value(
            "tag-pressure",
            dispatcher::telemetry::TagValue(10.0),
            2
        )
    );

    const auto request =
        dispatcher::telemetry::TelemetryWriteRequest::batch(
            std::move(values),
            "maintenance-tool"
        );

    EXPECT_FALSE(request.empty());
    EXPECT_FALSE(request.single());
    EXPECT_TRUE(request.batch());
    EXPECT_EQ(request.size(), 2);

    EXPECT_TRUE(request.has_source());
    EXPECT_EQ(request.source(), "maintenance-tool");

    EXPECT_TRUE(
        request.contains_tag(
            dispatcher::domain::TagId{ "tag-temperature" }
        )
    );

    EXPECT_TRUE(
        request.contains_tag(
            dispatcher::domain::TagId{ "tag-pressure" }
        )
    );

    EXPECT_FALSE(
        request.contains_tag(
            dispatcher::domain::TagId{ "tag-flow" }
        )
    );
}

TEST(TelemetryWriteResultTests, SuccessWithoutCountIsOk)
{
    const auto result =
        dispatcher::telemetry::TelemetryWriteResult::success();

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::Success
    );

    EXPECT_EQ(result.written_count(), 0);
    EXPECT_FALSE(result.wrote_any());
    EXPECT_TRUE(result.wrote_expected(0));

    EXPECT_FALSE(result.has_error());

    EXPECT_THROW(
        (void)result.error(),
        std::logic_error
    );
}

TEST(TelemetryWriteResultTests, SuccessWithWrittenCountIsOk)
{
    const auto result =
        dispatcher::telemetry::TelemetryWriteResult::success(2);

    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.written_count(), 2);
    EXPECT_TRUE(result.wrote_any());

    EXPECT_TRUE(result.wrote_expected(2));
    EXPECT_FALSE(result.wrote_expected(1));
}

TEST(TelemetryWriteResultTests, FailureContainsAdapterError)
{
    const auto result =
        dispatcher::telemetry::TelemetryWriteResult::failure(
            dispatcher::telemetry::TelemetryAdapterStatus::UnsupportedOperation,
            "adapter.write_current",
            "simulator",
            "tag-temperature",
            "capability",
            "write_current is not supported"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::UnsupportedOperation
    );

    EXPECT_EQ(result.written_count(), 0);
    EXPECT_FALSE(result.wrote_any());

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().operation(), "adapter.write_current");
    EXPECT_EQ(result.error().adapter_name(), "simulator");
    EXPECT_EQ(result.error().resource(), "tag-temperature");
    EXPECT_EQ(result.error().field(), "capability");
    EXPECT_EQ(result.error().message(), "write_current is not supported");

    EXPECT_FALSE(result.retryable());
}

TEST(TelemetryWriteResultTests, FailureCanKeepPartialWrittenCount)
{
    const auto result =
        dispatcher::telemetry::TelemetryWriteResult::failure(
            dispatcher::telemetry::TelemetryAdapterStatus::Timeout,
            "adapter.write_batch",
            "modbus-1",
            "batch",
            "timeout",
            "write timed out after partial success",
            2
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::Timeout
    );

    EXPECT_EQ(result.written_count(), 2);
    EXPECT_TRUE(result.wrote_any());
    EXPECT_TRUE(result.retryable());

    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(result.error().operation(), "adapter.write_batch");
}

TEST(TelemetryWriteResultTests, FailureRejectsSuccessStatus)
{
    const auto result =
        dispatcher::telemetry::TelemetryWriteResult::failure(
            dispatcher::telemetry::TelemetryAdapterStatus::Success,
            "adapter.write_current"
        );

    EXPECT_FALSE(result.ok());

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

TEST(TelemetryWriteResultTests, FromAdapterResultConvertsSuccess)
{
    const auto adapter_result =
        dispatcher::telemetry::TelemetryAdapterResult::success();

    const auto result =
        dispatcher::telemetry::TelemetryWriteResult::from_adapter_result(
            adapter_result,
            3
        );

    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.written_count(), 3);
    EXPECT_TRUE(result.wrote_expected(3));
    EXPECT_FALSE(result.has_error());
}

TEST(TelemetryWriteResultTests, FromAdapterResultConvertsFailure)
{
    const auto adapter_result =
        dispatcher::telemetry::TelemetryAdapterResult::failure(
            dispatcher::telemetry::TelemetryAdapterStatus::IoError,
            "adapter.write_current",
            "opcua-1",
            "tag-temperature",
            "socket",
            "write failed"
        );

    const auto result =
        dispatcher::telemetry::TelemetryWriteResult::from_adapter_result(
            adapter_result,
            0
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::IoError
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().operation(), "adapter.write_current");
    EXPECT_EQ(result.error().adapter_name(), "opcua-1");
    EXPECT_EQ(result.error().resource(), "tag-temperature");
    EXPECT_EQ(result.error().field(), "socket");
    EXPECT_EQ(result.error().message(), "write failed");

    EXPECT_TRUE(result.retryable());
}