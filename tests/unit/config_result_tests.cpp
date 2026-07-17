#include <dispatcher/config/configuration_io_error.hpp>
#include <dispatcher/config/configuration_io_result.hpp>
#include <dispatcher/config/configuration_io_status.hpp>

#include <gtest/gtest.h>

TEST(ConfigurationIoStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::config::to_string(
            dispatcher::config::ConfigurationIoStatus::Success
        ),
        "success"
    );

    EXPECT_STREQ(
        dispatcher::config::to_string(
            dispatcher::config::ConfigurationIoStatus::UnsupportedFormat
        ),
        "unsupported_format"
    );

    EXPECT_STREQ(
        dispatcher::config::to_string(
            dispatcher::config::ConfigurationIoStatus::ValidationError
        ),
        "validation_error"
    );

    EXPECT_STREQ(
        dispatcher::config::to_string(
            dispatcher::config::ConfigurationIoStatus::ParseError
        ),
        "parse_error"
    );

    EXPECT_STREQ(
        dispatcher::config::to_string(
            dispatcher::config::ConfigurationIoStatus::SerializationError
        ),
        "serialization_error"
    );

    EXPECT_STREQ(
        dispatcher::config::to_string(
            dispatcher::config::ConfigurationIoStatus::NotFound
        ),
        "not_found"
    );

    EXPECT_STREQ(
        dispatcher::config::to_string(
            dispatcher::config::ConfigurationIoStatus::Conflict
        ),
        "conflict"
    );

    EXPECT_STREQ(
        dispatcher::config::to_string(
            dispatcher::config::ConfigurationIoStatus::IoError
        ),
        "io_error"
    );

    EXPECT_STREQ(
        dispatcher::config::to_string(
            dispatcher::config::ConfigurationIoStatus::UnknownError
        ),
        "unknown_error"
    );
}

TEST(ConfigurationIoStatusTests, PredicatesClassifyStatuses)
{
    EXPECT_TRUE(
        dispatcher::config::is_success(
            dispatcher::config::ConfigurationIoStatus::Success
        )
    );

    EXPECT_FALSE(
        dispatcher::config::is_failure(
            dispatcher::config::ConfigurationIoStatus::Success
        )
    );

    EXPECT_TRUE(
        dispatcher::config::is_failure(
            dispatcher::config::ConfigurationIoStatus::ParseError
        )
    );
}

TEST(ConfigurationIoErrorTests, EmptyReflectsSuccessfulEmptyError)
{
    const dispatcher::config::ConfigurationIoError error{
        .status = dispatcher::config::ConfigurationIoStatus::Success
    };

    EXPECT_TRUE(error.empty());
    EXPECT_FALSE(error.has_operation());
    EXPECT_FALSE(error.has_resource());
    EXPECT_FALSE(error.has_field());
    EXPECT_FALSE(error.has_message());
}

TEST(ConfigurationIoErrorTests, NonEmptyErrorReportsFields)
{
    const dispatcher::config::ConfigurationIoError error{
        .status = dispatcher::config::ConfigurationIoStatus::ParseError,
        .operation = "configuration.import",
        .resource = "production.json",
        .field = "devices[0].device_id",
        .message = "device id is required"
    };

    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(error.has_operation());
    EXPECT_TRUE(error.has_resource());
    EXPECT_TRUE(error.has_field());
    EXPECT_TRUE(error.has_message());
}

TEST(ConfigurationIoResultTests, SuccessResultWorks)
{
    const auto result =
        dispatcher::config::ConfigurationIoResult::success();

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::Success
    );

    EXPECT_TRUE(result.error().empty());
}

TEST(ConfigurationIoResultTests, FailureResultCapturesError)
{
    const auto result =
        dispatcher::config::ConfigurationIoResult::failure(
            dispatcher::config::ConfigurationIoStatus::ParseError,
            "configuration.import",
            "production.json",
            "tags[0].data_type",
            "unsupported data type"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::ParseError
    );

    EXPECT_EQ(result.operation(), "configuration.import");
    EXPECT_EQ(result.resource(), "production.json");
    EXPECT_EQ(result.field(), "tags[0].data_type");
    EXPECT_EQ(result.message(), "unsupported data type");

    EXPECT_EQ(result.error().operation, "configuration.import");
    EXPECT_EQ(result.error().resource, "production.json");
    EXPECT_EQ(result.error().field, "tags[0].data_type");
    EXPECT_EQ(result.error().message, "unsupported data type");
}

TEST(ConfigurationIoResultTests, FailureWithSuccessStatusBecomesUnknownError)
{
    const auto result =
        dispatcher::config::ConfigurationIoResult::failure(
            dispatcher::config::ConfigurationIoStatus::Success,
            "configuration.export",
            "production.json",
            {},
            "invalid failure status"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::config::ConfigurationIoStatus::UnknownError
    );

    EXPECT_EQ(result.operation(), "configuration.export");
    EXPECT_EQ(result.resource(), "production.json");
    EXPECT_EQ(result.message(), "invalid failure status");
}