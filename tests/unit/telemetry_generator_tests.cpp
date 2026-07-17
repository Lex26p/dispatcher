#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/simulator/telemetry_generator.hpp>

#include <gtest/gtest.h>

TEST(TelemetryGeneratorTests, StoresConfiguredTagCount)
{
    const dispatcher::simulator::TelemetryGenerator generator(10);

    EXPECT_EQ(generator.tag_count(), 10);
    EXPECT_EQ(generator.sequence(), 0);
}

TEST(TelemetryGeneratorTests, GeneratesBatchWithConfiguredTagCount)
{
    dispatcher::simulator::TelemetryGenerator generator(3);

    const auto batch = generator.next_batch();

    ASSERT_EQ(batch.size(), 3);

    EXPECT_EQ(batch[0].tag_id().value(), "tag-1");
    EXPECT_EQ(batch[1].tag_id().value(), "tag-2");
    EXPECT_EQ(batch[2].tag_id().value(), "tag-3");
}

TEST(TelemetryGeneratorTests, IncrementsSequenceForEachBatch)
{
    dispatcher::simulator::TelemetryGenerator generator(2);

    const auto first_batch = generator.next_batch();
    const auto second_batch = generator.next_batch();

    ASSERT_EQ(first_batch.size(), 2);
    ASSERT_EQ(second_batch.size(), 2);

    EXPECT_EQ(first_batch[0].sequence(), 1);
    EXPECT_EQ(first_batch[1].sequence(), 1);

    EXPECT_EQ(second_batch[0].sequence(), 2);
    EXPECT_EQ(second_batch[1].sequence(), 2);

    EXPECT_EQ(generator.sequence(), 2);
}

TEST(TelemetryGeneratorTests, GeneratesFloat64GoodValues)
{
    dispatcher::simulator::TelemetryGenerator generator(1);

    const auto batch = generator.next_batch();

    ASSERT_EQ(batch.size(), 1);

    EXPECT_EQ(batch[0].value().type(), dispatcher::domain::DataType::Float64);
    EXPECT_EQ(batch[0].quality(), dispatcher::domain::Quality::Good);
}