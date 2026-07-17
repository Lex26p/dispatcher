#include <dispatcher/domain/configuration_metadata.hpp>
#include <dispatcher/domain/configuration_status.hpp>

#include <gtest/gtest.h>

TEST(ConfigurationStatusTests, ToStringReturnsExpectedValues)
{
    using dispatcher::domain::ConfigurationStatus;
    using dispatcher::domain::to_string;

    EXPECT_EQ(to_string(ConfigurationStatus::Draft), "draft");
    EXPECT_EQ(to_string(ConfigurationStatus::Published), "published");
}

TEST(ConfigurationMetadataTests, StoresMetadataValues)
{
    using namespace dispatcher::domain;

    const auto now = ConfigurationMetadata::Clock::now();

    const ConfigurationMetadata metadata(
        42,
        ConfigurationStatus::Draft,
        "Initial draft",
        now
    );

    EXPECT_EQ(metadata.config_version(), 42);
    EXPECT_EQ(metadata.status(), ConfigurationStatus::Draft);
    EXPECT_EQ(metadata.description(), "Initial draft");
    EXPECT_EQ(metadata.created_at(), now);
    EXPECT_TRUE(metadata.is_draft());
    EXPECT_FALSE(metadata.is_published());
}

TEST(ConfigurationMetadataTests, DraftFactoryCreatesDraftMetadata)
{
    const auto metadata = dispatcher::domain::ConfigurationMetadata::draft(
        7,
        "Draft config"
    );

    EXPECT_EQ(metadata.config_version(), 7);
    EXPECT_TRUE(metadata.is_draft());
    EXPECT_FALSE(metadata.is_published());
    EXPECT_EQ(metadata.description(), "Draft config");
}

TEST(ConfigurationMetadataTests, PublishedFactoryCreatesPublishedMetadata)
{
    const auto metadata = dispatcher::domain::ConfigurationMetadata::published(
        8,
        "Published config"
    );

    EXPECT_EQ(metadata.config_version(), 8);
    EXPECT_FALSE(metadata.is_draft());
    EXPECT_TRUE(metadata.is_published());
    EXPECT_EQ(metadata.description(), "Published config");
}