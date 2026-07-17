#include <dispatcher/api/api_page_request.hpp>
#include <dispatcher/api/api_status.hpp>
#include <dispatcher/api/dispatcher_history_api.hpp>
#include <dispatcher/api/history_api.hpp>
#include <dispatcher/api/history_query_api_result.hpp>
#include <dispatcher/api/history_query_request.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/runtime/dispatcher_runtime.hpp>
#include <dispatcher/storage/in_memory_storage_repository.hpp>

#include <gtest/gtest.h>

TEST(HistoryQueryRequestTests, DefaultRequestIsUnbounded)
{
    const dispatcher::api::HistoryQueryRequest request;

    EXPECT_FALSE(request.has_tag_id());
    EXPECT_FALSE(request.has_from());
    EXPECT_FALSE(request.has_to());
    EXPECT_FALSE(request.has_time_range());
    EXPECT_FALSE(request.has_bounded_time_range());
    EXPECT_FALSE(request.requests_latest_only());

    const auto storage_query = request.to_storage_query();

    EXPECT_FALSE(storage_query.has_tag());
    EXPECT_FALSE(storage_query.has_from());
    EXPECT_FALSE(storage_query.has_to());
    EXPECT_FALSE(storage_query.has_time_range());
    EXPECT_FALSE(storage_query.has_bounded_time_range());
    EXPECT_TRUE(storage_query.has_limit());
    EXPECT_FALSE(storage_query.requests_latest_only());
    EXPECT_EQ(storage_query.limit, 100);
}

TEST(HistoryQueryRequestTests, RequestConvertsToStorageQuery)
{
    dispatcher::api::HistoryQueryRequest request;

    request.tag_id = dispatcher::domain::TagId{ "tag-1" };
    request.page = dispatcher::api::ApiPageRequest{
        .offset = 20,
        .limit = 10
    };
    request.latest_only = true;

    EXPECT_TRUE(request.has_tag_id());
    EXPECT_TRUE(request.requests_latest_only());

    const auto storage_query = request.to_storage_query();

    EXPECT_TRUE(storage_query.has_tag());
    EXPECT_TRUE(storage_query.has_limit());
    EXPECT_TRUE(storage_query.requests_latest_only());

    ASSERT_TRUE(storage_query.tag_id.has_value());
    EXPECT_EQ(storage_query.tag_id.value(), dispatcher::domain::TagId{ "tag-1" });
    EXPECT_EQ(storage_query.limit, 10);
}

TEST(HistoryQueryApiResultTests, SuccessResultContainsSamplesAndPage)
{
    const auto result = dispatcher::api::HistoryQueryApiResult::success(
        {},
        dispatcher::api::ApiPage{
            20,
            10,
            0,
            0
        }
    );

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(result.status(), dispatcher::api::ApiStatus::Success);

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.sample_count(), 0);
    EXPECT_TRUE(result.samples().empty());

    EXPECT_EQ(result.page().offset(), 20);
    EXPECT_EQ(result.page().limit(), 10);
    EXPECT_EQ(result.page().returned_count(), 0);
    EXPECT_EQ(result.page().total_count(), 0);
}

TEST(HistoryQueryApiResultTests, FailureResultCapturesError)
{
    const auto result = dispatcher::api::HistoryQueryApiResult::failure(
        dispatcher::api::ApiStatus::StorageError,
        "history.query",
        "storage_repository",
        {},
        "runtime does not have a storage repository"
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(result.status(), dispatcher::api::ApiStatus::StorageError);

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.sample_count(), 0);

    EXPECT_EQ(result.error().operation, "history.query");
    EXPECT_EQ(result.error().resource, "storage_repository");
    EXPECT_EQ(
        result.error().message,
        "runtime does not have a storage repository"
    );
}

TEST(DispatcherHistoryApiTests, QueryWithoutStorageRepositoryFails)
{
    dispatcher::runtime::DispatcherRuntime runtime;
    dispatcher::api::DispatcherHistoryApi api(runtime);

    dispatcher::api::HistoryApi& history_api = api;

    const auto result = history_api.query(
        dispatcher::api::HistoryQueryRequest{}
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(result.status(), dispatcher::api::ApiStatus::StorageError);
    EXPECT_EQ(result.error().operation, "history.query");
    EXPECT_EQ(result.error().resource, "storage_repository");
}

TEST(DispatcherHistoryApiTests, QueryWithStorageRepositoryReturnsEmptyPage)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;
    dispatcher::runtime::DispatcherRuntime runtime(storage_repository);

    dispatcher::api::DispatcherHistoryApi api(runtime);
    dispatcher::api::HistoryApi& history_api = api;

    const auto result = history_api.query(
        dispatcher::api::HistoryQueryRequest{
            .page = dispatcher::api::ApiPageRequest{
                .offset = 0,
                .limit = 25
            }
        }
    );

    ASSERT_TRUE(result.ok());

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.sample_count(), 0);

    EXPECT_EQ(result.page().offset(), 0);
    EXPECT_EQ(result.page().limit(), 25);
    EXPECT_EQ(result.page().returned_count(), 0);
    EXPECT_EQ(result.page().total_count(), 0);
}

TEST(DispatcherHistoryApiTests, ExposesWrappedRuntime)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;
    dispatcher::runtime::DispatcherRuntime runtime(storage_repository);

    dispatcher::api::DispatcherHistoryApi api(runtime);

    EXPECT_EQ(&api.runtime(), &runtime);

    const auto& const_api = api;

    EXPECT_EQ(&const_api.runtime(), &runtime);
}