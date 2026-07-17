#include <dispatcher/core/telemetry_ingestor.hpp>
#include <dispatcher/domain/configuration_snapshot_builder.hpp>
#include <dispatcher/domain/device_definition_builder.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/domain/tag_definition_builder.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <string>

namespace
{
    dispatcher::domain::DeviceDefinition make_device()
    {
        using namespace dispatcher::domain;

        return DeviceDefinitionBuilder{}
            .organization_id(OrganizationId{ "org-1" })
            .site_id(SiteId{ "site-1" })
            .area_id(AreaId{ "area-1" })
            .device_id(DeviceId{ "device-1" })
            .local_name("plc-1")
            .display_name("Main PLC")
            .build();
    }

    dispatcher::domain::TagDefinition make_tag(
        std::string tag_id,
        std::string local_name,
        dispatcher::domain::DataType data_type,
        bool enabled = true
    )
    {
        using namespace dispatcher::domain;

        return TagDefinitionBuilder{}
            .organization_id(OrganizationId{ "org-1" })
            .site_id(SiteId{ "site-1" })
            .area_id(AreaId{ "area-1" })
            .device_id(DeviceId{ "device-1" })
            .tag_id(TagId{ std::move(tag_id) })
            .local_name(std::move(local_name))
            .display_name("Tag")
            .data_type(data_type)
            .enabled(enabled)
            .build();
    }

    dispatcher::domain::ConfigurationSnapshot make_snapshot_with_single_tag(
        dispatcher::domain::DataType data_type = dispatcher::domain::DataType::Float64,
        bool enabled = true
    )
    {
        dispatcher::domain::ConfigurationSnapshotBuilder builder;

        EXPECT_TRUE(builder.add_device(make_device()).valid());
        EXPECT_TRUE(
            builder.add_tag(
                make_tag(
                    "tag-temperature",
                    "temperature",
                    data_type,
                    enabled
                )
            ).valid()
        );

        return builder
            .config_version(1)
            .published()
            .build();
    }

    dispatcher::telemetry::TelemetryValue make_telemetry_value(
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

    dispatcher::telemetry::TelemetryValue make_telemetry_value_with_quality(
        std::string tag_id,
        dispatcher::telemetry::TagValue value,
        dispatcher::domain::Quality quality,
        std::uint64_t sequence = 1
    )
    {
        using dispatcher::domain::TagId;
        using dispatcher::telemetry::TelemetryValue;

        const auto now = TelemetryValue::Clock::now();

        return TelemetryValue(
            TagId{ std::move(tag_id) },
            std::move(value),
            quality,
            now,
            now,
            sequence
        );
    }
}

TEST(TelemetryIngestStatusTests, ToStringReturnsExpectedValues)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::to_string;

    EXPECT_EQ(to_string(TelemetryIngestStatus::Accepted), "accepted");
    EXPECT_EQ(to_string(TelemetryIngestStatus::AcceptedNoChange), "accepted_no_change");
    EXPECT_EQ(to_string(TelemetryIngestStatus::UnknownTag), "unknown_tag");
    EXPECT_EQ(to_string(TelemetryIngestStatus::DisabledTag), "disabled_tag");
    EXPECT_EQ(to_string(TelemetryIngestStatus::DataTypeMismatch), "data_type_mismatch");
    EXPECT_EQ(to_string(TelemetryIngestStatus::StaleSequence), "stale_sequence");
    EXPECT_EQ(to_string(TelemetryIngestStatus::Accepted), "accepted");
    EXPECT_EQ(to_string(TelemetryIngestStatus::BadQuality), "bad_quality");
}

TEST(TelemetryIngestResultTests, AcceptedResultReportsAccepted)
{
    const dispatcher::core::TelemetryIngestResult result(
        dispatcher::core::TelemetryIngestStatus::Accepted
    );

    EXPECT_TRUE(result.accepted());
    EXPECT_FALSE(result.rejected());
}

TEST(TelemetryIngestorTests, AcceptsKnownEnabledTagWithMatchingType)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    const auto result = ingestor.ingest(
        make_telemetry_value(
            "tag-temperature",
            TagValue(21.5)
        )
    );

    EXPECT_TRUE(result.accepted());
    EXPECT_EQ(result.status(), TelemetryIngestStatus::Accepted);
    EXPECT_EQ(ingestor.current_state().size(), 1);

    const auto stored = ingestor.current_state().find(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_TRUE(stored.has_value());
    EXPECT_DOUBLE_EQ(stored->value().as<double>(), 21.5);
}

TEST(TelemetryIngestorTests, RejectsUnknownTag)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    const auto result = ingestor.ingest(
        make_telemetry_value(
            "unknown-tag",
            TagValue(21.5)
        )
    );

    EXPECT_TRUE(result.rejected());
    EXPECT_EQ(result.status(), TelemetryIngestStatus::UnknownTag);
    EXPECT_EQ(ingestor.current_state().size(), 0);
}

TEST(TelemetryIngestorTests, RejectsDisabledTag)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag(
            dispatcher::domain::DataType::Float64,
            false
        )
    );

    const auto result = ingestor.ingest(
        make_telemetry_value(
            "tag-temperature",
            TagValue(21.5)
        )
    );

    EXPECT_TRUE(result.rejected());
    EXPECT_EQ(result.status(), TelemetryIngestStatus::DisabledTag);
    EXPECT_EQ(ingestor.current_state().size(), 0);
}

TEST(TelemetryIngestorTests, RejectsDataTypeMismatch)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag(
            dispatcher::domain::DataType::Float64,
            true
        )
    );

    const auto result = ingestor.ingest(
        make_telemetry_value(
            "tag-temperature",
            TagValue("not-a-double")
        )
    );

    EXPECT_TRUE(result.rejected());
    EXPECT_EQ(result.status(), TelemetryIngestStatus::DataTypeMismatch);
    EXPECT_EQ(ingestor.current_state().size(), 0);
}

TEST(TelemetryIngestorTests, ReplacesCurrentStateForRepeatedAcceptedValues)
{
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    EXPECT_TRUE(
        ingestor.ingest(
            make_telemetry_value(
                "tag-temperature",
                TagValue(10.0),
                1
            )
        ).accepted()
    );

    EXPECT_TRUE(
        ingestor.ingest(
            make_telemetry_value(
                "tag-temperature",
                TagValue(20.0),
                2
            )
        ).accepted()
    );

    EXPECT_EQ(ingestor.current_state().size(), 1);

    const auto stored = ingestor.current_state().find(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->sequence(), 2);
    EXPECT_DOUBLE_EQ(stored->value().as<double>(), 20.0);
}

TEST(TelemetryIngestorStatisticsTests, InitialStatisticsAreZero)
{
    dispatcher::core::TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    EXPECT_EQ(ingestor.statistics().accepted_count(), 0);
    EXPECT_EQ(ingestor.statistics().rejected_count(), 0);
    EXPECT_EQ(ingestor.statistics().unknown_tag_count(), 0);
    EXPECT_EQ(ingestor.statistics().disabled_tag_count(), 0);
    EXPECT_EQ(ingestor.statistics().data_type_mismatch_count(), 0);
    EXPECT_EQ(ingestor.statistics().total_count(), 0);
    EXPECT_EQ(ingestor.statistics().future_source_timestamp_count(), 0);
    EXPECT_EQ(ingestor.statistics().bad_quality_count(), 0);
}

TEST(TelemetryIngestorStatisticsTests, AcceptedTelemetryIncrementsAcceptedCount)
{
    using dispatcher::telemetry::TagValue;

    dispatcher::core::TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    ASSERT_TRUE(
        ingestor.ingest(
            make_telemetry_value(
                "tag-temperature",
                TagValue(21.5)
            )
        ).accepted()
    );

    EXPECT_EQ(ingestor.statistics().accepted_count(), 1);
    EXPECT_EQ(ingestor.statistics().rejected_count(), 0);
    EXPECT_EQ(ingestor.statistics().total_count(), 1);
}

TEST(TelemetryIngestorStatisticsTests, UnknownTagIncrementsRejectedAndUnknownTagCounts)
{
    using dispatcher::telemetry::TagValue;

    dispatcher::core::TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    ASSERT_TRUE(
        ingestor.ingest(
            make_telemetry_value(
                "unknown-tag",
                TagValue(21.5)
            )
        ).rejected()
    );

    EXPECT_EQ(ingestor.statistics().accepted_count(), 0);
    EXPECT_EQ(ingestor.statistics().rejected_count(), 1);
    EXPECT_EQ(ingestor.statistics().unknown_tag_count(), 1);
    EXPECT_EQ(ingestor.statistics().total_count(), 1);
}

TEST(TelemetryIngestorStatisticsTests, DisabledTagIncrementsRejectedAndDisabledTagCounts)
{
    using dispatcher::telemetry::TagValue;

    dispatcher::core::TelemetryIngestor ingestor(
        make_snapshot_with_single_tag(
            dispatcher::domain::DataType::Float64,
            false
        )
    );

    ASSERT_TRUE(
        ingestor.ingest(
            make_telemetry_value(
                "tag-temperature",
                TagValue(21.5)
            )
        ).rejected()
    );

    EXPECT_EQ(ingestor.statistics().accepted_count(), 0);
    EXPECT_EQ(ingestor.statistics().rejected_count(), 1);
    EXPECT_EQ(ingestor.statistics().disabled_tag_count(), 1);
    EXPECT_EQ(ingestor.statistics().total_count(), 1);
}

TEST(TelemetryIngestorStatisticsTests, DataTypeMismatchIncrementsRejectedAndMismatchCounts)
{
    using dispatcher::telemetry::TagValue;

    dispatcher::core::TelemetryIngestor ingestor(
        make_snapshot_with_single_tag(
            dispatcher::domain::DataType::Float64,
            true
        )
    );

    ASSERT_TRUE(
        ingestor.ingest(
            make_telemetry_value(
                "tag-temperature",
                TagValue("not-a-double")
            )
        ).rejected()
    );

    EXPECT_EQ(ingestor.statistics().accepted_count(), 0);
    EXPECT_EQ(ingestor.statistics().rejected_count(), 1);
    EXPECT_EQ(ingestor.statistics().data_type_mismatch_count(), 1);
    EXPECT_EQ(ingestor.statistics().total_count(), 1);
}

TEST(TelemetryIngestorStatisticsTests, ResetStatisticsClearsCounters)
{
    using dispatcher::telemetry::TagValue;

    dispatcher::core::TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    EXPECT_TRUE(
        ingestor.ingest(
            make_telemetry_value(
                "tag-temperature",
                TagValue(21.5)
            )
        ).accepted()
    );

    EXPECT_TRUE(
        ingestor.ingest(
            make_telemetry_value(
                "unknown-tag",
                TagValue(21.5)
            )
        ).rejected()
    );

    EXPECT_EQ(ingestor.statistics().total_count(), 2);

    ingestor.reset_statistics();

    EXPECT_EQ(ingestor.statistics().accepted_count(), 0);
    EXPECT_EQ(ingestor.statistics().rejected_count(), 0);
    EXPECT_EQ(ingestor.statistics().unknown_tag_count(), 0);
    EXPECT_EQ(ingestor.statistics().disabled_tag_count(), 0);
    EXPECT_EQ(ingestor.statistics().data_type_mismatch_count(), 0);
    EXPECT_EQ(ingestor.statistics().total_count(), 0);
    EXPECT_EQ(ingestor.statistics().future_source_timestamp_count(), 0);
    EXPECT_EQ(ingestor.statistics().bad_quality_count(), 0);
}

TEST(TelemetryIngestBatchResultTests, EmptyBatchResultReportsEmpty)
{
    const dispatcher::core::TelemetryIngestBatchResult result;

    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(result.all_accepted());
    EXPECT_FALSE(result.has_rejections());
    EXPECT_EQ(result.total_count(), 0);
}

TEST(TelemetryIngestBatchResultTests, RecordsAcceptedStatus)
{
    dispatcher::core::TelemetryIngestBatchResult result;

    result.record(dispatcher::core::TelemetryIngestStatus::Accepted);

    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.all_accepted());
    EXPECT_FALSE(result.has_rejections());
    EXPECT_EQ(result.total_count(), 1);
    EXPECT_EQ(result.accepted_count(), 1);
    EXPECT_EQ(result.rejected_count(), 0);
}

TEST(TelemetryIngestBatchResultTests, RecordsRejectedStatuses)
{
    dispatcher::core::TelemetryIngestBatchResult result;

    result.record(dispatcher::core::TelemetryIngestStatus::UnknownTag);
    result.record(dispatcher::core::TelemetryIngestStatus::DisabledTag);
    result.record(dispatcher::core::TelemetryIngestStatus::DataTypeMismatch);

    EXPECT_FALSE(result.empty());
    EXPECT_FALSE(result.all_accepted());
    EXPECT_TRUE(result.has_rejections());

    EXPECT_EQ(result.total_count(), 3);
    EXPECT_EQ(result.accepted_count(), 0);
    EXPECT_EQ(result.rejected_count(), 3);
    EXPECT_EQ(result.unknown_tag_count(), 1);
    EXPECT_EQ(result.disabled_tag_count(), 1);
    EXPECT_EQ(result.data_type_mismatch_count(), 1);
}

TEST(TelemetryIngestorBatchTests, AcceptsBatchWithKnownTags)
{
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    std::vector<dispatcher::telemetry::TelemetryValue> values;

    values.push_back(
        make_telemetry_value(
            "tag-temperature",
            TagValue(10.0),
            1
        )
    );

    values.push_back(
        make_telemetry_value(
            "tag-temperature",
            TagValue(20.0),
            2
        )
    );

    const auto result = ingestor.ingest_batch(std::move(values));

    EXPECT_EQ(result.total_count(), 2);
    EXPECT_EQ(result.accepted_count(), 2);
    EXPECT_EQ(result.rejected_count(), 0);
    EXPECT_TRUE(result.all_accepted());
    EXPECT_FALSE(result.has_rejections());

    EXPECT_EQ(ingestor.current_state().size(), 1);
    EXPECT_EQ(ingestor.statistics().accepted_count(), 2);
    EXPECT_EQ(ingestor.statistics().total_count(), 2);

    const auto stored = ingestor.current_state().find(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->sequence(), 2);
    EXPECT_DOUBLE_EQ(stored->value().as<double>(), 20.0);
}

TEST(TelemetryIngestorBatchTests, RecordsMixedBatchResults)
{
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag(
            dispatcher::domain::DataType::Float64,
            true
        )
    );

    std::vector<dispatcher::telemetry::TelemetryValue> values;

    values.push_back(
        make_telemetry_value(
            "tag-temperature",
            TagValue(10.0),
            1
        )
    );

    values.push_back(
        make_telemetry_value(
            "unknown-tag",
            TagValue(20.0),
            2
        )
    );

    values.push_back(
        make_telemetry_value(
            "tag-temperature",
            TagValue("bad-type"),
            3
        )
    );

    const auto result = ingestor.ingest_batch(std::move(values));

    EXPECT_EQ(result.total_count(), 3);
    EXPECT_EQ(result.accepted_count(), 1);
    EXPECT_EQ(result.rejected_count(), 2);
    EXPECT_EQ(result.unknown_tag_count(), 1);
    EXPECT_EQ(result.data_type_mismatch_count(), 1);
    EXPECT_TRUE(result.has_rejections());
    EXPECT_FALSE(result.all_accepted());

    EXPECT_EQ(ingestor.current_state().size(), 1);
    EXPECT_EQ(ingestor.statistics().accepted_count(), 1);
    EXPECT_EQ(ingestor.statistics().rejected_count(), 2);
    EXPECT_EQ(ingestor.statistics().unknown_tag_count(), 1);
    EXPECT_EQ(ingestor.statistics().data_type_mismatch_count(), 1);
    EXPECT_EQ(ingestor.statistics().total_count(), 3);
}

TEST(TelemetryIngestorBatchTests, EmptyBatchDoesNothing)
{
    dispatcher::core::TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    std::vector<dispatcher::telemetry::TelemetryValue> values;

    const auto result = ingestor.ingest_batch(std::move(values));

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.total_count(), 0);
    EXPECT_EQ(ingestor.current_state().size(), 0);
    EXPECT_EQ(ingestor.statistics().total_count(), 0);
}

TEST(TelemetryIngestResultTests, AcceptedNoChangeIsAcceptedButNotStored)
{
    const dispatcher::core::TelemetryIngestResult result(
        dispatcher::core::TelemetryIngestStatus::AcceptedNoChange
    );

    EXPECT_TRUE(result.accepted());
    EXPECT_FALSE(result.stored());
    EXPECT_TRUE(result.no_change());
    EXPECT_FALSE(result.rejected());
}

TEST(TelemetryIngestorDeadbandTests, FirstValueIsStoredEvenWithDeadband)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag(
            dispatcher::domain::DataType::Float64,
            true
        )
    );

    const auto result = ingestor.ingest(
        make_telemetry_value(
            "tag-temperature",
            TagValue(10.0),
            1
        )
    );

    EXPECT_EQ(result.status(), TelemetryIngestStatus::Accepted);
    EXPECT_TRUE(result.stored());
    EXPECT_EQ(ingestor.current_state().size(), 1);
    EXPECT_EQ(ingestor.statistics().stored_count(), 1);
    EXPECT_EQ(ingestor.statistics().accepted_no_change_count(), 0);
}

TEST(TelemetryIngestorDeadbandTests, SmallFloat64ChangeIsAcceptedButNotStored)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    ASSERT_TRUE(builder.add_device(make_device()).valid());
    ASSERT_TRUE(
        builder.add_tag(
            dispatcher::domain::TagDefinitionBuilder{}
            .organization_id(dispatcher::domain::OrganizationId{ "org-1" })
            .site_id(dispatcher::domain::SiteId{ "site-1" })
            .area_id(dispatcher::domain::AreaId{ "area-1" })
            .device_id(dispatcher::domain::DeviceId{ "device-1" })
            .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
            .local_name("temperature")
            .data_type(dispatcher::domain::DataType::Float64)
            .deadband(dispatcher::domain::Deadband{ 0.5 })
            .build()
        ).valid()
    );

    TelemetryIngestor ingestor(
        builder.published().build()
    );

    EXPECT_EQ(
        ingestor.ingest(
            make_telemetry_value(
                "tag-temperature",
                TagValue(10.0),
                1
            )
        ).status(),
        TelemetryIngestStatus::Accepted
    );

    const auto second_result = ingestor.ingest(
        make_telemetry_value(
            "tag-temperature",
            TagValue(10.2),
            2
        )
    );

    EXPECT_EQ(second_result.status(), TelemetryIngestStatus::AcceptedNoChange);
    EXPECT_TRUE(second_result.accepted());
    EXPECT_FALSE(second_result.stored());

    EXPECT_EQ(ingestor.current_state().size(), 1);

    const auto stored = ingestor.current_state().find(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->sequence(), 1);
    EXPECT_DOUBLE_EQ(stored->value().as<double>(), 10.0);

    EXPECT_EQ(ingestor.statistics().accepted_count(), 2);
    EXPECT_EQ(ingestor.statistics().stored_count(), 1);
    EXPECT_EQ(ingestor.statistics().accepted_no_change_count(), 1);
    EXPECT_EQ(ingestor.statistics().rejected_count(), 0);
}

TEST(TelemetryIngestorDeadbandTests, LargeFloat64ChangeIsStored)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    ASSERT_TRUE(builder.add_device(make_device()).valid());
    ASSERT_TRUE(
        builder.add_tag(
            dispatcher::domain::TagDefinitionBuilder{}
            .organization_id(dispatcher::domain::OrganizationId{ "org-1" })
            .site_id(dispatcher::domain::SiteId{ "site-1" })
            .area_id(dispatcher::domain::AreaId{ "area-1" })
            .device_id(dispatcher::domain::DeviceId{ "device-1" })
            .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
            .local_name("temperature")
            .data_type(dispatcher::domain::DataType::Float64)
            .deadband(dispatcher::domain::Deadband{ 0.5 })
            .build()
        ).valid()
    );

    TelemetryIngestor ingestor(
        builder.published().build()
    );

    EXPECT_EQ(
        ingestor.ingest(
            make_telemetry_value(
                "tag-temperature",
                TagValue(10.0),
                1
            )
        ).status(),
        TelemetryIngestStatus::Accepted
    );

    const auto second_result = ingestor.ingest(
        make_telemetry_value(
            "tag-temperature",
            TagValue(10.5),
            2
        )
    );

    EXPECT_EQ(second_result.status(), TelemetryIngestStatus::Accepted);
    EXPECT_TRUE(second_result.stored());

    EXPECT_EQ(ingestor.current_state().size(), 1);

    const auto stored = ingestor.current_state().find(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->sequence(), 2);
    EXPECT_DOUBLE_EQ(stored->value().as<double>(), 10.5);

    EXPECT_EQ(ingestor.statistics().accepted_count(), 2);
    EXPECT_EQ(ingestor.statistics().stored_count(), 2);
    EXPECT_EQ(ingestor.statistics().accepted_no_change_count(), 0);
}

TEST(TelemetryIngestBatchResultTests, RecordsAcceptedNoChangeStatus)
{
    dispatcher::core::TelemetryIngestBatchResult result;

    result.record(dispatcher::core::TelemetryIngestStatus::AcceptedNoChange);

    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.all_accepted());
    EXPECT_FALSE(result.has_rejections());

    EXPECT_EQ(result.total_count(), 1);
    EXPECT_EQ(result.accepted_count(), 1);
    EXPECT_EQ(result.stored_count(), 0);
    EXPECT_EQ(result.accepted_no_change_count(), 1);
    EXPECT_EQ(result.rejected_count(), 0);
}

TEST(TelemetryIngestorSequenceTests, LastSequenceIsEmptyBeforeAnyAcceptedValue)
{
    dispatcher::core::TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    const auto last_sequence = ingestor.last_sequence(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    EXPECT_FALSE(last_sequence.has_value());
}

TEST(TelemetryIngestorSequenceTests, AcceptedStoredValueUpdatesLastSequence)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    const auto result = ingestor.ingest(
        make_telemetry_value(
            "tag-temperature",
            TagValue(10.0),
            10
        )
    );

    ASSERT_EQ(result.status(), TelemetryIngestStatus::Accepted);

    const auto last_sequence = ingestor.last_sequence(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_TRUE(last_sequence.has_value());
    EXPECT_EQ(last_sequence.value(), 10);
}

TEST(TelemetryIngestorSequenceTests, RejectsLowerSequenceForSameTag)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    ASSERT_EQ(
        ingestor.ingest(
            make_telemetry_value(
                "tag-temperature",
                TagValue(10.0),
                10
            )
        ).status(),
        TelemetryIngestStatus::Accepted
    );

    const auto result = ingestor.ingest(
        make_telemetry_value(
            "tag-temperature",
            TagValue(20.0),
            9
        )
    );

    EXPECT_EQ(result.status(), TelemetryIngestStatus::StaleSequence);
    EXPECT_TRUE(result.rejected());

    EXPECT_EQ(ingestor.statistics().stale_sequence_count(), 1);
    EXPECT_EQ(ingestor.statistics().rejected_count(), 1);

    const auto stored = ingestor.current_state().find(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->sequence(), 10);
    EXPECT_DOUBLE_EQ(stored->value().as<double>(), 10.0);
}

TEST(TelemetryIngestorSequenceTests, RejectsEqualSequenceForSameTag)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    ASSERT_EQ(
        ingestor.ingest(
            make_telemetry_value(
                "tag-temperature",
                TagValue(10.0),
                10
            )
        ).status(),
        TelemetryIngestStatus::Accepted
    );

    const auto result = ingestor.ingest(
        make_telemetry_value(
            "tag-temperature",
            TagValue(20.0),
            10
        )
    );

    EXPECT_EQ(result.status(), TelemetryIngestStatus::StaleSequence);
    EXPECT_TRUE(result.rejected());

    EXPECT_EQ(ingestor.statistics().stale_sequence_count(), 1);
    EXPECT_EQ(ingestor.statistics().rejected_count(), 1);

    const auto last_sequence = ingestor.last_sequence(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_TRUE(last_sequence.has_value());
    EXPECT_EQ(last_sequence.value(), 10);
}

TEST(TelemetryIngestorSequenceTests, HigherSequenceIsAccepted)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    ASSERT_EQ(
        ingestor.ingest(
            make_telemetry_value(
                "tag-temperature",
                TagValue(10.0),
                10
            )
        ).status(),
        TelemetryIngestStatus::Accepted
    );

    const auto result = ingestor.ingest(
        make_telemetry_value(
            "tag-temperature",
            TagValue(20.0),
            11
        )
    );

    EXPECT_EQ(result.status(), TelemetryIngestStatus::Accepted);
    EXPECT_TRUE(result.stored());

    const auto last_sequence = ingestor.last_sequence(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_TRUE(last_sequence.has_value());
    EXPECT_EQ(last_sequence.value(), 11);

    const auto stored = ingestor.current_state().find(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->sequence(), 11);
    EXPECT_DOUBLE_EQ(stored->value().as<double>(), 20.0);
}

TEST(TelemetryIngestorSequenceTests, AcceptedNoChangeAlsoUpdatesLastSequence)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    ASSERT_TRUE(builder.add_device(make_device()).valid());
    ASSERT_TRUE(
        builder.add_tag(
            dispatcher::domain::TagDefinitionBuilder{}
            .organization_id(dispatcher::domain::OrganizationId{ "org-1" })
            .site_id(dispatcher::domain::SiteId{ "site-1" })
            .area_id(dispatcher::domain::AreaId{ "area-1" })
            .device_id(dispatcher::domain::DeviceId{ "device-1" })
            .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
            .local_name("temperature")
            .data_type(dispatcher::domain::DataType::Float64)
            .deadband(dispatcher::domain::Deadband{ 0.5 })
            .build()
        ).valid()
    );

    TelemetryIngestor ingestor(
        builder.published().build()
    );

    ASSERT_EQ(
        ingestor.ingest(
            make_telemetry_value(
                "tag-temperature",
                TagValue(10.0),
                10
            )
        ).status(),
        TelemetryIngestStatus::Accepted
    );

    ASSERT_EQ(
        ingestor.ingest(
            make_telemetry_value(
                "tag-temperature",
                TagValue(10.1),
                11
            )
        ).status(),
        TelemetryIngestStatus::AcceptedNoChange
    );

    const auto last_sequence = ingestor.last_sequence(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_TRUE(last_sequence.has_value());
    EXPECT_EQ(last_sequence.value(), 11);

    const auto stale_result = ingestor.ingest(
        make_telemetry_value(
            "tag-temperature",
            TagValue(20.0),
            10
        )
    );

    EXPECT_EQ(stale_result.status(), TelemetryIngestStatus::StaleSequence);

    const auto stored = ingestor.current_state().find(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->sequence(), 10);
    EXPECT_DOUBLE_EQ(stored->value().as<double>(), 10.0);
}

TEST(TelemetryIngestBatchResultTests, RecordsStaleSequenceStatus)
{
    dispatcher::core::TelemetryIngestBatchResult result;

    result.record(dispatcher::core::TelemetryIngestStatus::StaleSequence);

    EXPECT_FALSE(result.empty());
    EXPECT_FALSE(result.all_accepted());
    EXPECT_TRUE(result.has_rejections());

    EXPECT_EQ(result.total_count(), 1);
    EXPECT_EQ(result.accepted_count(), 0);
    EXPECT_EQ(result.rejected_count(), 1);
    EXPECT_EQ(result.stale_sequence_count(), 1);
}

TEST(TelemetryIngestorTimestampTests, DefaultFutureSourceTimestampSkewIsFiveSeconds)
{
    dispatcher::core::TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    EXPECT_EQ(
        ingestor.max_future_source_timestamp_skew(),
        std::chrono::milliseconds{ 5000 }
    );
}

TEST(TelemetryIngestorTimestampTests, RejectsSourceTimestampTooFarInFuture)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::domain::Quality;
    using dispatcher::domain::TagId;
    using dispatcher::telemetry::TagValue;
    using dispatcher::telemetry::TelemetryValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag(),
        std::chrono::milliseconds{ 1000 }
    );

    const auto now = TelemetryValue::Clock::now();

    const auto result = ingestor.ingest(
        TelemetryValue(
            TagId{ "tag-temperature" },
            TagValue(10.0),
            Quality::Good,
            now + std::chrono::seconds{ 10 },
            now,
            1
        )
    );

    EXPECT_EQ(result.status(), TelemetryIngestStatus::FutureSourceTimestamp);
    EXPECT_TRUE(result.rejected());

    EXPECT_EQ(ingestor.current_state().size(), 0);
    EXPECT_EQ(ingestor.statistics().future_source_timestamp_count(), 1);
    EXPECT_EQ(ingestor.statistics().rejected_count(), 1);
    EXPECT_EQ(ingestor.statistics().total_count(), 1);

    const auto last_sequence = ingestor.last_sequence(
        TagId{ "tag-temperature" }
    );

    EXPECT_FALSE(last_sequence.has_value());
}

TEST(TelemetryIngestorTimestampTests, AcceptsSourceTimestampWithinAllowedFutureSkew)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::domain::Quality;
    using dispatcher::domain::TagId;
    using dispatcher::telemetry::TagValue;
    using dispatcher::telemetry::TelemetryValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag(),
        std::chrono::milliseconds{ 10000 }
    );

    const auto now = TelemetryValue::Clock::now();

    const auto result = ingestor.ingest(
        TelemetryValue(
            TagId{ "tag-temperature" },
            TagValue(10.0),
            Quality::Good,
            now + std::chrono::seconds{ 1 },
            now,
            1
        )
    );

    EXPECT_EQ(result.status(), TelemetryIngestStatus::Accepted);
    EXPECT_TRUE(result.accepted());
    EXPECT_TRUE(result.stored());

    EXPECT_EQ(ingestor.current_state().size(), 1);
    EXPECT_EQ(ingestor.statistics().future_source_timestamp_count(), 0);
    EXPECT_EQ(ingestor.statistics().accepted_count(), 1);
}

TEST(TelemetryIngestorTimestampTests, BatchResultRecordsFutureSourceTimestamp)
{
    dispatcher::core::TelemetryIngestBatchResult result;

    result.record(dispatcher::core::TelemetryIngestStatus::FutureSourceTimestamp);

    EXPECT_FALSE(result.empty());
    EXPECT_FALSE(result.all_accepted());
    EXPECT_TRUE(result.has_rejections());

    EXPECT_EQ(result.total_count(), 1);
    EXPECT_EQ(result.accepted_count(), 0);
    EXPECT_EQ(result.rejected_count(), 1);
    EXPECT_EQ(result.future_source_timestamp_count(), 1);
}

TEST(TelemetryIngestorQualityTests, RejectsBadQuality)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::domain::Quality;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    const auto result = ingestor.ingest(
        make_telemetry_value_with_quality(
            "tag-temperature",
            TagValue(10.0),
            Quality::Bad,
            1
        )
    );

    EXPECT_EQ(result.status(), TelemetryIngestStatus::BadQuality);
    EXPECT_TRUE(result.rejected());

    EXPECT_EQ(ingestor.current_state().size(), 0);
    EXPECT_EQ(ingestor.statistics().bad_quality_count(), 1);
    EXPECT_EQ(ingestor.statistics().rejected_count(), 1);
    EXPECT_EQ(ingestor.statistics().total_count(), 1);

    EXPECT_FALSE(
        ingestor.last_sequence(
            dispatcher::domain::TagId{ "tag-temperature" }
        ).has_value()
    );
}

TEST(TelemetryIngestorQualityTests, RejectsTimeoutQuality)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::domain::Quality;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    const auto result = ingestor.ingest(
        make_telemetry_value_with_quality(
            "tag-temperature",
            TagValue(10.0),
            Quality::Timeout,
            1
        )
    );

    EXPECT_EQ(result.status(), TelemetryIngestStatus::BadQuality);
    EXPECT_TRUE(result.rejected());
    EXPECT_EQ(ingestor.statistics().bad_quality_count(), 1);
}

TEST(TelemetryIngestorQualityTests, RejectsCommunicationErrorQuality)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::domain::Quality;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    const auto result = ingestor.ingest(
        make_telemetry_value_with_quality(
            "tag-temperature",
            TagValue(10.0),
            Quality::CommunicationError,
            1
        )
    );

    EXPECT_EQ(result.status(), TelemetryIngestStatus::BadQuality);
    EXPECT_TRUE(result.rejected());
    EXPECT_EQ(ingestor.statistics().bad_quality_count(), 1);
}

TEST(TelemetryIngestorQualityTests, RejectsDisabledQuality)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::domain::Quality;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    const auto result = ingestor.ingest(
        make_telemetry_value_with_quality(
            "tag-temperature",
            TagValue(10.0),
            Quality::Disabled,
            1
        )
    );

    EXPECT_EQ(result.status(), TelemetryIngestStatus::BadQuality);
    EXPECT_TRUE(result.rejected());
    EXPECT_EQ(ingestor.statistics().bad_quality_count(), 1);
}

TEST(TelemetryIngestorQualityTests, AcceptsUncertainQuality)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::domain::Quality;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    const auto result = ingestor.ingest(
        make_telemetry_value_with_quality(
            "tag-temperature",
            TagValue(10.0),
            Quality::Uncertain,
            1
        )
    );

    EXPECT_EQ(result.status(), TelemetryIngestStatus::Accepted);
    EXPECT_TRUE(result.accepted());
    EXPECT_EQ(ingestor.current_state().size(), 1);
    EXPECT_EQ(ingestor.statistics().bad_quality_count(), 0);
}

TEST(TelemetryIngestorQualityTests, AcceptsStaleQuality)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::domain::Quality;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    const auto result = ingestor.ingest(
        make_telemetry_value_with_quality(
            "tag-temperature",
            TagValue(10.0),
            Quality::Stale,
            1
        )
    );

    EXPECT_EQ(result.status(), TelemetryIngestStatus::Accepted);
    EXPECT_TRUE(result.accepted());
    EXPECT_EQ(ingestor.current_state().size(), 1);
    EXPECT_EQ(ingestor.statistics().bad_quality_count(), 0);
}

TEST(TelemetryIngestBatchResultTests, RecordsBadQualityStatus)
{
    dispatcher::core::TelemetryIngestBatchResult result;

    result.record(dispatcher::core::TelemetryIngestStatus::BadQuality);

    EXPECT_FALSE(result.empty());
    EXPECT_FALSE(result.all_accepted());
    EXPECT_TRUE(result.has_rejections());

    EXPECT_EQ(result.total_count(), 1);
    EXPECT_EQ(result.accepted_count(), 0);
    EXPECT_EQ(result.rejected_count(), 1);
    EXPECT_EQ(result.bad_quality_count(), 1);
}

TEST(TelemetryIngestorReloadConfigurationTests, ReloadsPublishedValidConfiguration)
{
    using dispatcher::core::TelemetryIngestor;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    ASSERT_TRUE(builder.add_device(make_device()).valid());
    ASSERT_TRUE(
        builder.add_tag(
            make_tag(
                "tag-pressure",
                "pressure",
                dispatcher::domain::DataType::Float64,
                true
            )
        ).valid()
    );

    const auto result = ingestor.reload_configuration(
        builder
        .config_version(2)
        .published()
        .description("Reloaded configuration")
        .build()
    );

    EXPECT_TRUE(result.valid());
    EXPECT_EQ(ingestor.configuration_snapshot().config_version(), 2);
    EXPECT_TRUE(ingestor.configuration_snapshot().find_tag_by_id(
        dispatcher::domain::TagId{ "tag-pressure" }
    ).has_value());
}

TEST(TelemetryIngestorReloadConfigurationTests, RejectsDraftConfiguration)
{
    using dispatcher::core::TelemetryIngestor;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    ASSERT_TRUE(builder.add_device(make_device()).valid());
    ASSERT_TRUE(
        builder.add_tag(
            make_tag(
                "tag-pressure",
                "pressure",
                dispatcher::domain::DataType::Float64,
                true
            )
        ).valid()
    );

    const auto result = ingestor.reload_configuration(
        builder
        .config_version(2)
        .draft()
        .description("Draft configuration")
        .build()
    );

    EXPECT_FALSE(result.valid());
    EXPECT_TRUE(result.has_errors());
    EXPECT_EQ(ingestor.configuration_snapshot().config_version(), 1);

    const auto tag = ingestor.configuration_snapshot().find_tag_by_id(
        dispatcher::domain::TagId{ "tag-pressure" }
    );

    EXPECT_FALSE(tag.has_value());
}

TEST(TelemetryIngestorReloadConfigurationTests, RejectsInvalidConfiguration)
{
    using dispatcher::core::TelemetryIngestor;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    ASSERT_TRUE(
        builder.add_tag(
            make_tag(
                "tag-orphan",
                "orphan",
                dispatcher::domain::DataType::Float64,
                true
            )
        ).valid()
    );

    const auto result = ingestor.reload_configuration(
        builder
        .config_version(2)
        .published()
        .description("Invalid configuration")
        .build()
    );

    EXPECT_FALSE(result.valid());
    EXPECT_TRUE(result.has_errors());
    EXPECT_EQ(ingestor.configuration_snapshot().config_version(), 1);

    const auto tag = ingestor.configuration_snapshot().find_tag_by_id(
        dispatcher::domain::TagId{ "tag-orphan" }
    );

    EXPECT_FALSE(tag.has_value());
}

TEST(TelemetryIngestorReloadConfigurationTests, ReloadDoesNotClearCurrentStateStatisticsOrLastSequence)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    ASSERT_EQ(
        ingestor.ingest(
            make_telemetry_value(
                "tag-temperature",
                TagValue(10.0),
                10
            )
        ).status(),
        TelemetryIngestStatus::Accepted
    );

    EXPECT_EQ(ingestor.current_state().size(), 1);
    EXPECT_EQ(ingestor.statistics().accepted_count(), 1);

    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    ASSERT_TRUE(builder.add_device(make_device()).valid());
    ASSERT_TRUE(
        builder.add_tag(
            make_tag(
                "tag-temperature",
                "temperature",
                dispatcher::domain::DataType::Float64,
                true
            )
        ).valid()
    );

    const auto result = ingestor.reload_configuration(
        builder
        .config_version(2)
        .published()
        .description("Reloaded same tag")
        .build()
    );

    ASSERT_TRUE(result.valid());

    EXPECT_EQ(ingestor.current_state().size(), 1);
    EXPECT_EQ(ingestor.statistics().accepted_count(), 1);

    const auto last_sequence = ingestor.last_sequence(
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    ASSERT_TRUE(last_sequence.has_value());
    EXPECT_EQ(last_sequence.value(), 10);
}

TEST(TelemetryIngestorReloadConfigurationTests, NewConfigurationAffectsSubsequentIngest)
{
    using dispatcher::core::TelemetryIngestStatus;
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    ASSERT_TRUE(builder.add_device(make_device()).valid());
    ASSERT_TRUE(
        builder.add_tag(
            make_tag(
                "tag-pressure",
                "pressure",
                dispatcher::domain::DataType::Float64,
                true
            )
        ).valid()
    );

    ASSERT_TRUE(
        ingestor.reload_configuration(
            builder
            .config_version(2)
            .published()
            .description("Only pressure tag")
            .build()
        ).valid()
    );

    EXPECT_EQ(
        ingestor.ingest(
            make_telemetry_value(
                "tag-temperature",
                TagValue(10.0),
                1
            )
        ).status(),
        TelemetryIngestStatus::UnknownTag
    );

    EXPECT_EQ(
        ingestor.ingest(
            make_telemetry_value(
                "tag-pressure",
                TagValue(20.0),
                1
            )
        ).status(),
        TelemetryIngestStatus::Accepted
    );
}

TEST(TelemetryRuntimeSnapshotTests, InitialRuntimeSnapshotReflectsEmptyRuntimeState)
{
    dispatcher::core::TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    const auto snapshot = ingestor.runtime_snapshot();

    EXPECT_EQ(snapshot.configuration_version, 1);
    EXPECT_EQ(snapshot.current_state_size, 0);

    EXPECT_EQ(snapshot.accepted_count, 0);
    EXPECT_EQ(snapshot.stored_count, 0);
    EXPECT_EQ(snapshot.accepted_no_change_count, 0);

    EXPECT_EQ(snapshot.rejected_count, 0);
    EXPECT_EQ(snapshot.unknown_tag_count, 0);
    EXPECT_EQ(snapshot.disabled_tag_count, 0);
    EXPECT_EQ(snapshot.data_type_mismatch_count, 0);
    EXPECT_EQ(snapshot.stale_sequence_count, 0);
    EXPECT_EQ(snapshot.future_source_timestamp_count, 0);
    EXPECT_EQ(snapshot.bad_quality_count, 0);

    EXPECT_EQ(snapshot.total_count, 0);
}

TEST(TelemetryRuntimeSnapshotTests, RuntimeSnapshotReflectsAcceptedTelemetry)
{
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    ASSERT_TRUE(
        ingestor.ingest(
            make_telemetry_value(
                "tag-temperature",
                TagValue(10.0),
                1
            )
        ).accepted()
    );

    const auto snapshot = ingestor.runtime_snapshot();

    EXPECT_EQ(snapshot.configuration_version, 1);
    EXPECT_EQ(snapshot.current_state_size, 1);

    EXPECT_EQ(snapshot.accepted_count, 1);
    EXPECT_EQ(snapshot.stored_count, 1);
    EXPECT_EQ(snapshot.accepted_no_change_count, 0);
    EXPECT_EQ(snapshot.rejected_count, 0);
    EXPECT_EQ(snapshot.total_count, 1);
}

TEST(TelemetryRuntimeSnapshotTests, RuntimeSnapshotReflectsRejectedTelemetry)
{
    using dispatcher::core::TelemetryIngestor;
    using dispatcher::telemetry::TagValue;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    ASSERT_TRUE(
        ingestor.ingest(
            make_telemetry_value(
                "unknown-tag",
                TagValue(10.0),
                1
            )
        ).rejected()
    );

    const auto snapshot = ingestor.runtime_snapshot();

    EXPECT_EQ(snapshot.configuration_version, 1);
    EXPECT_EQ(snapshot.current_state_size, 0);

    EXPECT_EQ(snapshot.accepted_count, 0);
    EXPECT_EQ(snapshot.stored_count, 0);
    EXPECT_EQ(snapshot.rejected_count, 1);
    EXPECT_EQ(snapshot.unknown_tag_count, 1);
    EXPECT_EQ(snapshot.total_count, 1);
}

TEST(TelemetryRuntimeSnapshotTests, RuntimeSnapshotReflectsConfigurationReload)
{
    using dispatcher::core::TelemetryIngestor;

    TelemetryIngestor ingestor(
        make_snapshot_with_single_tag()
    );

    dispatcher::domain::ConfigurationSnapshotBuilder builder;

    ASSERT_TRUE(builder.add_device(make_device()).valid());
    ASSERT_TRUE(
        builder.add_tag(
            make_tag(
                "tag-pressure",
                "pressure",
                dispatcher::domain::DataType::Float64,
                true
            )
        ).valid()
    );

    ASSERT_TRUE(
        ingestor.reload_configuration(
            builder
            .config_version(2)
            .published()
            .description("Reloaded config")
            .build()
        ).valid()
    );

    const auto snapshot = ingestor.runtime_snapshot();

    EXPECT_EQ(snapshot.configuration_version, 2);
    EXPECT_EQ(snapshot.current_state_size, 0);
}