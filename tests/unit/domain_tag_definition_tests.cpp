#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/deadband.hpp>
#include <dispatcher/domain/history_policy.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/domain/scaling.hpp>
#include <dispatcher/domain/tag_definition.hpp>

#include <gtest/gtest.h>

TEST(DomainEnumsTests, QualityToStringReturnsExpectedValues)
{
    using dispatcher::domain::Quality;
    using dispatcher::domain::to_string;

    EXPECT_EQ(to_string(Quality::Good), "good");
    EXPECT_EQ(to_string(Quality::CommunicationError), "communication_error");
    EXPECT_EQ(to_string(Quality::ManualOverride), "manual_override");
}

TEST(DomainEnumsTests, HistoryPolicyToStringReturnsExpectedValues)
{
    using dispatcher::domain::HistoryPolicy;
    using dispatcher::domain::to_string;

    EXPECT_EQ(to_string(HistoryPolicy::OnChangeWithForcedSample), "on_change_with_forced_sample");
    EXPECT_EQ(to_string(HistoryPolicy::CriticalLossless), "critical_lossless");
    EXPECT_EQ(to_string(HistoryPolicy::DiagnosticBestEffort), "diagnostic_best_effort");
}

TEST(DomainEnumsTests, DataTypeToStringReturnsExpectedValues)
{
    using dispatcher::domain::DataType;
    using dispatcher::domain::to_string;

    EXPECT_EQ(to_string(DataType::Boolean), "boolean");
    EXPECT_EQ(to_string(DataType::Float64), "float64");
    EXPECT_EQ(to_string(DataType::String), "string");
}

TEST(DeadbandTests, ZeroDeadbandAcceptsDifferentValues)
{
    const dispatcher::domain::Deadband deadband;

    EXPECT_FALSE(deadband.enabled());
    EXPECT_TRUE(deadband.accepts_change(10.0, 11.0));
    EXPECT_FALSE(deadband.accepts_change(10.0, 10.0));
}

TEST(DeadbandTests, PositiveDeadbandAcceptsOnlyLargeEnoughChanges)
{
    const dispatcher::domain::Deadband deadband(0.5);

    EXPECT_TRUE(deadband.enabled());
    EXPECT_FALSE(deadband.accepts_change(10.0, 10.2));
    EXPECT_TRUE(deadband.accepts_change(10.0, 10.5));
    EXPECT_TRUE(deadband.accepts_change(10.0, 11.0));
}

TEST(ScalingTests, AppliesMultiplierAndOffset)
{
    const dispatcher::domain::Scaling scaling(2.0, 5.0);

    EXPECT_DOUBLE_EQ(scaling.apply(10.0), 25.0);
}

TEST(TagDefinitionTests, StoresTagMetadata)
{
    using namespace dispatcher::domain;

    const TagDefinition tag(
        OrganizationId{ "org-1" },
        SiteId{ "site-1" },
        AreaId{ "area-1" },
        DeviceId{ "device-1" },
        TagId{ "tag-1" },
        "Temperature",
        "Main motor temperature",
        DataType::Float64,
        "C",
        HistoryPolicy::OnChangeWithForcedSample,
        Deadband{ 0.1 },
        Scaling{ 1.0, 0.0 },
        true,
        1
    );

    EXPECT_EQ(tag.organization_id().value(), "org-1");
    EXPECT_EQ(tag.site_id().value(), "site-1");
    EXPECT_EQ(tag.area_id().value(), "area-1");
    EXPECT_EQ(tag.device_id().value(), "device-1");
    EXPECT_EQ(tag.tag_id().value(), "tag-1");

    EXPECT_EQ(tag.name(), "Temperature");
    EXPECT_EQ(tag.description(), "Main motor temperature");
    EXPECT_EQ(tag.data_type(), DataType::Float64);
    EXPECT_EQ(tag.engineering_unit(), "C");
    EXPECT_EQ(tag.history_policy(), HistoryPolicy::OnChangeWithForcedSample);
    EXPECT_DOUBLE_EQ(tag.deadband().value(), 0.1);
    EXPECT_DOUBLE_EQ(tag.scaling().apply(10.0), 10.0);
    EXPECT_TRUE(tag.enabled());
    EXPECT_EQ(tag.config_version(), 1);
}