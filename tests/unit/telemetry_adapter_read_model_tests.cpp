#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_adapter_error.hpp>
#include <dispatcher/telemetry/telemetry_adapter_result.hpp>
#include <dispatcher/telemetry/telemetry_adapter_status.hpp>
#include <dispatcher/telemetry/telemetry_read_request.hpp>
#include <dispatcher/telemetry/telemetry_read_result.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
    dispatcher::telemetry::TelemetryValue make_read_model_value(
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

TEST(TelemetryReadRequestTests, DefaultRequestIsEmpty)
{
    const dispatcher::telemetry::TelemetryReadRequest request;

    EXPECT_TRUE(request.empty());
    EXPECT_FALSE(request.single());
    EXPECT_FALSE(request.batch());
    EXPECT_EQ(request.size(), 0);
    EXPECT_FALSE(request.has_source());
    EXPECT_TRUE(request.tag_ids().empty());
}

TEST(TelemetryReadRequestTests, SingleRequestCapturesTagAndSource)
{
    const auto request =
        dispatcher::telemetry::TelemetryReadRequest::single(
            dispatcher::domain::TagId{ "tag-temperature" },
            "modbus-1"
        );

    EXPECT_FALSE(request.empty());
    EXPECT_TRUE(request.single());
    EXPECT_FALSE(request.batch());
    EXPECT_EQ(request.size(), 1);

    EXPECT_TRUE(request.has_source());
    EXPECT_EQ(request.source(), "modbus-1");

    ASSERT_EQ(request.tag_ids().size(), 1);
    EXPECT_EQ(request.tag_ids().front(), dispatcher::domain::TagId{ "tag-temperature" });

    EXPECT_TRUE(
        request.contains(
            dispatcher::domain::TagId{ "tag-temperature" }
        )
    );

    EXPECT_FALSE(
        request.contains(
            dispatcher::domain::TagId{ "tag-pressure" }
        )
    );
}

TEST(TelemetryReadRequestTests, BatchRequestCapturesTags)
{
    const auto request =
        dispatcher::telemetry::TelemetryReadRequest::batch(
            {
                dispatcher::domain::TagId{"tag-temperature"},
                dispatcher::domain::TagId{"tag-pressure"}
            },
            "opcua-1"
        );

    EXPECT_FALSE(request.empty());
    EXPECT_FALSE(request.single());
    EXPECT_TRUE(request.batch());
    EXPECT_EQ(request.size(), 2);

    EXPECT_EQ(request.source(), "opcua-1");

    EXPECT_TRUE(
        request.contains(
            dispatcher::domain::TagId{ "tag-temperature" }
        )
    );

    EXPECT_TRUE(
        request.contains(
            dispatcher::domain::TagId{ "tag-pressure" }
        )
    );
}

TEST(TelemetryReadResultTests, SuccessWithSingleValue)
{
    const auto result =
        dispatcher::telemetry::TelemetryReadResult::success(
            make_read_model_value(
                "tag-temperature",
                dispatcher::telemetry::TagValue(42.0),
                7
            )
        );

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::Success
    );

    EXPECT_FALSE(result.has_error());

    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.single());
    EXPECT_FALSE(result.batch());
    EXPECT_EQ(result.size(), 1);

    EXPECT_EQ(result.value().tag_id(), dispatcher::domain::TagId{ "tag-temperature" });
    EXPECT_EQ(result.value().sequence(), 7);

    EXPECT_THROW(
        (void)result.error(),
        std::logic_error
    );
}

TEST(TelemetryReadResultTests, SuccessWithBatchValues)
{
    std::vector<dispatcher::telemetry::TelemetryValue> values;

    values.push_back(
        make_read_model_value(
            "tag-temperature",
            dispatcher::telemetry::TagValue(42.0),
            1
        )
    );

    values.push_back(
        make_read_model_value(
            "tag-pressure",
            dispatcher::telemetry::TagValue(10.0),
            2
        )
    );

    const auto result =
        dispatcher::telemetry::TelemetryReadResult::success(
            std::move(values)
        );

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.empty());
    EXPECT_FALSE(result.single());
    EXPECT_TRUE(result.batch());
    EXPECT_EQ(result.size(), 2);

    EXPECT_EQ(result.values()[0].tag_id(), dispatcher::domain::TagId{ "tag-temperature" });
    EXPECT_EQ(result.values()[1].tag_id(), dispatcher::domain::TagId{ "tag-pressure" });
}

TEST(TelemetryReadResultTests, EmptySuccessRepresentsNoValuesButOk)
{
    const auto result =
        dispatcher::telemetry::TelemetryReadResult::success(
            std::vector<dispatcher::telemetry::TelemetryValue>{}
        );

    EXPECT_TRUE(result.ok());
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(result.single());
    EXPECT_FALSE(result.batch());
    EXPECT_EQ(result.size(), 0);

    EXPECT_THROW(
        (void)result.value(),
        std::logic_error
    );
}

TEST(TelemetryReadResultTests, FailureContainsAdapterError)
{
    const auto result =
        dispatcher::telemetry::TelemetryReadResult::failure(
            dispatcher::telemetry::TelemetryAdapterStatus::UnknownTag,
            "adapter.read_current",
            "simulator",
            "tag-missing",
            "tag_id",
            "tag id is not configured"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::UnknownTag
    );

    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(result.single());
    EXPECT_FALSE(result.batch());

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().operation(), "adapter.read_current");
    EXPECT_EQ(result.error().adapter_name(), "simulator");
    EXPECT_EQ(result.error().resource(), "tag-missing");
    EXPECT_EQ(result.error().field(), "tag_id");
    EXPECT_EQ(result.error().message(), "tag id is not configured");

    EXPECT_FALSE(result.retryable());
}

TEST(TelemetryReadResultTests, FailureRejectsSuccessStatus)
{
    const auto result =
        dispatcher::telemetry::TelemetryReadResult::failure(
            dispatcher::telemetry::TelemetryAdapterStatus::Success,
            "adapter.read_current"
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

TEST(TelemetryReadResultTests, FromAdapterResultConvertsSuccessToEmptyReadSuccess)
{
    const auto adapter_result =
        dispatcher::telemetry::TelemetryAdapterResult::success();

    const auto result =
        dispatcher::telemetry::TelemetryReadResult::from_adapter_result(
            adapter_result
        );

    EXPECT_TRUE(result.ok());
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(result.has_error());
}

TEST(TelemetryReadResultTests, FromAdapterResultConvertsFailure)
{
    const auto adapter_result =
        dispatcher::telemetry::TelemetryAdapterResult::failure(
            dispatcher::telemetry::TelemetryAdapterStatus::Timeout,
            "adapter.read_current",
            "modbus-1",
            "tag-temperature",
            "timeout",
            "read timed out"
        );

    const auto result =
        dispatcher::telemetry::TelemetryReadResult::from_adapter_result(
            adapter_result
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::Timeout
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().operation(), "adapter.read_current");
    EXPECT_EQ(result.error().adapter_name(), "modbus-1");
    EXPECT_EQ(result.error().resource(), "tag-temperature");
    EXPECT_EQ(result.error().field(), "timeout");
    EXPECT_EQ(result.error().message(), "read timed out");

    EXPECT_TRUE(result.retryable());
}