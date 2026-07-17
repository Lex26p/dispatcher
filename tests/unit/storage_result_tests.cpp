#include <dispatcher/storage/storage_error.hpp>
#include <dispatcher/storage/storage_result.hpp>
#include <dispatcher/storage/storage_status.hpp>

#include <gtest/gtest.h>

TEST(StorageStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::storage::to_string(
            dispatcher::storage::StorageStatus::Success
        ),
        "success"
    );

    EXPECT_STREQ(
        dispatcher::storage::to_string(
            dispatcher::storage::StorageStatus::NotFound
        ),
        "not_found"
    );

    EXPECT_STREQ(
        dispatcher::storage::to_string(
            dispatcher::storage::StorageStatus::AlreadyExists
        ),
        "already_exists"
    );

    EXPECT_STREQ(
        dispatcher::storage::to_string(
            dispatcher::storage::StorageStatus::Conflict
        ),
        "conflict"
    );

    EXPECT_STREQ(
        dispatcher::storage::to_string(
            dispatcher::storage::StorageStatus::ValidationError
        ),
        "validation_error"
    );

    EXPECT_STREQ(
        dispatcher::storage::to_string(
            dispatcher::storage::StorageStatus::SerializationError
        ),
        "serialization_error"
    );

    EXPECT_STREQ(
        dispatcher::storage::to_string(
            dispatcher::storage::StorageStatus::IoError
        ),
        "io_error"
    );

    EXPECT_STREQ(
        dispatcher::storage::to_string(
            dispatcher::storage::StorageStatus::BackendUnavailable
        ),
        "backend_unavailable"
    );

    EXPECT_STREQ(
        dispatcher::storage::to_string(
            dispatcher::storage::StorageStatus::Timeout
        ),
        "timeout"
    );

    EXPECT_STREQ(
        dispatcher::storage::to_string(
            dispatcher::storage::StorageStatus::UnsupportedOperation
        ),
        "unsupported_operation"
    );

    EXPECT_STREQ(
        dispatcher::storage::to_string(
            dispatcher::storage::StorageStatus::UnknownError
        ),
        "unknown_error"
    );
}

TEST(StorageStatusTests, SuccessAndFailurePredicatesWork)
{
    EXPECT_TRUE(
        dispatcher::storage::is_success(
            dispatcher::storage::StorageStatus::Success
        )
    );

    EXPECT_FALSE(
        dispatcher::storage::is_failure(
            dispatcher::storage::StorageStatus::Success
        )
    );

    EXPECT_FALSE(
        dispatcher::storage::is_success(
            dispatcher::storage::StorageStatus::NotFound
        )
    );

    EXPECT_TRUE(
        dispatcher::storage::is_failure(
            dispatcher::storage::StorageStatus::NotFound
        )
    );
}

TEST(StorageErrorTests, EmptyReflectsSuccessfulEmptyError)
{
    const dispatcher::storage::StorageError error{
        .status = dispatcher::storage::StorageStatus::Success
    };

    EXPECT_TRUE(error.empty());
    EXPECT_FALSE(error.has_operation());
    EXPECT_FALSE(error.has_key());
    EXPECT_FALSE(error.has_message());
}

TEST(StorageErrorTests, NonEmptyErrorReportsFields)
{
    const dispatcher::storage::StorageError error{
        .status = dispatcher::storage::StorageStatus::NotFound,
        .operation = "load",
        .key = "history/tag-1",
        .message = "sample not found"
    };

    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(error.has_operation());
    EXPECT_TRUE(error.has_key());
    EXPECT_TRUE(error.has_message());
}

TEST(StorageResultTests, SuccessResultWorks)
{
    const auto result = dispatcher::storage::StorageResult::success();

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::storage::StorageStatus::Success
    );

    EXPECT_TRUE(result.error().empty());
    EXPECT_TRUE(result.operation().empty());
    EXPECT_TRUE(result.key().empty());
    EXPECT_TRUE(result.message().empty());
}

TEST(StorageResultTests, FailureResultCapturesError)
{
    const auto result = dispatcher::storage::StorageResult::failure(
        dispatcher::storage::StorageStatus::NotFound,
        "load",
        "history/tag-1",
        "sample not found"
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::storage::StorageStatus::NotFound
    );

    EXPECT_EQ(result.operation(), "load");
    EXPECT_EQ(result.key(), "history/tag-1");
    EXPECT_EQ(result.message(), "sample not found");

    EXPECT_EQ(
        result.error().status,
        dispatcher::storage::StorageStatus::NotFound
    );

    EXPECT_EQ(result.error().operation, "load");
    EXPECT_EQ(result.error().key, "history/tag-1");
    EXPECT_EQ(result.error().message, "sample not found");
}

TEST(StorageResultTests, FailureWithSuccessStatusIsConvertedToUnknownError)
{
    const auto result = dispatcher::storage::StorageResult::failure(
        dispatcher::storage::StorageStatus::Success,
        "write",
        "alarm/event-1",
        "invalid failure status"
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::storage::StorageStatus::UnknownError
    );

    EXPECT_EQ(result.operation(), "write");
    EXPECT_EQ(result.key(), "alarm/event-1");
    EXPECT_EQ(result.message(), "invalid failure status");
}