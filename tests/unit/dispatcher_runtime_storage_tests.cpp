#include <dispatcher/domain/configuration_snapshot_builder.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/runtime/dispatcher_runtime.hpp>
#include <dispatcher/storage/configuration_storage_query.hpp>
#include <dispatcher/storage/in_memory_storage_repository.hpp>

#include <gtest/gtest.h>

#include <cstdint>

namespace
{
    dispatcher::domain::ConfigurationSnapshot make_runtime_storage_configuration(
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

TEST(DispatcherRuntimeStorageTests, DefaultRuntimeHasNoStorageRepository)
{
    dispatcher::runtime::DispatcherRuntime runtime;

    EXPECT_FALSE(runtime.has_storage_repository());
    EXPECT_EQ(runtime.storage_repository(), nullptr);
}

TEST(DispatcherRuntimeStorageTests, RuntimeCanBeConstructedWithStorageRepository)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;

    dispatcher::runtime::DispatcherRuntime runtime(storage_repository);

    EXPECT_TRUE(runtime.has_storage_repository());
    EXPECT_EQ(runtime.storage_repository(), &storage_repository);

    const auto query_result =
        storage_repository.configuration_storage().query(
            dispatcher::storage::ConfigurationStorageQuery{
                .latest_only = true
            }
        );

    ASSERT_TRUE(query_result.ok());
    ASSERT_EQ(query_result.snapshot_count(), 1);

    EXPECT_EQ(
        query_result.snapshots().front().config_version(),
        runtime.telemetry_ingestor().configuration_snapshot().config_version()
    );
}

TEST(DispatcherRuntimeStorageTests, RuntimePersistsInitialTelemetryConfiguration)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;

    dispatcher::runtime::DispatcherRuntime runtime(
        make_runtime_storage_configuration(7),
        storage_repository
    );

    EXPECT_TRUE(runtime.has_storage_repository());

    const auto query_result =
        storage_repository.configuration_storage().query(
            dispatcher::storage::ConfigurationStorageQuery{
                .config_version = 7
            }
        );

    ASSERT_TRUE(query_result.ok());
    ASSERT_EQ(query_result.snapshot_count(), 1);
    EXPECT_EQ(query_result.snapshots().front().config_version(), 7);
}

TEST(DispatcherRuntimeStorageTests, RuntimePersistsReloadedTelemetryConfiguration)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;

    dispatcher::runtime::DispatcherRuntime runtime(
        make_runtime_storage_configuration(1),
        storage_repository
    );

    const auto reload_result = runtime.reload_telemetry_configuration(
        make_runtime_storage_configuration(2)
    );

    ASSERT_FALSE(reload_result.has_errors());

    const auto query_result =
        storage_repository.configuration_storage().query(
            dispatcher::storage::ConfigurationStorageQuery{
                .config_version = 2
            }
        );

    ASSERT_TRUE(query_result.ok());
    ASSERT_EQ(query_result.snapshot_count(), 1);
    EXPECT_EQ(query_result.snapshots().front().config_version(), 2);
}

TEST(DispatcherRuntimeStorageTests, RuntimeDoesNotPersistRejectedTelemetryConfiguration)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;

    dispatcher::runtime::DispatcherRuntime runtime(
        make_runtime_storage_configuration(1),
        storage_repository
    );

    const auto reload_result = runtime.reload_telemetry_configuration(
        make_runtime_storage_configuration(
            2,
            dispatcher::domain::ConfigurationStatus::Draft
        )
    );

    ASSERT_TRUE(reload_result.has_errors());

    const auto rejected_query_result =
        storage_repository.configuration_storage().query(
            dispatcher::storage::ConfigurationStorageQuery{
                .config_version = 2
            }
        );

    ASSERT_TRUE(rejected_query_result.ok());
    EXPECT_TRUE(rejected_query_result.empty());

    const auto original_query_result =
        storage_repository.configuration_storage().query(
            dispatcher::storage::ConfigurationStorageQuery{
                .config_version = 1
            }
        );

    ASSERT_TRUE(original_query_result.ok());
    ASSERT_EQ(original_query_result.snapshot_count(), 1);
}