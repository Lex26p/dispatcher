#include <dispatcher/domain/configuration_snapshot_builder.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/storage/configuration_storage_query.hpp>
#include <dispatcher/storage/in_memory_storage_repository.hpp>
#include <dispatcher/storage/storage_status.hpp>

#include <gtest/gtest.h>

namespace
{
    dispatcher::domain::ConfigurationSnapshot make_configuration_snapshot(
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

TEST(InMemoryStorageRepositoryTests, RepositoryStartsEmpty)
{
    dispatcher::storage::InMemoryStorageRepository repository;

    EXPECT_TRUE(repository.in_memory_history_storage().empty());
    EXPECT_TRUE(repository.in_memory_alarm_event_storage().empty());
    EXPECT_TRUE(repository.in_memory_alarm_acknowledgement_storage().empty());
    EXPECT_TRUE(repository.in_memory_configuration_storage().empty());

    EXPECT_EQ(repository.in_memory_history_storage().size(), 0);
    EXPECT_EQ(repository.in_memory_alarm_event_storage().size(), 0);
    EXPECT_EQ(repository.in_memory_alarm_acknowledgement_storage().size(), 0);
    EXPECT_EQ(repository.in_memory_configuration_storage().size(), 0);
}

TEST(InMemoryStorageRepositoryTests, RepositoryExposesStorageInterfaces)
{
    dispatcher::storage::InMemoryStorageRepository repository;

    const auto history_query_result =
        repository.history_storage().query(
            dispatcher::storage::HistoryStorageQuery{}
        );

    EXPECT_TRUE(history_query_result.ok());
    EXPECT_TRUE(history_query_result.empty());

    const auto alarm_event_query_result =
        repository.alarm_event_storage().query(
            dispatcher::storage::AlarmEventStorageQuery{}
        );

    EXPECT_TRUE(alarm_event_query_result.ok());
    EXPECT_TRUE(alarm_event_query_result.empty());

    const auto acknowledgement_query_result =
        repository.alarm_acknowledgement_storage().query(
            dispatcher::storage::AlarmAcknowledgementStorageQuery{}
        );

    EXPECT_TRUE(acknowledgement_query_result.ok());
    EXPECT_TRUE(acknowledgement_query_result.empty());

    const auto configuration_query_result =
        repository.configuration_storage().query(
            dispatcher::storage::ConfigurationStorageQuery{}
        );

    EXPECT_TRUE(configuration_query_result.ok());
    EXPECT_TRUE(configuration_query_result.empty());
}

TEST(InMemoryStorageRepositoryTests, ConfigurationStorageSavesAndQueriesSnapshots)
{
    dispatcher::storage::InMemoryStorageRepository repository;

    const auto save_result_1 = repository.configuration_storage().save(
        make_configuration_snapshot(
            1,
            dispatcher::domain::ConfigurationStatus::Draft
        )
    );

    const auto save_result_2 = repository.configuration_storage().save(
        make_configuration_snapshot(
            2,
            dispatcher::domain::ConfigurationStatus::Published
        )
    );

    ASSERT_TRUE(save_result_1.ok());
    ASSERT_TRUE(save_result_2.ok());

    EXPECT_EQ(repository.in_memory_configuration_storage().size(), 2);

    const auto published_query_result =
        repository.configuration_storage().query(
            dispatcher::storage::ConfigurationStorageQuery{
                .status =
                    dispatcher::domain::ConfigurationStatus::Published
            }
        );

    ASSERT_TRUE(published_query_result.ok());
    ASSERT_EQ(published_query_result.snapshot_count(), 1);
    EXPECT_EQ(published_query_result.snapshots().front().config_version(), 2);

    const auto latest_query_result =
        repository.configuration_storage().query(
            dispatcher::storage::ConfigurationStorageQuery{
                .latest_only = true
            }
        );

    ASSERT_TRUE(latest_query_result.ok());
    ASSERT_EQ(latest_query_result.snapshot_count(), 1);
    EXPECT_EQ(latest_query_result.snapshots().front().config_version(), 2);
}

TEST(InMemoryStorageRepositoryTests, ConfigurationStorageUpsertsByVersion)
{
    dispatcher::storage::InMemoryStorageRepository repository;

    const auto save_draft_result = repository.configuration_storage().save(
        make_configuration_snapshot(
            7,
            dispatcher::domain::ConfigurationStatus::Draft
        )
    );

    const auto save_published_result = repository.configuration_storage().save(
        make_configuration_snapshot(
            7,
            dispatcher::domain::ConfigurationStatus::Published
        )
    );

    ASSERT_TRUE(save_draft_result.ok());
    ASSERT_TRUE(save_published_result.ok());

    EXPECT_EQ(repository.in_memory_configuration_storage().size(), 1);

    const auto query_result = repository.configuration_storage().query(
        dispatcher::storage::ConfigurationStorageQuery{
            .config_version = 7
        }
    );

    ASSERT_TRUE(query_result.ok());
    ASSERT_EQ(query_result.snapshot_count(), 1);
    EXPECT_EQ(
        query_result.snapshots().front().status(),
        dispatcher::domain::ConfigurationStatus::Published
    );
}

TEST(InMemoryStorageRepositoryTests, ConfigurationStorageRemoveByVersionWorks)
{
    dispatcher::storage::InMemoryStorageRepository repository;

    ASSERT_TRUE(
        repository.configuration_storage()
        .save(make_configuration_snapshot(7))
        .ok()
    );

    ASSERT_EQ(repository.in_memory_configuration_storage().size(), 1);

    const auto remove_result =
        repository.configuration_storage().remove_by_version(7);

    EXPECT_TRUE(remove_result.ok());
    EXPECT_TRUE(repository.in_memory_configuration_storage().empty());
}

TEST(InMemoryStorageRepositoryTests, ConfigurationStorageRemoveMissingVersionFails)
{
    dispatcher::storage::InMemoryStorageRepository repository;

    const auto remove_result =
        repository.configuration_storage().remove_by_version(404);

    EXPECT_FALSE(remove_result.ok());

    EXPECT_EQ(
        remove_result.status(),
        dispatcher::storage::StorageStatus::NotFound
    );

    EXPECT_EQ(remove_result.operation(), "configuration.remove_by_version");
    EXPECT_EQ(remove_result.key(), "404");
}

TEST(InMemoryStorageRepositoryTests, ConfigurationStorageNameFilterIsUnsupported)
{
    dispatcher::storage::InMemoryStorageRepository repository;

    const auto query_result = repository.configuration_storage().query(
        dispatcher::storage::ConfigurationStorageQuery{
            .name = "production"
        }
    );

    EXPECT_TRUE(query_result.failed());

    EXPECT_EQ(
        query_result.status(),
        dispatcher::storage::StorageStatus::UnsupportedOperation
    );
}

TEST(InMemoryStorageRepositoryTests, ClearEmptiesAllStorages)
{
    dispatcher::storage::InMemoryStorageRepository repository;

    ASSERT_TRUE(
        repository.configuration_storage()
        .save(make_configuration_snapshot(7))
        .ok()
    );

    ASSERT_FALSE(repository.in_memory_configuration_storage().empty());

    EXPECT_TRUE(repository.history_storage().clear().ok());
    EXPECT_TRUE(repository.alarm_event_storage().clear().ok());
    EXPECT_TRUE(repository.alarm_acknowledgement_storage().clear().ok());
    EXPECT_TRUE(repository.configuration_storage().clear().ok());

    EXPECT_TRUE(repository.in_memory_history_storage().empty());
    EXPECT_TRUE(repository.in_memory_alarm_event_storage().empty());
    EXPECT_TRUE(repository.in_memory_alarm_acknowledgement_storage().empty());
    EXPECT_TRUE(repository.in_memory_configuration_storage().empty());
}

TEST(InMemoryStorageRepositoryTests, UnsupportedRemoveOperationsReturnFailure)
{
    dispatcher::storage::InMemoryStorageRepository repository;

    const auto history_remove_result =
        repository.history_storage().remove_by_tag(
            dispatcher::domain::TagId{ "tag-1" }
        );

    EXPECT_TRUE(history_remove_result.failed());

    EXPECT_EQ(
        history_remove_result.status(),
        dispatcher::storage::StorageStatus::UnsupportedOperation
    );

    const auto alarm_event_remove_result =
        repository.alarm_event_storage().remove_by_alarm(
            dispatcher::domain::AlarmId{ "alarm-1" }
        );

    EXPECT_TRUE(alarm_event_remove_result.failed());

    EXPECT_EQ(
        alarm_event_remove_result.status(),
        dispatcher::storage::StorageStatus::UnsupportedOperation
    );

    const auto acknowledgement_remove_result =
        repository.alarm_acknowledgement_storage().remove_by_operator(
            "operator-1"
        );

    EXPECT_TRUE(acknowledgement_remove_result.failed());

    EXPECT_EQ(
        acknowledgement_remove_result.status(),
        dispatcher::storage::StorageStatus::UnsupportedOperation
    );
}