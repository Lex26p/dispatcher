#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <string>

TEST(TagValueTests, DetectsBooleanType)
{
    const dispatcher::telemetry::TagValue value(true);

    EXPECT_EQ(value.type(), dispatcher::domain::DataType::Boolean);
    EXPECT_TRUE(value.as<bool>());
}

TEST(TagValueTests, DetectsInt32Type)
{
    const dispatcher::telemetry::TagValue value(std::int32_t{ 42 });

    EXPECT_EQ(value.type(), dispatcher::domain::DataType::Int32);
    EXPECT_EQ(value.as<std::int32_t>(), 42);
}

TEST(TagValueTests, DetectsInt64Type)
{
    const dispatcher::telemetry::TagValue value(std::int64_t{ 123456789 });

    EXPECT_EQ(value.type(), dispatcher::domain::DataType::Int64);
    EXPECT_EQ(value.as<std::int64_t>(), 123456789);
}

TEST(TagValueTests, DetectsFloat32Type)
{
    const dispatcher::telemetry::TagValue value(12.5f);

    EXPECT_EQ(value.type(), dispatcher::domain::DataType::Float32);
    EXPECT_FLOAT_EQ(value.as<float>(), 12.5f);
}

TEST(TagValueTests, DetectsFloat64Type)
{
    const dispatcher::telemetry::TagValue value(25.75);

    EXPECT_EQ(value.type(), dispatcher::domain::DataType::Float64);
    EXPECT_DOUBLE_EQ(value.as<double>(), 25.75);
}

TEST(TagValueTests, DetectsStringType)
{
    const dispatcher::telemetry::TagValue value("running");

    EXPECT_EQ(value.type(), dispatcher::domain::DataType::String);
    EXPECT_EQ(value.as<std::string>(), "running");
}

TEST(TelemetryValueTests, StoresTelemetryMetadata)
{
    using dispatcher::domain::Quality;
    using dispatcher::domain::TagId;
    using dispatcher::telemetry::TagValue;
    using dispatcher::telemetry::TelemetryValue;

    const auto source_timestamp = TelemetryValue::Clock::now();
    const auto ingest_timestamp = source_timestamp + std::chrono::milliseconds(5);

    const TelemetryValue telemetry_value(
        TagId{ "tag-temperature-1" },
        TagValue(21.5),
        Quality::Good,
        source_timestamp,
        ingest_timestamp,
        1001
    );

    EXPECT_EQ(telemetry_value.tag_id().value(), "tag-temperature-1");
    EXPECT_EQ(telemetry_value.value().type(), dispatcher::domain::DataType::Float64);
    EXPECT_DOUBLE_EQ(telemetry_value.value().as<double>(), 21.5);
    EXPECT_EQ(telemetry_value.quality(), Quality::Good);
    EXPECT_EQ(telemetry_value.source_timestamp(), source_timestamp);
    EXPECT_EQ(telemetry_value.ingest_timestamp(), ingest_timestamp);
    EXPECT_EQ(telemetry_value.sequence(), 1001);
}