#include <dispatcher/api/api_error.hpp>
#include <dispatcher/api/api_result.hpp>
#include <dispatcher/api/api_status.hpp>

#include <gtest/gtest.h>

TEST(ApiStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::api::to_string(dispatcher::api::ApiStatus::Success),
        "success"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(dispatcher::api::ApiStatus::Accepted),
        "accepted"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(dispatcher::api::ApiStatus::NoContent),
        "no_content"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(dispatcher::api::ApiStatus::BadRequest),
        "bad_request"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(dispatcher::api::ApiStatus::Unauthorized),
        "unauthorized"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(dispatcher::api::ApiStatus::Forbidden),
        "forbidden"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(dispatcher::api::ApiStatus::NotFound),
        "not_found"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(dispatcher::api::ApiStatus::Conflict),
        "conflict"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::ApiStatus::ValidationError
        ),
        "validation_error"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::ApiStatus::RuntimeRejected
        ),
        "runtime_rejected"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(dispatcher::api::ApiStatus::StorageError),
        "storage_error"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(dispatcher::api::ApiStatus::Timeout),
        "timeout"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::ApiStatus::UnsupportedOperation
        ),
        "unsupported_operation"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(dispatcher::api::ApiStatus::InternalError),
        "internal_error"
    );
}

TEST(ApiStatusTests, PredicatesClassifyStatuses)
{
    EXPECT_TRUE(dispatcher::api::is_success(dispatcher::api::ApiStatus::Success));
    EXPECT_TRUE(dispatcher::api::is_success(dispatcher::api::ApiStatus::Accepted));
    EXPECT_TRUE(dispatcher::api::is_success(dispatcher::api::ApiStatus::NoContent));

    EXPECT_FALSE(dispatcher::api::is_failure(dispatcher::api::ApiStatus::Success));
    EXPECT_TRUE(dispatcher::api::is_failure(dispatcher::api::ApiStatus::BadRequest));

    EXPECT_TRUE(dispatcher::api::is_client_error(dispatcher::api::ApiStatus::BadRequest));
    EXPECT_TRUE(dispatcher::api::is_client_error(dispatcher::api::ApiStatus::Unauthorized));
    EXPECT_TRUE(dispatcher::api::is_client_error(dispatcher::api::ApiStatus::Forbidden));
    EXPECT_TRUE(dispatcher::api::is_client_error(dispatcher::api::ApiStatus::NotFound));
    EXPECT_TRUE(dispatcher::api::is_client_error(dispatcher::api::ApiStatus::Conflict));
    EXPECT_TRUE(dispatcher::api::is_client_error(dispatcher::api::ApiStatus::ValidationError));

    EXPECT_FALSE(dispatcher::api::is_client_error(dispatcher::api::ApiStatus::InternalError));

    EXPECT_TRUE(dispatcher::api::is_server_error(dispatcher::api::ApiStatus::RuntimeRejected));
    EXPECT_TRUE(dispatcher::api::is_server_error(dispatcher::api::ApiStatus::StorageError));
    EXPECT_TRUE(dispatcher::api::is_server_error(dispatcher::api::ApiStatus::Timeout));
    EXPECT_TRUE(dispatcher::api::is_server_error(dispatcher::api::ApiStatus::UnsupportedOperation));
    EXPECT_TRUE(dispatcher::api::is_server_error(dispatcher::api::ApiStatus::InternalError));

    EXPECT_FALSE(dispatcher::api::is_server_error(dispatcher::api::ApiStatus::BadRequest));
}

TEST(ApiErrorTests, EmptyReflectsSuccessfulEmptyError)
{
    const dispatcher::api::ApiError error{
        .status = dispatcher::api::ApiStatus::Success
    };

    EXPECT_TRUE(error.empty());
    EXPECT_FALSE(error.has_operation());
    EXPECT_FALSE(error.has_resource());
    EXPECT_FALSE(error.has_field());
    EXPECT_FALSE(error.has_message());
}

TEST(ApiErrorTests, NonEmptyErrorReportsFields)
{
    const dispatcher::api::ApiError error{
        .status = dispatcher::api::ApiStatus::ValidationError,
        .operation = "telemetry.ingest",
        .resource = "tag-1",
        .field = "value",
        .message = "data type mismatch"
    };

    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(error.has_operation());
    EXPECT_TRUE(error.has_resource());
    EXPECT_TRUE(error.has_field());
    EXPECT_TRUE(error.has_message());
}

TEST(ApiResultTests, SuccessAcceptedAndNoContentResultsWork)
{
    const auto success = dispatcher::api::ApiResult::success();
    const auto accepted = dispatcher::api::ApiResult::accepted();
    const auto no_content = dispatcher::api::ApiResult::no_content();

    EXPECT_TRUE(success.ok());
    EXPECT_TRUE(accepted.ok());
    EXPECT_TRUE(no_content.ok());

    EXPECT_FALSE(success.failed());
    EXPECT_FALSE(accepted.failed());
    EXPECT_FALSE(no_content.failed());

    EXPECT_EQ(success.status(), dispatcher::api::ApiStatus::Success);
    EXPECT_EQ(accepted.status(), dispatcher::api::ApiStatus::Accepted);
    EXPECT_EQ(no_content.status(), dispatcher::api::ApiStatus::NoContent);
}

TEST(ApiResultTests, FailureResultCapturesError)
{
    const auto result = dispatcher::api::ApiResult::failure(
        dispatcher::api::ApiStatus::ValidationError,
        "telemetry.ingest",
        "tag-1",
        "value",
        "data type mismatch"
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::ApiStatus::ValidationError
    );

    EXPECT_EQ(result.operation(), "telemetry.ingest");
    EXPECT_EQ(result.resource(), "tag-1");
    EXPECT_EQ(result.field(), "value");
    EXPECT_EQ(result.message(), "data type mismatch");

    EXPECT_EQ(
        result.error().status,
        dispatcher::api::ApiStatus::ValidationError
    );

    EXPECT_EQ(result.error().operation, "telemetry.ingest");
    EXPECT_EQ(result.error().resource, "tag-1");
    EXPECT_EQ(result.error().field, "value");
    EXPECT_EQ(result.error().message, "data type mismatch");
}

TEST(ApiResultTests, FailureWithSuccessStatusIsConvertedToInternalError)
{
    const auto result = dispatcher::api::ApiResult::failure(
        dispatcher::api::ApiStatus::Success,
        "runtime.snapshot",
        "runtime",
        {},
        "invalid failure status"
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::ApiStatus::InternalError
    );

    EXPECT_EQ(result.operation(), "runtime.snapshot");
    EXPECT_EQ(result.resource(), "runtime");
    EXPECT_TRUE(result.field().empty());
    EXPECT_EQ(result.message(), "invalid failure status");
}