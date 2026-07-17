#include <dispatcher/config/configuration_document.hpp>
#include <dispatcher/config/configuration_document_result.hpp>
#include <dispatcher/config/configuration_format.hpp>
#include <dispatcher/config/configuration_io_status.hpp>

#include <gtest/gtest.h>

#include <stdexcept>

TEST(ConfigurationFormatTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::config::to_string(
            dispatcher::config::ConfigurationFormat::Json
        ),
        "json"
    );

    EXPECT_STREQ(
        dispatcher::config::to_string(
            dispatcher::config::ConfigurationFormat::Unknown
        ),
        "unknown"
    );
}

TEST(ConfigurationFormatTests, ParseRecognizesJsonAliases)
{
    EXPECT_EQ(
        dispatcher::config::parse_configuration_format("json"),
        dispatcher::config::ConfigurationFormat::Json
    );

    EXPECT_EQ(
        dispatcher::config::parse_configuration_format("JSON"),
        dispatcher::config::ConfigurationFormat::Json
    );

    EXPECT_EQ(
        dispatcher::config::parse_configuration_format("application/json"),
        dispatcher::config::ConfigurationFormat::Json
    );
}

TEST(ConfigurationFormatTests, ParseUnknownFormat)
{
    EXPECT_EQ(
        dispatcher::config::parse_configuration_format("yaml"),
        dispatcher::config::ConfigurationFormat::Unknown
    );

    EXPECT_FALSE(
        dispatcher::config::is_known_configuration_format(
            dispatcher::config::ConfigurationFormat::Unknown
        )
    );

    EXPECT_TRUE(
        dispatcher::config::is_known_configuration_format(
            dispatcher::config::ConfigurationFormat::Json
        )
    );
}

TEST(ConfigurationDocumentTests, DefaultDocumentIsEmptyAndUnknown)
{
    const dispatcher::config::ConfigurationDocument document;

    EXPECT_EQ(
        document.format(),
        dispatcher::config::ConfigurationFormat::Unknown
    );

    EXPECT_TRUE(document.empty());
    EXPECT_EQ(document.size(), 0);
    EXPECT_FALSE(document.has_name());
    EXPECT_FALSE(document.has_known_format());
}

TEST(ConfigurationDocumentTests, JsonDocumentFactoryWorks)
{
    const auto document = dispatcher::config::ConfigurationDocument::json(
        "{\"config_version\":7}",
        "production.json"
    );

    EXPECT_EQ(
        document.format(),
        dispatcher::config::ConfigurationFormat::Json
    );

    EXPECT_EQ(document.content(), "{\"config_version\":7}");
    EXPECT_EQ(document.name(), "production.json");
    EXPECT_EQ(document.size(), 20);
    EXPECT_FALSE(document.empty());
    EXPECT_TRUE(document.has_name());
    EXPECT_TRUE(document.has_known_format());
}

TEST(ConfigurationDocumentResultTests, SuccessResultContainsDocument)
{
    const auto result = dispatcher::config::ConfigurationDocumentResult::success(
        dispatcher::config::ConfigurationDocument::json(
            "{\"config_version\":7}",
            "production.json"
        )
    );

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::Success
    );

    EXPECT_TRUE(result.has_document());

    EXPECT_EQ(
        result.document().format(),
        dispatcher::config::ConfigurationFormat::Json
    );

    EXPECT_EQ(result.document().name(), "production.json");
}

TEST(ConfigurationDocumentResultTests, FailureResultDoesNotContainDocument)
{
    const auto result = dispatcher::config::ConfigurationDocumentResult::failure(
        dispatcher::config::ConfigurationIoStatus::UnsupportedFormat,
        "configuration.import",
        "production.yaml",
        {},
        "unsupported configuration format"
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::UnsupportedFormat
    );

    EXPECT_FALSE(result.has_document());

    EXPECT_EQ(result.error().operation, "configuration.import");
    EXPECT_EQ(result.error().resource, "production.yaml");
    EXPECT_EQ(result.error().message, "unsupported configuration format");

    EXPECT_THROW(
        (void)result.document(),
        std::logic_error
    );
}