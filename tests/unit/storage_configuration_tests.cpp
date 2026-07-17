#include <dispatcher/domain/configuration_snapshot_builder.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/storage/configuration_snapshot_query_result.hpp>
#include <dispatcher/storage/configuration_storage.hpp>
#include <dispatcher/storage/configuration_storage_query.hpp>
#include <dispatcher/storage/storage_result.hpp>
#include <dispatcher/storage/storage_status.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

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

    class FakeConfigurationStorage final
        : public dispatcher::storage::ConfigurationStorage
    {
    public:
        [[nodiscard]] dispatcher::storage::StorageResult save(
            const dispatcher::domain::ConfigurationSnapshot& snapshot
        ) override
        {
            last_saved_config_version_ = snapshot.config_version();

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::ConfigurationSnapshotQueryResult query(
            const dispatcher::storage::ConfigurationStorageQuery& query
        ) const override
        {
            last_query_ = query;

            return dispatcher::storage::ConfigurationSnapshotQueryResult::success(
                {}
            );
        }

        [[nodiscard]] dispatcher::storage::StorageResult remove_by_version(
            std::uint64_t config_version
        ) override
        {
            last_removed_config_version_ = config_version;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::StorageResult clear() override
        {
            clear_called_ = true;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] const std::optional<std::uint64_t>&
            last_saved_config_version() const noexcept
        {
            return last_saved_config_version_;
        }

        [[nodiscard]] const std::optional<std::uint64_t>&
            last_removed_config_version() const noexcept
        {
            return last_removed_config_version_;
        }

        [[nodiscard]] const std::optional<
            dispatcher::storage::ConfigurationStorageQuery
        >&
            last_query() const noexcept
        {
            return last_query_;
        }

        [[nodiscard]] bool clear_called() const noexcept
        {
            return clear_called_;
        }

    private:
        std::optional<std::uint64_t> last_saved_config_version_;
        std::optional<std::uint64_t> last_removed_config_version_;

        mutable std::optional<
            dispatcher::storage::ConfigurationStorageQuery
        > last_query_;

        bool clear_called_{ false };
    };
}

TEST(ConfigurationStorageQueryTests, DefaultQueryIsUnbounded)
{
    const dispatcher::storage::ConfigurationStorageQuery query;

    EXPECT_FALSE(query.has_config_version());
    EXPECT_FALSE(query.has_status());
    EXPECT_FALSE(query.has_name());
    EXPECT_FALSE(query.has_limit());
    EXPECT_FALSE(query.requests_latest_only());
}

TEST(ConfigurationStorageQueryTests, QueryPredicatesReflectConfiguredFields)
{
    dispatcher::storage::ConfigurationStorageQuery query;

    query.config_version = 7;
    query.status = dispatcher::domain::ConfigurationStatus::Published;
    query.name = "production";
    query.limit = 10;
    query.latest_only = true;

    EXPECT_TRUE(query.has_config_version());
    EXPECT_TRUE(query.has_status());
    EXPECT_TRUE(query.has_name());
    EXPECT_TRUE(query.has_limit());
    EXPECT_TRUE(query.requests_latest_only());
}

TEST(ConfigurationStorageQueryTests, EmptyNameIsNotConsideredConfigured)
{
    dispatcher::storage::ConfigurationStorageQuery query;

    query.name = "";

    EXPECT_FALSE(query.has_name());
}

TEST(ConfigurationSnapshotQueryResultTests, SuccessResultWorks)
{
    const auto result =
        dispatcher::storage::ConfigurationSnapshotQueryResult::success({});

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::storage::StorageStatus::Success
    );

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.snapshot_count(), 0);
    EXPECT_TRUE(result.snapshots().empty());
}

TEST(ConfigurationSnapshotQueryResultTests, FailureResultCapturesError)
{
    const auto result =
        dispatcher::storage::ConfigurationSnapshotQueryResult::failure(
            dispatcher::storage::StorageStatus::BackendUnavailable,
            "configuration.query",
            "config-version-7",
            "backend is down"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::storage::StorageStatus::BackendUnavailable
    );

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.snapshot_count(), 0);

    EXPECT_EQ(result.error().operation, "configuration.query");
    EXPECT_EQ(result.error().key, "config-version-7");
    EXPECT_EQ(result.error().message, "backend is down");
}

TEST(ConfigurationStorageInterfaceTests, InterfaceCanBeImplemented)
{
    FakeConfigurationStorage storage;

    const auto snapshot = make_configuration_snapshot(7);

    const auto save_result = storage.save(snapshot);

    EXPECT_TRUE(save_result.ok());

    ASSERT_TRUE(storage.last_saved_config_version().has_value());
    EXPECT_EQ(storage.last_saved_config_version().value(), 7);

    const dispatcher::storage::ConfigurationStorageQuery query{
        .config_version = 7,
        .status = dispatcher::domain::ConfigurationStatus::Published,
        .name = "production",
        .limit = 10,
        .latest_only = true
    };

    const auto query_result = storage.query(query);

    EXPECT_TRUE(query_result.ok());
    EXPECT_TRUE(query_result.empty());

    ASSERT_TRUE(storage.last_query().has_value());
    EXPECT_TRUE(storage.last_query()->has_config_version());
    EXPECT_TRUE(storage.last_query()->has_status());
    EXPECT_TRUE(storage.last_query()->has_name());
    EXPECT_TRUE(storage.last_query()->has_limit());
    EXPECT_TRUE(storage.last_query()->requests_latest_only());

    const auto remove_result = storage.remove_by_version(7);

    EXPECT_TRUE(remove_result.ok());

    ASSERT_TRUE(storage.last_removed_config_version().has_value());
    EXPECT_EQ(storage.last_removed_config_version().value(), 7);

    const auto clear_result = storage.clear();

    EXPECT_TRUE(clear_result.ok());
    EXPECT_TRUE(storage.clear_called());
}