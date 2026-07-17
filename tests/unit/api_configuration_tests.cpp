#include <dispatcher/api/api_page_request.hpp>
#include <dispatcher/api/api_status.hpp>
#include <dispatcher/api/configuration_api.hpp>
#include <dispatcher/api/configuration_query_api_result.hpp>
#include <dispatcher/api/configuration_query_request.hpp>
#include <dispatcher/api/configuration_reload_api_result.hpp>
#include <dispatcher/api/configuration_reload_request.hpp>
#include <dispatcher/api/dispatcher_configuration_api.hpp>
#include <dispatcher/domain/configuration_snapshot_builder.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/runtime/dispatcher_runtime.hpp>
#include <dispatcher/storage/in_memory_storage_repository.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>

namespace
{
    dispatcher::domain::ConfigurationSnapshot make_api_configuration_snapshot(
        std::uint64_t config_version,
        dispatcher::domain::ConfigurationStatus status =
        dispatcher::domain::ConfigurationStatus::Published
    )
    {
        return dispatcher::domain::ConfigurationSnapshotBuilder{}
            .config_version(config_version)
            .status(status)
            .build();
    }
}

TEST(ConfigurationQueryRequestTests, DefaultRequestConvertsToStorageQuery)
{
    const dispatcher::api::ConfigurationQueryRequest request;

    EXPECT_FALSE(request.has_config_version());
    EXPECT_FALSE(request.has_status());
    EXPECT_FALSE(request.has_name());
    EXPECT_FALSE(request.requests_latest_only());

    const auto storage_query = request.to_storage_query();

    EXPECT_FALSE(storage_query.has_config_version());
    EXPECT_FALSE(storage_query.has_status());
    EXPECT_FALSE(storage_query.has_name());
    EXPECT_TRUE(storage_query.has_limit());
    EXPECT_FALSE(storage_query.requests_latest_only());
    EXPECT_EQ(storage_query.limit, 100);
}

TEST(ConfigurationQueryRequestTests, RequestConvertsToStorageQuery)
{
    dispatcher::api::ConfigurationQueryRequest request;

    request.config_version = 7;
    request.status = dispatcher::domain::ConfigurationStatus::Published;
    request.name = "production";
    request.page = dispatcher::api::ApiPageRequest{
        .offset = 0,
        .limit = 25
    };
    request.latest_only = true;

    EXPECT_TRUE(request.has_config_version());
    EXPECT_TRUE(request.has_status());
    EXPECT_TRUE(request.has_name());
    EXPECT_TRUE(request.requests_latest_only());

    const auto storage_query = request.to_storage_query();

    EXPECT_TRUE(storage_query.has_config_version());
    EXPECT_TRUE(storage_query.has_status());
    EXPECT_TRUE(storage_query.has_name());
    EXPECT_TRUE(storage_query.has_limit());
    EXPECT_TRUE(storage_query.requests_latest_only());

    EXPECT_EQ(storage_query.config_version.value(), 7);
    EXPECT_EQ(
        storage_query.status.value(),
        dispatcher::domain::ConfigurationStatus::Published
    );
    EXPECT_EQ(storage_query.name.value(), "production");
    EXPECT_EQ(storage_query.limit, 25);
}

TEST(ConfigurationQueryApiResultTests, SuccessResultContainsSnapshotsAndPage)
{
    const auto result =
        dispatcher::api::ConfigurationQueryApiResult::success(
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
    EXPECT_EQ(result.snapshot_count(), 0);
    EXPECT_TRUE(result.snapshots().empty());

    EXPECT_EQ(result.page().offset(), 20);
    EXPECT_EQ(result.page().limit(), 10);
    EXPECT_EQ(result.page().returned_count(), 0);
    EXPECT_EQ(result.page().total_count(), 0);
}

TEST(ConfigurationQueryApiResultTests, FailureResultCapturesError)
{
    const auto result =
        dispatcher::api::ConfigurationQueryApiResult::failure(
            dispatcher::api::ApiStatus::StorageError,
            "configuration.query",
            "storage_repository",
            {},
            "runtime does not have a storage repository"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(result.status(), dispatcher::api::ApiStatus::StorageError);
    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.snapshot_count(), 0);

    EXPECT_EQ(result.error().operation, "configuration.query");
    EXPECT_EQ(result.error().resource, "storage_repository");
    EXPECT_EQ(
        result.error().message,
        "runtime does not have a storage repository"
    );
}

TEST(ConfigurationReloadRequestTests, RequestExposesSnapshotMetadata)
{
    const dispatcher::api::ConfigurationReloadRequest request(
        make_api_configuration_snapshot(7)
    );

    EXPECT_EQ(request.config_version(), 7);
    EXPECT_EQ(
        request.status(),
        dispatcher::domain::ConfigurationStatus::Published
    );
    EXPECT_EQ(request.snapshot().config_version(), 7);
}

TEST(ConfigurationReloadApiResultTests, SuccessResultContainsSnapshot)
{
    const auto result =
        dispatcher::api::ConfigurationReloadApiResult::success(
            make_api_configuration_snapshot(7)
        );

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(result.status(), dispatcher::api::ApiStatus::Success);
    EXPECT_TRUE(result.has_snapshot());
    EXPECT_EQ(result.snapshot().config_version(), 7);
}

TEST(ConfigurationReloadApiResultTests, FailureResultDoesNotContainSnapshot)
{
    const auto result =
        dispatcher::api::ConfigurationReloadApiResult::failure(
            dispatcher::api::ApiStatus::ValidationError,
            "configuration.reload",
            "7",
            "status",
            "configuration must be published"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::ApiStatus::ValidationError
    );

    EXPECT_FALSE(result.has_snapshot());

    EXPECT_EQ(result.error().operation, "configuration.reload");
    EXPECT_EQ(result.error().resource, "7");
    EXPECT_EQ(result.error().field, "status");
    EXPECT_EQ(result.error().message, "configuration must be published");

    EXPECT_THROW(
        (void)result.snapshot(),
        std::logic_error
    );
}

TEST(DispatcherConfigurationApiTests, QueryWithoutStorageRepositoryFails)
{
    dispatcher::runtime::DispatcherRuntime runtime;
    dispatcher::api::DispatcherConfigurationApi api(runtime);

    dispatcher::api::ConfigurationApi& configuration_api = api;

    const auto result = configuration_api.query(
        dispatcher::api::ConfigurationQueryRequest{}
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(result.status(), dispatcher::api::ApiStatus::StorageError);
    EXPECT_EQ(result.error().operation, "configuration.query");
    EXPECT_EQ(result.error().resource, "storage_repository");
}

TEST(DispatcherConfigurationApiTests, QueryWithStorageRepositoryReturnsSnapshots)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;

    dispatcher::runtime::DispatcherRuntime runtime(
        make_api_configuration_snapshot(7),
        storage_repository
    );

    dispatcher::api::DispatcherConfigurationApi api(runtime);
    dispatcher::api::ConfigurationApi& configuration_api = api;

    const auto result = configuration_api.query(
        dispatcher::api::ConfigurationQueryRequest{
            .config_version = 7,
            .page = dispatcher::api::ApiPageRequest{
                .offset = 0,
                .limit = 25
            }
        }
    );

    ASSERT_TRUE(result.ok());

    ASSERT_EQ(result.snapshot_count(), 1);
    EXPECT_EQ(result.snapshots().front().config_version(), 7);

    EXPECT_EQ(result.page().offset(), 0);
    EXPECT_EQ(result.page().limit(), 25);
    EXPECT_EQ(result.page().returned_count(), 1);
    EXPECT_EQ(result.page().total_count(), 1);
}

TEST(DispatcherConfigurationApiTests, ReloadPublishedConfigurationWorks)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;

    dispatcher::runtime::DispatcherRuntime runtime(
        make_api_configuration_snapshot(1),
        storage_repository
    );

    dispatcher::api::DispatcherConfigurationApi api(runtime);
    dispatcher::api::ConfigurationApi& configuration_api = api;

    const auto result = configuration_api.reload(
        dispatcher::api::ConfigurationReloadRequest(
            make_api_configuration_snapshot(2)
        )
    );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_snapshot());

    EXPECT_EQ(result.snapshot().config_version(), 2);

    const auto stored_query = storage_repository.configuration_storage().query(
        dispatcher::storage::ConfigurationStorageQuery{
            .config_version = 2
        }
    );

    ASSERT_TRUE(stored_query.ok());
    ASSERT_EQ(stored_query.snapshot_count(), 1);
    EXPECT_EQ(stored_query.snapshots().front().config_version(), 2);
}

TEST(DispatcherConfigurationApiTests, ReloadDraftConfigurationFails)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;

    dispatcher::runtime::DispatcherRuntime runtime(
        make_api_configuration_snapshot(1),
        storage_repository
    );

    dispatcher::api::DispatcherConfigurationApi api(runtime);
    dispatcher::api::ConfigurationApi& configuration_api = api;

    const auto result = configuration_api.reload(
        dispatcher::api::ConfigurationReloadRequest(
            make_api_configuration_snapshot(
                2,
                dispatcher::domain::ConfigurationStatus::Draft
            )
        )
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::ApiStatus::ValidationError
    );

    EXPECT_FALSE(result.has_snapshot());

    EXPECT_EQ(result.error().operation, "configuration.reload");
    EXPECT_EQ(result.error().resource, "2");
    EXPECT_TRUE(result.error().has_field());
    EXPECT_TRUE(result.error().has_message());

    const auto rejected_query =
        storage_repository.configuration_storage().query(
            dispatcher::storage::ConfigurationStorageQuery{
                .config_version = 2
            }
        );

    ASSERT_TRUE(rejected_query.ok());
    EXPECT_TRUE(rejected_query.empty());
}

TEST(DispatcherConfigurationApiTests, ExposesWrappedRuntime)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;

    dispatcher::runtime::DispatcherRuntime runtime(
        make_api_configuration_snapshot(7),
        storage_repository
    );

    dispatcher::api::DispatcherConfigurationApi api(runtime);

    EXPECT_EQ(&api.runtime(), &runtime);

    const auto& const_api = api;

    EXPECT_EQ(&const_api.runtime(), &runtime);
}